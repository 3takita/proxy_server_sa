#include "dns.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <netdb.h>
#include <netdb.h>


DNSForwarder::DNSForwarder(uint16_t listen_port, std::string upstream, uint16_t upstream_port, uint32_t cache_ttl_seconds) {
    listen_port_ = listen_port;
    upstream_ = std::move(upstream);
    upstream_port_ = upstream_port;
    cache_ttl_seconds_ = cache_ttl_seconds;
    sockfd_ = -1;
    running_ = false; 
}

DNSForwarder::~DNSForwarder() {
    stop();
}

bool DNSForwarder::start() {
    if (running_) return false;

    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        perror("socket");
        return false;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port_);

    // Allow reuse
    int opt = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    running_ = true;
    listener_thread_ = std::thread(&DNSForwarder::listenerLoop, this);
    return true;
}

void DNSForwarder::stop() {
    if (!running_) return;
    running_ = false;
    // wake up recvfrom by closing socket
    if (sockfd_ >= 0) close(sockfd_);
    sockfd_ = -1;
    if (listener_thread_.joinable()) listener_thread_.join();
}

void DNSForwarder::listenerLoop() {
    const size_t BUF_SIZE = 4096;
    std::vector<uint8_t> buf(BUF_SIZE);

    while (running_) {
        sockaddr_in client;
        socklen_t clientlen = sizeof(client);
        ssize_t n = recvfrom(sockfd_, buf.data(), (int)buf.size(), 0,
                             reinterpret_cast<sockaddr*>(&client), &clientlen);

        if (n <= 0) {
            if (!running_) break;
            continue;
        }
        std::vector<uint8_t> query(buf.begin(), buf.begin() + n);

        // Extract qname + qtype for cache key
        std::string qname;
        uint16_t qtype = 0;
        size_t qname_end = 0;
        if (!extractQNameAndType(query.data(), query.size(), qname, qtype, qname_end)) {
            // malformed query - ignore
            continue;
        }
        std::string key = makeCacheKey(qname, qtype);

        std::vector<uint8_t> reply;
        if (cacheLookup(key, reply)) {
            // replace transaction ID in cached reply with incoming query's ID
            if (reply.size() >= 2 && query.size() >= 2) {
                std::vector<uint8_t> outgoing = reply;
                outgoing[0] = query[0];
                outgoing[1] = query[1];
                ssize_t sent = sendto(sockfd_, outgoing.data(), (int)outgoing.size(), 0,
                                      reinterpret_cast<sockaddr*>(&client), clientlen);
                (void)sent;
            }
            continue;
        }

        // Not in cache: forward to upstream
        std::vector<uint8_t> upstream_resp;
        if (forwardToUpstream(query, upstream_resp)) {
            // store in cache
            cacheInsert(key, upstream_resp);

            // replace transaction ID with query's ID (the upstream response has original txn id)
            if (upstream_resp.size() >= 2 && query.size() >= 2) {
                upstream_resp[0] = query[0];
                upstream_resp[1] = query[1];
            }
            ssize_t sent = sendto(sockfd_, upstream_resp.data(), (int)upstream_resp.size(), 0,
                                  reinterpret_cast<sockaddr*>(&client), clientlen);
            (void)sent;
        } else {
            // upstream failed - optionally reply with SERVFAIL (not implemented), here we just drop
            // Could construct a DNS header with RCODE=2 and send back
        }

        // occasionally cleanup expired entries
        cleanupExpired();
    }
}

bool DNSForwarder::forwardToUpstream(const std::vector<uint8_t>& query, std::vector<uint8_t>& response, int timeout_ms) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { perror("socket(upstream)"); return false; }

    sockaddr_in upstream_addr{};
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_port = htons(upstream_port_);

    // Resolve hostname if needed
    struct in_addr addr;
    if (inet_pton(AF_INET, upstream_.c_str(), &addr) <= 0) {
        // try getaddrinfo
        struct addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo(upstream_.c_str(), nullptr, &hints, &res) != 0 || !res) {
            close(s);
            return false;
        }
        upstream_addr.sin_addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
        freeaddrinfo(res);
    } else {
        upstream_addr.sin_addr = addr;
    }

    // send query
    ssize_t sent = sendto(s, query.data(), query.size(), 0,
                          reinterpret_cast<sockaddr*>(&upstream_addr), sizeof(upstream_addr));
    if (sent < 0) { perror("sendto(upstream)"); close(s); return false; }

    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint8_t buf[4096];
    sockaddr_in from{};
    socklen_t fromlen = sizeof(from);
    ssize_t n = recvfrom(s, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &fromlen);
    if (n <= 0) { close(s); return false; }

    response.assign(buf, buf + n);
    close(s);
    return true;
}

bool DNSForwarder::resolveQueryUDP(const std::vector<uint8_t>& query, std::vector<uint8_t>& response) {
    // helper for synchronous resolving (tries cache then upstream)
    std::string qname;
    uint16_t qtype = 0;
    size_t qname_end = 0;
    if (!extractQNameAndType(query.data(), query.size(), qname, qtype, qname_end)) return false;
    std::string key = makeCacheKey(qname, qtype);
    if (cacheLookup(key, response)) {
        if (response.size() >= 2 && query.size() >= 2) {
            response[0] = query[0];
            response[1] = query[1];
        }
        return true;
    }
    if (!forwardToUpstream(query, response)) return false;
    cacheInsert(key, response);
    if (response.size() >= 2 && query.size() >= 2) {
        response[0] = query[0];
        response[1] = query[1];
    }
    return true;
}

bool DNSForwarder::extractQNameAndType(const uint8_t* buf, size_t len, std::string& qname, uint16_t& qtype, size_t& qname_end_offset) {
    // Minimal DNS question parser: skip 12-byte header, then parse QNAME labels
    if (len < 12) return false;
    size_t pos = 12;
    if (pos >= len) return false;

    std::string name;
    bool first = true;
    while (pos < len) {
        uint8_t label_len = buf[pos];
        if (label_len == 0) {
            pos++;
            break;
        }
        // sanity checks (no compression allowed in question)
        if (label_len & 0xC0) {
            // compression pointer in question is unusual; fail parsing
            return false;
        }
        pos++;
        if (pos + label_len > len) return false;
        if (!first) name.push_back('.');
        name.append(reinterpret_cast<const char*>(buf + pos), label_len);
        pos += label_len;
        first = false;
    }
    if (pos + 4 > len) return false;
    // next two bytes: QTYPE, next two bytes: QCLASS
    qtype = (uint16_t(buf[pos]) << 8) | uint16_t(buf[pos + 1]);
    qname = std::move(name);
    qname_end_offset = pos + 4;
    return true;
}

std::string DNSForwarder::makeCacheKey(const std::string& qname, uint16_t qtype) {
    return qname + ":" + std::to_string(qtype);
}

void DNSForwarder::cacheInsert(const std::string& key, const std::vector<uint8_t>& response) {
    CacheEntry e;
    e.response = response;
    e.expiry = std::chrono::steady_clock::now() + std::chrono::seconds(cache_ttl_seconds_);
    std::lock_guard<std::mutex> g(cache_mutex_);
    cache_[key] = std::move(e);
}

bool DNSForwarder::cacheLookup(const std::string& key, std::vector<uint8_t>& response) {
    std::lock_guard<std::mutex> g(cache_mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end()) return false;
    if (std::chrono::steady_clock::now() > it->second.expiry) {
        cache_.erase(it);
        return false;
    }
    response = it->second.response;
    return true;
}

void DNSForwarder::cleanupExpired() {
    std::lock_guard<std::mutex> g(cache_mutex_);
    auto now = std::chrono::steady_clock::now();
    for (auto it = cache_.begin(); it != cache_.end(); ) {
        if (now > it->second.expiry) it = cache_.erase(it);
        else ++it;
    }
}