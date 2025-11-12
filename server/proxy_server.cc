#include "proxy_server.h"
#include "protocol/detector.h"
#include "protocol/protocol.h"
#include "protocol/socks5.h"
#include "utils/string_utils.h"

#include <cstring>
#include <iostream> // Remove after Logger exists

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

                // ----------------------------
                // Event Error Handling
                // ----------------------------
                if (mask & (core::kEventError | core::kEventHangup | core::kEventOther)) {
                    if (fd == socket_) {
                        // TODO: Log fatal listening socket error/hangup
                        running = false;
                        break;
                    }
                    
                    auto it = connections_.find(fd);
                    if (it != connections_.end()) {
                        connections_.erase(it);
                    } else {
                        core::CloseSocket(fd);
                    }
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

                //Get the connection 
                auto it = connections_.find(fd);
                if (it == connections_.end()) continue;
                core::Connection& connection = it->second;

                // ----------------------------
                // 2. READABLE: read and stage response when ready
                // ----------------------------
                if (mask & core::kEventReadable) {
                    if (connection.Read() != core::ConnectionResult::OK) {
                        // The read failed
                        if (connection.closed_) {
                            connections_.erase(it);
                        }
                        continue;
                    }

                    // Uncomment below to see user requests outputed to the console
                    std::cout << core::utils::bytesToReadableString(connection.receive_buffer_) << std::endl;

                    if (connection.role_ == core::ConnectionRole::Client) {

                        if (!connection.protocol_) {
                            (void)connection.SetProtocol();
                        }

                        // If the connection has a protocol set, we can do work with it
                            if (connection.protocol_) {
                                connection.OnReadable(poller_);
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

      
                    if (connection.protocol_) {
                        connection.OnWritable(poller_);
                    }

                    // Attempt to drain as much as possible durring this event from the send_buffer
                    (void)connection.Write();

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

        // Close all connections
        for (auto it = connections_.begin(); it != connections_.end(); ) {
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