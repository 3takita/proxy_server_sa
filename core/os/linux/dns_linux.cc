// dns_linux.cc
#include "dns.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <array>
#include <cstddef>

struct IPAddress {
    std::string str;              // String representation
    int family;                   // AF_INET or AF_INET6
    std::vector<std::byte> bytes; // Raw bytes
};

// Resolve hostname to IPv4 or IPv6 addresses
std::vector<IPAddress> ResolveHostname(const std::string& hostname) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;      // Allow both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM;  // Stream socket (TCP)
    
    addrinfo* res = nullptr;
    int rc = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    if (rc != 0 || res == nullptr) {
        std::cerr << "getaddrinfo failed: " << gai_strerror(rc) << "\n";
        return {};
    }

    std::vector<IPAddress> results;

    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        char buf[INET6_ADDRSTRLEN] = {0};
        IPAddress ip;

        if (p->ai_family == AF_INET) {
            sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
            inet_ntop(AF_INET, &ipv4->sin_addr, buf, sizeof(buf));
            ip.family = AF_INET;
            ip.str = buf;
            ip.bytes.resize(4);
            std::memcpy(ip.bytes.data(), &ipv4->sin_addr, 4);
        } else if (p->ai_family == AF_INET6) {
            sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(p->ai_addr);
            inet_ntop(AF_INET6, &ipv6->sin6_addr, buf, sizeof(buf));
            ip.family = AF_INET6;
            ip.str = buf;
            ip.bytes.resize(16);
            std::memcpy(ip.bytes.data(), &ipv6->sin6_addr, 16);
        } else {
            continue;
        }

        results.push_back(ip);
    }

    freeaddrinfo(res);
    return results;
}
/*
int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <hostname>\n";
        return 1;
    }

    const auto hostname = argv[1];
    auto ips = ResolveHostname(hostname);

    if (ips.empty()) {
        std::cout << "No addresses found for " << hostname << "\n";
        return 1;
    }

    std::cout << "Addresses for " << hostname << ":\n";
    for (const auto& ip : ips) {
        std::cout << "  " << (ip.family == AF_INET ? "IPv4" : "IPv6") << ": " << ip.str << "\n";
    }

    return 0;
}
    */
