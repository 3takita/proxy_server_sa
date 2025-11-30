#include "dns.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

std::vector<IPAddress> ResolveHostname(const std::string& hostname) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return {};
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;
    int rc = GetAddrInfoA(hostname.c_str(), nullptr, &hints, &res);
    if (rc != 0 || res == nullptr) {
        std::cerr << "GetAddrInfo failed: " << gai_strerrorA(rc) << "\n";
        WSACleanup();
        return {};
    }

    std::vector<IPAddress> results;

    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        char buf[INET6_ADDRSTRLEN] = {0};
        IPAddress ip;

        if (p->ai_family == AF_INET) {
            auto* ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
            InetNtopA(AF_INET, &ipv4->sin_addr, buf, sizeof(buf));

            ip.family = AF_INET;
            ip.str = buf;
            ip.bytes.resize(4);
            std::memcpy(ip.bytes.data(), &ipv4->sin_addr, 4);

        } else if (p->ai_family == AF_INET6) {
            auto* ipv6 = reinterpret_cast<sockaddr_in6*>(p->ai_addr);
            InetNtopA(AF_INET6, &ipv6->sin6_addr, buf, sizeof(buf));

            ip.family = AF_INET6;
            ip.str = buf;
            ip.bytes.resize(16);
            std::memcpy(ip.bytes.data(), &ipv6->sin6_addr, 16);
        }
    }

    FreeAddrInfoA(res);
    WSACleanup();
    return results;
}
