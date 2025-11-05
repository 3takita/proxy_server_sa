#include "connection.h"
#include "network_constants.h"

#include <cerrno>
#include <cstring>

namespace server {

        ConnectionResult AcceptNewConnections(core::SocketIdentifier socket, core::EventPollerIdentifier poller, 
                                            ConnectionMap& clients, std::vector<core::SocketIdentifier>* accepted_out) {
        
        while (true) {
            core::SocketIdentifier client_socket = core::AcceptConnection(socket);

            if (client_socket < 0) {
                // TODO: Log info - no more queued connections, or accept() failed
                return ConnectionResult::OK; 
            }

            if (!core::SetSocketNonBlocking(client_socket)) {
                // TODO: Log that client socket could not be set non-blocking
                // Continuing could block the event loop so we have to close
                core::CloseSocket(client_socket);
                continue; // We do not need to fail the whole loop
            }

            if (!core::RegisterReadEvent(poller, client_socket)) {
                // epoll registration failed
                core::CloseSocket(client_socket);
                return ConnectionResult::EpollRegisterFailed;
            }

            Connection c;
            c.id = client_socket;
            c.receive_buffer.reserve(core::k_8KB);
            clients.emplace(client_socket, std::move(c));

            if (accepted_out != nullptr) accepted_out->push_back(client_socket);

            // TODO: Log that the client is accepted and registered with epoll
        }
    }
    

    ConnectionResult OnClientRead(core::SocketIdentifier socket, ConnectionMap& clients) {
        auto it = clients.find(socket);
        if (it == clients.end()) {
            // TODO: Log that client was not found
            // It is likely the client was already closed
            core::CloseSocket(socket);
            return ConnectionResult::UnknownError;
        }

        Connection& connection = it->second;
        std::byte buffer[core::k_16KB];

        while (true) {
            std::ptrdiff_t n = core::Receive(socket, buffer, sizeof(buffer));
            if (n > 0) {
                const std::size_t old = connection.receive_buffer.size();
                connection.receive_buffer.resize(old + static_cast<std::size_t>(n));
                std::memcpy(connection.receive_buffer.data() + old, buffer, static_cast<std::size_t>(n));
                continue; // drain until we would block
            }

            if (n == 0) {
                // TODO: Log peer closed connection gracefully
                connection.closed = true;
                break;
            }

            // n < 0
            // Receive will already log error
            break;
        }

        if (connection.closed) {
            core::CloseSocket(socket);
            clients.erase(it);
            // TODO: Log client closed gracefully
            return ConnectionResult::PeerClosed;
        }

        return ConnectionResult::OK;
    }

    ConnectionResult OnClientWrite(core::SocketIdentifier socket, ConnectionMap& clients) {
        auto it = clients.find(socket);
        if (it == clients.end()) {
            // TODO: Log that client was not found
            // It is likely the client was already closed
            core::CloseSocket(socket);
            return ConnectionResult::UnknownError;
        }

        Connection& connection = it->second;

        // Drain as many bytes as the kernel will accept
        // We are currently using a single contiguous vector
        // However we may want to try something different like a ring buffer in the future
        while (!connection.send_buffer.empty()) {
            const void* data = connection.send_buffer.data();
            const std::size_t data_length = connection.send_buffer.size();

            std::ptrdiff_t sent = core::Send(socket, data, data_length);
            if (sent > 0) {
                connection.send_buffer.erase(connection.send_buffer.begin(),
                                             connection.send_buffer.begin() + static_cast<std::size_t>(sent));
                continue; // Try sending more during this event
            }
            // sent <= 0: would-block or other error, can stop trying to send
            break;
        }
        return ConnectionResult::OK;
    }

    void CloseAndRemove(core::SocketIdentifier socket, ConnectionMap& clients) {
        auto it = clients.find(socket);
        if (it != clients.end()) {
            core::CloseSocket(socket);
            (void)clients.erase(it);
            // TODO: Log that client socket is closed
        } else {
            core::CloseSocket(socket);
            // TODO: Log that untracked client socket is closed
        }
    }

} // namespace server