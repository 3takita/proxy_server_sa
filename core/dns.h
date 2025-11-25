#ifndef DNS_H
#define DNS_H

#include <string>
#include <vector>
#include <cstddef>

struct IPAddress {
    std::string str;              // Printable IP (IPv4 or IPv6)
    int family;                   // AF_INET or AF_INET6
    std::vector<std::byte> bytes; // Raw binary address
};

// Resolve hostname to IPv4 and/or IPv6 addresses
std::vector<IPAddress> ResolveHostname(const std::string& hostname);

#endif // DNS_H
