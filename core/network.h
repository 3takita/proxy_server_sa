#ifndef PROXY_SERVER_CORE_NETWORK_H_
#define PROXY_SERVER_CORE_NETWORK_H_

#include <string>

namespace core {

    // File Descriptor in Linux
    // SOCKET in windows
    using SocketIdentifier = int;
    using EventPollerIdentifier = int;

    // ===========================
    // Generic Network Abstraction
    // ===========================

    // Initializes the network subsystem
    // On Linux this is a no-op
    // On Windows this calls WSAStartup
    bool InitializeNetwork();

    // Cleans up network resources
    // On Linux this is a no-op
    // On Windows this calls WSACleanup
    void ShutDownNetwork();

    // Creates a TCP listening socket
    // Returns -1 on failure
    SocketIdentifier CreateListeningSocket(const std::string& bind_host, int port, int max_connection_count);

    // Future use for SOCKS5 
    // SocketIdentifier CreateListeningSocketUDP(const std::string& bind_ip, int port);

    // Sets the socket to non-blocking mode
    // Returns true on success, false on failure
    bool SetSocketNonBlocking(SocketIdentifier id);

    // Creates an event poller handle
    // On Linux this is epoll
    // On Windows this is IOCP
    // Returns -1 on failure
    EventPollerIdentifier CreateEventPoller();

    // Registers a socket with the poller for reading events
    // Returns true on success, false on failure
    bool RegisterReadEvent(EventPollerIdentifier poller, SocketIdentifier socket);

    // Waits for socket events
    // Returns the number of triggered events
    // The parameter `events` and `max_events` determine output capacity
    int WaitForEvents(EventPollerIdentifier poller, void* events, int max_events, int timeout_ms);

    // CLoses a socket safely
    void CloseSocket(SocketIdentifier socket);

}   // namespace core

#endif //PROXY_SERVER_CORE_NETWORK_H_