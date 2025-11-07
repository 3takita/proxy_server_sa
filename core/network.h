#ifndef PROXY_SERVER_CORE_NETWORK_H_
#define PROXY_SERVER_CORE_NETWORK_H_

#include <string>
#include <cstdint>
#include <cstddef>

namespace core {

    // File Descriptor in Linux
    // SOCKET in windows
    using SocketIdentifier = int;
    using EventPollerIdentifier = int;

    // Event mask that maps to OS-specific bits
    inline constexpr uint32_t kEventReadable = 0x01; // Data available to read
    inline constexpr uint32_t kEventWritable = 0x02; // Data available to write
    inline constexpr uint32_t kEventError    = 0x04; // Error condition
    inline constexpr uint32_t kEventHangup   = 0x08; // Hangup/Peer closed
    inline constexpr uint32_t kEventOther    = 0xF0; // Temp for anything not above. Replace this when adding future event flags

    struct PollEvent {
        SocketIdentifier fd{};
        uint32_t mask{}; // bitwise OR of kEvent* flags
    };

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

    // Changes the socket to readable or writable
    // Returns true on success
    bool UpdateEventInterest(EventPollerIdentifier poller, SocketIdentifier socket, bool readable, bool writable);

    // Waits for socket events
    // Returns the number of triggered events
    // The parameter `events` and `max_events` determine output capacity
    int WaitForEvents(EventPollerIdentifier poller, core::PollEvent* events, int max_events, int timeout_ms);

    // Accepts a queued connection from a listening socket
    // Returns client socket or -1 on error
    SocketIdentifier AcceptConnection(SocketIdentifier listening_socket);

    // Receive up to `length` bytes into a buffer. Returns: 
    // > 0 : bytes read
    //   0 : peer closed
    // < 0 : error
    std::ptrdiff_t Receive(SocketIdentifier socket, void* buffer, std::size_t length);
    
    // Send up to `length` bytes from buffer
    // > 0 : bytes written
    // <=0 : error or would-block on non-blocking socket
    std::ptrdiff_t Send(SocketIdentifier socket, const void* buffer, std::size_t length);

    // Closes a socket safely
    void CloseSocket(SocketIdentifier socket);

    // Returns true and fills ip and port on success
    bool SocketToAddress(SocketIdentifier socket, std::string& ip, uint16_t& port);

}   // namespace core

#endif // PROXY_SERVER_CORE_NETWORK_H_