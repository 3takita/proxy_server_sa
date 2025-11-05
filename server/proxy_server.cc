#include "proxy_server.h"

#include <cstring>

namespace server {

    void ProxyServer::Run() {
        
        (void)core::InitializeNetwork();

        socket_ = core::CreateListeningSocket("", port_, backlog_);

        if (socket_ < 0) {
            // TODO: Log failed to create a listening socket on port_
            return; // We can't do anything without a listening socket
        }

        (void)core::SetSocketNonBlocking(socket_); // This can fail but we will handle the logging and consequences later

        poller_ = core::CreateEventPoller();
        if (poller_ < 0) {
            // TODO: Log failed to create an epoll instance
            CleanUpResources();
            return;
        }

        if (!core::RegisterReadEvent(poller_, socket_)) {
            // TODO: Log failed to register socket with epoll
            CleanUpResources();
            return;
        }

        core::PollEvent events[static_cast<std::size_t>(max_events_)];

        bool running = true;
        while (running) {
            int n = core::WaitForEvents(poller_, events, max_events_, -1); 
            if (n < 0) {
                // TODO: Log epoll_wait error
                // This is super rare but if it happens we may want to re-create epoll
                // For now just exit
                running = false;
                break;
            } else if (n == 0) {
                // Not expected with timeout = -1
                continue;
            }

            for (int i = 0; i < n; i++) {
                const auto fd = events[i].fd;
                const auto mask = events[i].mask;

                // ================================
                // Normal Proxy Server Flow Overview
                // ================================
                // 1. If this is the listening socket: accept new clients and register them for READ.
                // 2. For a client socket:
                //    - On READABLE: read and accumulate bytes into receive_buffer until EAGAIN.
                //      When we decide we have a full request (e.g., detected "\r\n\r\n"):
                //        a) Either fill send_buffer from cache or from an upstream fetch,
                //        b) Or generate a response locally (what we do here),
                //        c) Then flip the poller interest to WRITABLE so we can flush it.
                //    - On WRITABLE: write as many bytes as the kernel accepts from send_buffer.
                //      If we fully flush:
                //        a) Close (simple path now), or
                //        b) Switch back to READ interest for HTTP keep-alive (future).


                // ----------------------------
                // Event Error Handling
                // ----------------------------
                if (mask & (core::kEventError | core::kEventHangup | core::kEventOther)) {
                    if (fd == socket_) {
                        // TODO: Log fatal listening socket error/hangup
                        running = false;
                        break;
                    }
                    CloseAndRemove(fd, clients_);
                    continue;
                }

                // ----------------------------
                // 1. Handle New Connections
                // ----------------------------
                if (fd == socket_) {
                    (void)AcceptNewConnections(socket_, poller_, clients_);
                    continue;
                }

                // ----------------------------
                // 2. READABLE: read and stage response when ready
                // ----------------------------
                if (mask & core::kEventReadable) {
                    ConnectionResult read_result = OnClientRead(fd, clients_);
                    if (read_result != ConnectionResult::OK) {
                        // OnClientRead handles socket close
                        continue; 
                    }

                    auto it = clients_.find(fd);
                    if (it == clients_.end()) {
                        // Connection may have been closed or removed
                        continue; 
                    }

                    Connection& connection = it->second;

                    // TODO:
                    // Parse http or just send through if not http
                    // Check cache for requested resource

                    // TODO: Remove later when functionality is implemented
                    // -----------------------------------------------------
                    static const char kResponse[] =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 30\r\n"
                        "\r\n"
                        "Hello from your proxy server!\n";


                    const std::size_t kLen = sizeof(kResponse) - 1;
                    const std::size_t old_size = connection.send_buffer.size();
                    connection.send_buffer.resize(old_size + kLen);
                    std::memcpy(connection.send_buffer.data() + old_size, kResponse, kLen);
                    connection.want_write = true;

                    // TODO:
                    // We can add some sort of rate limiting here later on
                    // something where if the buffer has more data than a set amount
                    // we drop all incoming data from this socket (read = false)

                    (void)core::UpdateEventInterest(
                        poller_,
                        fd,
                        /*readable=*/true, // Replace with some rate limiting check
                        /*writable=*/true
                    );

                }

                // ----------------------------
                // 3. WRITABLE: drain send_buffer
                // ----------------------------
                if (mask & core::kEventWritable) {
                    auto it = clients_.find(fd);
                    if (it == clients_.end()) {
                        // Socket could have been closed / erased elsewhere
                        continue;
                    }
                    Connection& connection = it->second;

                    // Attempt to drain as much as possible durring this event from the send_buffer
                    (void)OnClientWrite(fd, clients_);

                    if (!connection.send_buffer.empty()) {
                        // We still have pending data to send so keep this socket WRITABLE
                        connection.want_write = true;
                    } else {
                        // All data is sent
                        connection.want_write = false;
                    }

                    (void)core::UpdateEventInterest(
                        poller_,
                        fd,
                        /*readable=*/true, // Add some sort of rate limiting later
                        /*writable=*/connection.want_write
                    );
                 
                }
            }
        }

        CleanUpResources();
    }


    void ProxyServer::CleanUpResources() {

        // Close all client sockets
        for (auto it = clients_.begin(); it != clients_.end(); ) {
            const core::SocketIdentifier socket = it->first;
            core::CloseSocket(socket);
            it = clients_.erase(it); //Returns the next it
        }

        // Close poller
        if (poller_ > 0) {
            core::CloseSocket(poller_);
            poller_ = 0;
        }

        // Close listening socket
        if (socket_ > 0) {
            core::CloseSocket(socket_);
            socket_ = 0;
        }

        core::ShutDownNetwork();
    }


} //namespace server