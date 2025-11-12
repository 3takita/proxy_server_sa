#ifndef PROXY_SERVER_CORE_CONNECTION_H_
#define PROXY_SERVER_CORE_CONNECTION_H_

#include "network.h"
#include "protocol/protocol.h"

#include <unordered_map>
#include <vector>
#include <cstddef>
#include <memory>


namespace core {

    enum ConnectionResult : uint8_t {
        OK = 0,
        AcceptFailed,
        EpollRegisterFailed,
        ReceiveFailed,
        PeerClosed,
        UnknownError
    };

    // The connection to the client needs one socket and 
    // the connection out to the internet needs one socket.
    // Therefore, each connection will either be a Client or Upstream
    // and each connection will have a matching connection file descriptor
    enum ConnectionRole : uint8_t {
        Client = 0, // Socket connected to client
        Upstream = 1 // Socket connected upstream 
    };

    class Connection {
    public:

        core::SocketIdentifier id_{};

        ConnectionRole role_{};
        core::SocketIdentifier peer_socket_id_{-1}; // fd of the peer socket; -1 means unpaired

        std::vector<std::byte> receive_buffer_;
        std::vector<std::byte> send_buffer_;
        bool want_write_{false};
        bool closed_{false};

        std::unique_ptr<core::protocol::Protocol> protocol_ = nullptr;
    };

    using ConnectionMap = std::unordered_map<core::SocketIdentifier, Connection>;

    // Accept as many queued connections as possible
    // Set them to non-blocking
    // Register for read events, and add them to a map of connections
    ConnectionResult AcceptNewClientConnection(core::SocketIdentifier socket, core::EventPollerIdentifier poller, 
                                            ConnectionMap& connections, std::vector<core::SocketIdentifier>* accepted_out);
    
    // Read available bytes from the socket into its connection struct buffer
    // Closes & removes the connection from connections on peer close or fatal error
    ConnectionResult OnClientRead(core::SocketIdentifier socket, ConnectionMap& connections);

    // Attempt to write as many bytes as possible from the send_buffer to the socket
    // Does not close the socket. The caller decides whether to keep-alive or close the connection
    ConnectionResult OnClientWrite(core::SocketIdentifier socket, ConnectionMap& connections);

    // Closes a specific client and removes them from the map
    void CloseAndRemove(core::SocketIdentifier socket, ConnectionMap& connections);

} // namespace core

#endif // PROXY_SERVER_CORE_CONNECTION_H_