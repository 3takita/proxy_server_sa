#ifndef PROXY_SERVER_SERVER_CONNECITON_H_
#define PROXY_SERVER_SERVER_CONNECTION_H_

#include "network.h"

#include <unordered_map>
#include <vector>
#include <cstddef>


namespace server {

    enum ConnectionResult {
        OK = 0,
        AcceptFailed,
        EpollRegisterFailed,
        RecieveFailed,
        PeerClosed,
        UnkownError
    };

    struct Connection {
        core::SocketIdentifier id{};
        std::vector<std::byte> receive_buffer;
        bool closed{false};
    };

    using ConnectionMap = std::unordered_map<core::SocketIdentifier, Connection>;

    // Accept as many queued connections as possible
    // Set them to non-blocking
    // Register for read events, and add them to a map of connections
    ConnectionResult AcceptNewConnections(core::SocketIdentifier socket, core::EventPollerIdentifier poller, ConnectionMap& clients);
    
    // Read avail bytes from the socket into its connection struct buffer
    // Closes & removes the connection from clients on peer close or fatal error
    ConnectionResult OnClientRead(core::SocketIdentifier socket, ConnectionMap& clients);

    // Closes a specific client and removes them from the map
    void CloseAndRemove(core::SocketIdentifier socket, ConnectionMap& clients);

} // namespace server

#endif // PROXY_SERVER_SERVER_CONNECTION_H_