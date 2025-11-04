#include "connection.h"

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>



namespace server {

    constexpr std::size_t k_8KB = 8 * 1024;
    constexpr std::size_t k_16KB = 16 * 1024;

    ConnectionResult AcceptNewConnections(core::SocketIdentifier socket, core::EventPollerIdentifier poller, ConnectionMap& clients) {
        
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
            c.receive_buffer.reserve(k_8KB);
            clients.emplace(client_socket, std::move(c));

            // TODO: Log that the client is accepted and registered with epoll
        }
    }
    

    ConnectionResult OnClientRead(core::SocketIdentifier socket, ConnectionMap& clients) {
        auto it = clients.find(socket);
        if (it == clients.end()) {
            // TODO: Log that client was not found
            // It is likely the client was already closed
            core::CloseSocket(socket);
            return ConnectionResult::UnkownError;
        }

        Connection& connection = it->second;
        std::byte buffer[k_16KB];

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