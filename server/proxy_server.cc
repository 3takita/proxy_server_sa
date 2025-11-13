#include "proxy_server.h"
#include "protocol/detector.h"
#include "protocol/protocol.h"
#include "protocol/socks5.h"
#include "utils/string_utils.h"

#include <cstring>
#include <iostream> // Remove after Logger exists
#include <vector>

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

        std::vector<core::PollEvent> events(static_cast<std::size_t>(max_events_));

        bool running = true;
        while (running) {
            int n = core::WaitForEvents(poller_, events.data(), max_events_, -1);
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
                //      When we decide we have a full request:
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
                    CloseAndRemove(fd, connections_);
                    continue;
                }

                // ----------------------------
                // 1. Handle New Connections
                // ----------------------------
                if (fd == socket_) {
                    std::vector<core::SocketIdentifier> accepted;
                    (void)AcceptNewClientConnection(socket_, poller_, connections_, &accepted);

                    // TODO: Remove the accepted socket vector once the logger is in place
                    // Do not change the AcceptNewConnections signature nor the functionality
                    // of AcceptNewConnections other than adding in the Logger
                    // Remove the printout here but keep the vector
                    // We can revisit this when the Logger is in place

                    for (auto a : accepted) {
                        std::string ip;
                        uint16_t port = 0;
                        if (core::SocketToAddress(a, ip, port)) {
                            std::cout << "[+] New connection " << a
                                    << " from " << ip << ":" << port << std::endl;
                        } else {
                            std::cout << "[+] New connection " << a
                                    << " (address unavailable)" << std::endl;
                        }
                    }

                    continue;
                }

                // ----------------------------
                // 2. READABLE: read and stage response when ready
                // ----------------------------
                if (mask & core::kEventReadable) {
                    core::ConnectionResult read_result = OnClientRead(fd, connections_);
                    if (read_result != core::ConnectionResult::OK) {
                        // OnClientRead handles socket close
                        continue; 
                    }

                    auto it = connections_.find(fd);
                    if (it == connections_.end()) {
                        // Connection may have been closed or removed
                        continue; 
                    }

                    core::Connection& connection = it->second;

                    // Uncomment below to see user requests outputed to the console
                    std::cout << core::utils::bytesToReadableString(connection.receive_buffer_) << std::endl;

                    if (connection.role_ == core::ConnectionRole::Client) {

                        if (connection.protocol_ == nullptr) {
                            // Set the protocol for this connection
                            core::protocol::ProtocolType protocol;
                            if(core::protocol::ProbeProtocol(connection.receive_buffer_, protocol)) {
                                switch(protocol) {
                                    case core::protocol::ProtocolType::Socks4:
                                        std::cout << "Socks4" << std::endl; // Not supported yet
                                        CloseAndRemove(fd, connections_); 
                                        break;
                                    case core::protocol::ProtocolType::Socks4a:
                                        std::cout << "Socks4a" << std::endl; // Not supported yet
                                        CloseAndRemove(fd, connections_);
                                        break;
                                    case core::protocol::ProtocolType::Socks5:
                                        std::cout << "Socks5" << std::endl;
                                        connection.protocol_ = std::make_unique<core::protocol::Socks5>();
                                        break;
                                    case core::protocol::ProtocolType::Http:
                                        std::cout << "Http" << std::endl; // Not supported yet
                                        CloseAndRemove(fd, connections_);
                                        break;
                                    case core::protocol::ProtocolType::Unsupported:
                                        [[fallthrough]];
                                    case core::protocol::ProtocolType::Unknown:
                                        [[fallthrough]];
                                    default:
                                        std::cout << "Unknown Protocol Type" << std::endl; // Not supported at all
                                        HealthResponse(connection);
                                        //CloseAndRemove(fd, connections_);
                                        
                                        break;
                                }
                            } else {
                                // Not enough bytes
                                // TODO: Decide if we want to log something here
                            }
                        }

                        // If the connection has a protocol set, we can do work with it
                            if (connection.protocol_ != nullptr) {
                                connection.protocol_->OnReadable(connection, connections_, poller_);
                            }
                    }

                    (void)core::UpdateEventInterest(
                        poller_,
                        fd,
                        /*readable=*/true, // Replace with some rate limiting check
                        /*writable=*/connection.want_write_
                    );
                }

                // ----------------------------
                // 3. WRITABLE: drain send_buffer
                // ----------------------------
                if (mask & core::kEventWritable) {
                    auto it = connections_.find(fd);
                    if (it == connections_.end()) {
                        // Socket could have been closed / erased elsewhere
                        continue;
                    }
                    core::Connection& connection = it->second;
      
                    if (connection.protocol_ != nullptr) {
                        connection.protocol_->OnWritable(connection, connections_, poller_);
                    }

                    // Attempt to drain as much as possible durring this event from the send_buffer
                    (void)OnClientWrite(fd, connections_);

                    if (!connection.send_buffer_.empty()) {
                        // We still have pending data to send so keep this socket WRITABLE
                        connection.want_write_ = true;
                    } else {
                        // All data is sent
                        connection.want_write_ = false;
                    }

                    (void)core::UpdateEventInterest(
                        poller_,
                        fd,
                        /*readable=*/true, // Add some sort of rate limiting later
                        /*writable=*/connection.want_write_
                    );
                 
                }
            }
        }

        CleanUpResources();
    }


    void ProxyServer::CleanUpResources() {

        // Close all client sockets
        for (auto it = connections_.begin(); it != connections_.end(); ) {
            const core::SocketIdentifier socket = it->first;
            core::CloseSocket(socket);
            it = connections_.erase(it); //Returns the next it
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

    void ProxyServer::HealthResponse(core::Connection& connection) {
        static const char kResponse[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 30\r\n"
            "\r\n"
            "Hello from your proxy server!\n";


        const std::size_t kLen = sizeof(kResponse) - 1;
        const std::size_t old_size = connection.send_buffer_.size();
        connection.send_buffer_.resize(old_size + kLen);
        std::memcpy(connection.send_buffer_.data() + old_size, kResponse, kLen);
        connection.want_write_ = true;
    }


} //namespace server