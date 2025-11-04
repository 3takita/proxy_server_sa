#include "proxy_server.h"
#include "network.h"

#include <iostream> // Potentially remove later

#include <sys/socket.h> // Remove later
#include <netdb.h> // Remove later
#include <string_view> // Remove later


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
                // Not expected with timout = -1
                // Keep for future use but we will probably
                // never need this branch
            }

            for (int i = 0; i < n; i++) {
                const auto fd = events[i].fd;
                const auto mask = events[i].mask;

                if (mask & (core::kEventError | core::kEventHangup | core::kEventOther)) {
                    if (fd == socket_) {
                        // TODO: Log fatal listening socket error/hangup
                        running = false;
                        break;
                    }
                    CloseAndRemove(fd, clients_);
                    continue;
                }

                if (fd == socket_) {
                    (void)AcceptNewConnections(socket_, poller_, clients_);
                    continue;
                }

                if (mask & core::kEventReadable) {
                    (void)OnClientRead(fd, clients_);
                    // TODO: Handle incoming data for this client
                    //----------------------------------------------
                    //Remove
                    const char response[] =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "Hello from your proxy server!\n";

                    ::send(fd, response, sizeof(response) - 1, 0);
                    core::CloseSocket(fd);
                    clients_.erase(fd);
                    //----------------------------------------------
                }

                if (mask & core::kEventWritable) {
                    // TODO: Handle outgoing data for this client
                    //----------------------------------------------
                    //Remove
                    
                }
                    //----------------------------------------------
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