#ifndef PROXY_SERVER_CORE_CONNECTION_H_
#define PROXY_SERVER_CORE_CONNECTION_H_

#include "network.h"

#include <unordered_map>
#include <vector>
#include <cstddef>


namespace core {

    enum ConnectionResult {
        OK = 0,
        AcceptFailed,
        EpollRegisterFailed,
        ReceiveFailed,
        PeerClosed,
        UnknownError
    };

    struct Connection {
        core::SocketIdentifier id{};
        std::vector<std::byte> receive_buffer;
        std::vector<std::byte> send_buffer;
        bool want_write{false};
        bool closed{false};
    };

    using ConnectionMap = std::unordered_map<core::SocketIdentifier, Connection>;

    // Accept as many queued connections as possible
    // Set them to non-blocking
    // Register for read events, and add them to a map of connections
    ConnectionResult AcceptNewConnections(core::SocketIdentifier socket, core::EventPollerIdentifier poller, 
                                            ConnectionMap& clients, std::vector<core::SocketIdentifier>* accepted_out);
    
    // Read available bytes from the socket into its connection struct buffer
    // Closes & removes the connection from clients on peer close or fatal error
    ConnectionResult OnClientRead(core::SocketIdentifier socket, ConnectionMap& clients);

    // Attempt to write as many bytes as possible from the send_buffer to the socket
    // Does not close the socket. The caller decides whether to keep-alive or close the connection
    ConnectionResult OnClientWrite(core::SocketIdentifier socket, ConnectionMap& clients);

    // Closes a specific client and removes them from the map
    void CloseAndRemove(core::SocketIdentifier socket, ConnectionMap& clients);

} // namespace core

#endif // PROXY_SERVER_SERVER_CONNECTION_H_