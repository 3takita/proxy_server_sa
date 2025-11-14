#include "network.h"

#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>


namespace core {

    // ---------------------
    // Linux Network Wrapper
    // ---------------------

    bool InitializeNetwork() {
        // No setup required for Linux
        return true;
    }

    void ShutDownNetwork() {
        // No cleanup required for Linux
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
            // TODO: Log getaddrinfo error
            // std::cerr << gai_strerror(status);
            // Reason: invalid host, missing DNS, or system resolver issues
            return -1;
        }

        SocketIdentifier listening_socket_id = -1;
        for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next) {
            SocketIdentifier socket_id = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (socket_id < 0) {
                // TODO: Log socket() failure
                // Reason: resource limits, permission issues, or unsupported family type on this kernel
                continue;
            }

            int optval = 1;
            ::setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

            if (ai->ai_family == AF_INET6) {
                int v6only = 0;
                if (::setsockopt(socket_id, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) < 0) {
                    // TODO: Log IPV6_V6ONLY setsockopt failure 
                    // Reason: system policy may enforce v6-only; non-fatal, continue with current socket_id
                }
            }

            if (::bind(socket_id, ai->ai_addr, ai->ai_addrlen) < 0) {
                // TODO: Log bind() failure
                // getnameinfo()
                // Reason: port in use, privileged port, or invalid interface binding
                ::close(socket_id);
                continue; // Try next address info candidate
            }

            if (::listen(socket_id, max_connection_count) < 0) {
                // TODO: Log listen() failure 
                // Include max_connection_count value
                // Reason: resource limits or transient kernel state
                ::close(socket_id);
                continue; // Try next address info candidate
            }

            // Success on the first candidates that binds & listens
            listening_socket_id = socket_id;
            break;
        }

        if (listening_socket_id < 0) {
            // TODO: Log failure to create any listening socket from getaddrinfo results
            // Reason: Every candidate failed; Probably a port issue or IPv4/IPv6 is not enabled
        }

        ::freeaddrinfo(result);
        return listening_socket_id;
    }

    bool SetSocketNonBlocking(SocketIdentifier socket) {
        int flags = ::fcntl(socket, F_GETFL, 0);
        if (flags == -1) {
            // TODO: Log fcntl(GET_FL) error
            // Reason: invalid file descriptor (Socket ID) or kernel issue
            // Continuing may cause blocking in the loop which will tank performance
            return false;
        }
        if (::fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
            // TODO: Log fcntl(F_SETFL) error
            // Reason: cannot enable non-blocking I/O
            return false;
        }
        return true;
    }

    EventPollerIdentifier CreateEventPoller() {
        EventPollerIdentifier epID = ::epoll_create1(0);
        if (epID < 0) {
            // TODO: Log epoll_create1 failure with errno
            // Reason: File Descriptor limits, kernel issues
        }
        return epID;
    }

    bool RegisterReadEvent(EventPollerIdentifier poller, SocketIdentifier socket) {
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = socket;
        if (::epoll_ctl(poller, EPOLL_CTL_ADD, socket, &ev) < 0) {
            // TODO: Log epoll_ctl(ADD) failure with errno, poller handle, and socket fd
            // Reason: invalid descriptors, already registered, or resource constraints
            return false;
        }
        return true;
    }

    bool UpdateEventInterest(EventPollerIdentifier poller, SocketIdentifier socket, bool readable, bool writable) {
        // This function translates the socket's desired interests (read/write)
        // into native epoll flags
        epoll_event ev{};
        ev.data.fd = socket;
        ev.events = 0;
        if (readable) ev.events |= EPOLLIN;
        if (writable) ev.events |= EPOLLOUT;

        if (::epoll_ctl(poller, EPOLL_CTL_MOD, socket, &ev) < 0) {
            // TODO: Log epoll_ctl(MOD) failure with errno, poller handle, and socket fd
            return false;
        }
        return true;
    }

    int WaitForEvents(EventPollerIdentifier poller, PollEvent* out_events, int max_events, int timeout_ms) {
        std::vector<epoll_event> tempEvents(static_cast<std::size_t>(max_events));
        int n = ::epoll_wait(poller, tempEvents.data(), max_events, timeout_ms);
        if (n < 0 && errno != EINTR) {
            // From the epoll man page
            // EINTR --> The call was interrupted by a signal handler before either
            // any of the requested events occurred or the timeout expired
            // therefore we can continue on if we get this error
            // TODO: Log epoll_wait failure with errno
            // Reason: poller invalidation or system call error
            return -1;
        }
        if (n <= 0) return n; // 0 or EINTR --> propage to called

        for (int i = 0; i < n; i++) {
            uint32_t mask = 0;
            const uint32_t ev = tempEvents[i].events;

            if (ev & EPOLLIN)   mask |= kEventReadable;
            if (ev & EPOLLOUT)  mask |= kEventWritable;
            if (ev & EPOLLERR)  mask |= kEventError;
            if (ev & (EPOLLHUP | EPOLLRDHUP)) mask |= kEventHangup;

            out_events[i].fd = tempEvents[i].data.fd;
            out_events[i].mask = mask;
        }

        return n;
    }

    SocketIdentifier AcceptConnection(SocketIdentifier listening_socket) {
        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);
        SocketIdentifier client_socket = ::accept(listening_socket, reinterpret_cast<sockaddr*>(&addr), &len);
        if (client_socket < 0) {
            // TODO: Log accept() failure
            // Check errno, it will be an error or EAGAIN/EWOULDBLOCK
            // It could be a real error or just no more clients to accept
        }
        return client_socket;
    }

    std::ptrdiff_t Receive(SocketIdentifier socket, void* buffer, std::size_t length) {
        auto n = ::recv(socket, buffer, length, 0);
        if (n < 0) {
            // TODO: Log recv() failure with errno unless it is EAGAIN/EWOULDBLOCK/EINTR
            // Reason: network error, fd closed, or would block in non blocking mode
        }
        return static_cast<std::ptrdiff_t>(n);
    }

    std::ptrdiff_t Send(SocketIdentifier socket, const void* buffer, std::size_t length) {
        auto n = ::send(socket, buffer, length, 0);
        return static_cast<std::ptrdiff_t>(n);
    }

    SocketIdentifier ConnectTCPIPv4(uint32_t dst_ipv4, uint16_t dst_port) {
        // Create an IPv4 TCP socket
        SocketIdentifier sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            // TODO: Log socket() creation failure
            return -1;
        }

        // Set the socket non-blocking
        if (!SetSocketNonBlocking(sock)) {
            // TODO: Log socket non-blocked failure
            CloseSocket(sock);
            return -1;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(dst_ipv4);
        addr.sin_port = htons(dst_port);

        int rc = ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        if (rc == 0) {
            // Connected immediately (rare)
            return sock;
        } else if(rc < 0) {
            if (errno == EINPROGRESS) {
                // Connection started, will be detected by EPOLLOUT
                return sock;
            }

            CloseSocket(sock);
            return -1;
        }
        return sock;
    }

    void CloseSocket(SocketIdentifier socket) {
        if (socket >= 0) {
            if (::close(socket) < 0) {
                // TODO: Log close() failure with errno and socket fd
                // Reason: Typically a double free but otherwise a rare error
            }
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

        int rc = ::getnameinfo(reinterpret_cast<sockaddr*>(&ss), len, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICHOST);
        if (rc != 0) return false;

        ip = host;
        port = static_cast<uint16_t>(std::stoi(serv));
        return true;
    }

} // namespace core