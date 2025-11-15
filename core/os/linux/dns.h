#ifndef DNS_H
#define DNS_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

class DNSForwarder {
public:
    // listen_port: UDP port to listen on (default 5353 to avoid root requirement)
    // upstream: upstream DNS server IP (default 8.8.8.8)
    // upstream_port: upstream DNS server port (default 53)
    // cache_ttl_seconds: default TTL for cached entries (if not parsed from response)
    DNSForwarder(uint16_t listen_port = 5353,
                 std::string upstream = "8.8.8.8",
                 uint16_t upstream_port = 53,
                 uint32_t cache_ttl_seconds = 300);

    ~DNSForwarder();

    // start listening and forwarding
    bool start();

    // stop the background listener and join thread
    void stop();

    // synchronous resolve helper (tries cache then upstream) returns true if got a response
    bool resolveQueryUDP(const std::vector<uint8_t>& query,
                         std::vector<uint8_t>& response);

private:
    struct CacheEntry {
        std::vector<uint8_t> response;
        std::chrono::steady_clock::time_point expiry;
    };

    uint16_t listen_port_;
    std::string upstream_;
    uint16_t upstream_port_;
    uint32_t cache_ttl_seconds_;

    int sockfd_; // listening UDP socket
    std::atomic<bool> running_;
    std::thread listener_thread_;
    std::mutex cache_mutex_;
    std::unordered_map<std::string, CacheEntry> cache_;

    // internal helpers
    void listenerLoop();
    bool forwardToUpstream(const std::vector<uint8_t>& query, std::vector<uint8_t>& response, int timeout_ms = 2000);
    static bool extractQNameAndType(const uint8_t* buf, size_t len, std::string& qname, uint16_t& qtype, size_t& qname_end_offset);
    static std::string makeCacheKey(const std::string& qname, uint16_t qtype);
    void cacheInsert(const std::string& key, const std::vector<uint8_t>& response);
    bool cacheLookup(const std::string& key, std::vector<uint8_t>& response);
    void cleanupExpired();
};

#endif // DNS_H
