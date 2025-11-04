#ifndef PROXY_SERVER_SERVER_CONNECITON_H_
#define PROXY_SERVER_SERVER_CONNECTION_H_

#include "core/network.h"

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
        std::vector<std::byte> recieve_buffer;
        bool closed{false};
    };

    using ConnectionMap = std::unordered_map<core::SocketIdentifier, Connection>;

    // Accept as many queued connections as possible
    // Set them to non-blocking
    // Register for read events, and add them to a map of connections
    void AcceptNewConnections(core::SocketIdentifier listen_socket_id,
                              core::EventPollerIdentifier poller,
                              ConnectionMap& clients);
    
    // Read avail bytes from the socket into its connection struct buffer
    // Closes & removes the connection from clients on peer close or fatal error
    void OnClientRead(core::SocketIdentifier id, ConnectionMap& clients);

    // Closes a specific client and removes them from the map
    void CloseAndRemove(core::SocketIdentifier id, ConnectionMap& clients);

} // namespace server

#endif // PROXY_SERVER_SERVER_CONNECTION_H_