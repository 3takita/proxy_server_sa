#include "network.h"

#include <vector>
#include <unordered_map>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

namespace core {

    // ---------------------
    // macOS Network Wrapper (poll-based)
    // ---------------------

    bool InitializeNetwork() {
        // No setup required for macOS
        return true;
    }

    void ShutDownNetwork() {
        // No cleanup required for macOS
    }

    SocketIdentifier CreateListeningSocket(const std::string& bind_host, int port, int max_connection_count) {
        struct addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        addrinfo* result = nullptr;
        const std::string port_str = std::to_string(port);

        int status = getaddrinfo(bind_host.empty() ? nullptr : bind_host.c_str(),
                                 port_str.c_str(), &hints, &result);
        if (status) {
            return -1;
        }

        SocketIdentifier listening_socket_id = -1;
        for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next) {
            SocketIdentifier socket_id = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (socket_id < 0) {
                continue;
            }

            int optval = 1;
            ::setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

            if (::bind(socket_id, ai->ai_addr, ai->ai_addrlen) < 0) {
                ::close(socket_id);
                continue;
            }

            if (::listen(socket_id, max_connection_count) < 0) {
                ::close(socket_id);
                continue;
            }

            listening_socket_id = socket_id;
            break;
        }

        ::freeaddrinfo(result);
        return listening_socket_id;
    }

    bool SetSocketNonBlocking(SocketIdentifier socket) {
        int flags = ::fcntl(socket, F_GETFL, 0);
        if (flags == -1) {
            return false;
        }
        if (::fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
            return false;
        }
        return true;
    }

    // Simple poll-based poller implementation
    using PollerStore = std::unordered_map<EventPollerIdentifier, std::vector<pollfd>>;
    static PollerStore g_pollers;
    static EventPollerIdentifier g_next_poller_id = 1;

    EventPollerIdentifier CreateEventPoller() {
        const EventPollerIdentifier id = g_next_poller_id++;
        g_pollers.emplace(id, std::vector<pollfd>());
        return id;
    }

    bool RegisterReadEvent(EventPollerIdentifier poller, SocketIdentifier socket) {
        auto it = g_pollers.find(poller);
        if (it == g_pollers.end()) return false;

        auto &vec = it->second;
        auto found = std::find_if(vec.begin(), vec.end(), [&](const pollfd &p){ return p.fd == socket; });
        if (found == vec.end()) {
            pollfd p{};
            p.fd = socket;
            p.events = POLLIN;
            p.revents = 0;
            vec.push_back(p);
        } else {
            found->events |= POLLIN;
        }
        return true;
    }

    bool UpdateEventInterest(EventPollerIdentifier poller, SocketIdentifier socket, bool readable, bool writable) {
        auto it = g_pollers.find(poller);
        if (it == g_pollers.end()) return false;

        auto &vec = it->second;
        auto found = std::find_if(vec.begin(), vec.end(), [&](const pollfd &p){ return p.fd == socket; });
        if (found == vec.end()) {
            // If not present, add it with the requested flags
            pollfd p{};
            p.fd = socket;
            p.events = 0;
            if (readable) p.events |= POLLIN;
            if (writable) p.events |= POLLOUT;
            vec.push_back(p);
            return true;
        }

        short ev = 0;
        if (readable) ev |= POLLIN;
        if (writable) ev |= POLLOUT;
        found->events = ev;
        return true;
    }

    int WaitForEvents(EventPollerIdentifier poller, PollEvent* out_events, int max_events, int timeout_ms) {
        auto it = g_pollers.find(poller);
        if (it == g_pollers.end()) return -1;

        auto &vec = it->second;
        // poll requires nfds = 0..; if vec.empty() pass nullptr with 0
        int n = ::poll(vec.empty() ? nullptr : vec.data(), static_cast<nfds_t>(vec.size()), timeout_ms);
        if (n < 0 && errno != EINTR) return -1;
        if (n <= 0) return n;

        int out_count = 0;
        for (std::size_t i = 0; i < vec.size() && out_count < max_events; ++i) {
            const short rev = vec[i].revents;
            if (rev == 0) continue;

            uint32_t mask = 0;
            if (rev & POLLIN) mask |= kEventReadable;
            if (rev & POLLOUT) mask |= kEventWritable;
            if (rev & POLLERR) mask |= kEventError;
            if (rev & POLLHUP) mask |= kEventHangup;
            if (rev & POLLNVAL) mask |= kEventOther;

            out_events[out_count].fd = vec[i].fd;
            out_events[out_count].mask = mask;
            ++out_count;
        }

        return out_count;
    }

    SocketIdentifier AcceptConnection(SocketIdentifier listening_socket) {
        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);
        SocketIdentifier client_socket = ::accept(listening_socket, reinterpret_cast<sockaddr*>(&addr), &len);
        return client_socket;
    }

    std::ptrdiff_t Receive(SocketIdentifier socket, void* buffer, std::size_t length) {
        auto n = ::recv(socket, buffer, length, 0);
        return static_cast<std::ptrdiff_t>(n);
    }

    std::ptrdiff_t Send(SocketIdentifier socket, const void* buffer, std::size_t length) {
        auto n = ::send(socket, buffer, length, 0);
        return static_cast<std::ptrdiff_t>(n);
    }

    void CloseSocket(SocketIdentifier socket) {
        if (socket >= 0) {
            ::close(socket);
        }
    }

    bool SocketToAddress(SocketIdentifier socket, std::string& ip, uint16_t& port) {
        sockaddr_storage ss{};
        socklen_t len = sizeof(ss);
        if (::getpeername(socket, reinterpret_cast<sockaddr*>(&ss), &len) < 0) {
            return false;
        }

        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];

        int rc = ::getnameinfo(reinterpret_cast<sockaddr*>(&ss), len, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
        if (rc != 0) return false;

        ip = host;
        port = static_cast<uint16_t>(std::stoi(serv));
        return true;
    }

    bool GetSocketInfo(SocketIdentifier socket, ConnectionInfo& info) {
        // TODO: Implement
        return false;
    }

} // namespace core
