#ifndef PROXY_SERVER_CORE_CONNECTION_H_
#define PROXY_SERVER_CORE_CONNECTION_H_

#include "network.h"
#include "protocol/protocol.h"

#include <vector>
#include <cstddef>
#include <memory>
#include <unordered_map>


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

        // Constructors & Destructors
        Connection() noexcept;
        Connection(core::SocketIdentifier id, ConnectionRole role) noexcept;
        ~Connection();

        // Copy Semantics --> Delete
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        // Move Semantics
        Connection(Connection&& other) noexcept;
        Connection& operator=(Connection&& other) noexcept;

        // Read available bytes from the socket into its connection struct buffer
        // Does not close the socket
        ConnectionResult Read();

        // Attempt to write as many bytes as possible from the send_buffer to the socket
        // Does not close the socket
        ConnectionResult Write();

        // Closes a socket
        void Close();

        // Determines the protocol and sets it if not already set
        // Returns true if we know what the protocol is, false otherwise
        bool SetProtocol();

        core::SocketIdentifier id_;

        ConnectionRole role_;
        core::SocketIdentifier peer_socket_id_; // fd of the peer socket; -1 means unpaired

        std::vector<std::byte> receive_buffer_;
        std::vector<std::byte> send_buffer_;
        bool want_write_;
        bool closed_;

        std::unique_ptr<core::protocol::Protocol> protocol_;
    };

    using ConnectionMap = std::unordered_map<core::SocketIdentifier, Connection> ;

    // Accept as many queued connections as possible
    // Set them to non-blocking
    // Register for read events, and add them to a map of connections
    ConnectionResult AcceptNewClientConnection(core::SocketIdentifier socket, core::EventPollerIdentifier poller, 
                                               ConnectionMap& connections, 
                                               std::vector<core::SocketIdentifier>* accepted_out);

    // Creates an upstream (server --> internet) connection for a client.
    // dest_ip is 4 bytes of IPv4 address in network byte order,
    // dest_port is the port in network byte order
    ConnectionResult CreateUpstreamTCPConnection(core::EventPollerIdentifier poller,
                                                    ConnectionMap& connections,
                                                    Connection& client,
                                                    const std::byte dest_ip[4],
                                                    uint16_t dest_port);
    

} // namespace core

#endif // PROXY_SERVER_CORE_CONNECTION_H_