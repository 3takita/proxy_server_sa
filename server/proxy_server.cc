#include "proxy_server.h"

#include "protocol/detector.h"
#include "protocol/protocol.h"
#include "protocol/socks5.h"
#include "protocol/socks4.h"
#include "utils/string_utils.h"

#include <cstring>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace server {

void ProxyServer::Run() 
{
    (void)core::InitializeNetwork();

    // -----------------------------
    // Use config from config.h/.cc
    // -----------------------------
    const auto& cfg = core::GetConfig();

    socket_ = core::CreateListeningSocket(cfg.bind_host, cfg.port, cfg.backlog);

    if (socket_ < 0) {
        // TODO: add logger
        return;
    }

    if (!core::SetSocketNonBlocking(socket_)) {
        CleanUpResources();
        return;
    }

    poller_ = core::CreateEventPoller();
    if (poller_ < 0) {
        CleanUpResources();
        return;
    }

    if (!core::RegisterReadEvent(poller_, socket_)) {
        CleanUpResources();
        return;
    }

    core::PollEvent events[static_cast<std::size_t>(cfg.max_events)];
    bool running = true;

    while (running) {

        int n = core::WaitForEvents(
            poller_,
            events,
            cfg.max_events,
            -1
        );

        if (n < 0) {
            // Rare epoll_wait failure
            running = false;
            break;
        }

        for (int i = 0; i < n; i++) {

            const auto fd = events[i].fd;
            const auto mask = events[i].mask;

            // ----------------------------------
            // Error or hangup event
            // ----------------------------------
            if (mask & (core::kEventError | core::kEventHangup | core::kEventOther)) {
                if (fd == socket_) {
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

            // ----------------------------------
            // 1. New incoming client connection
            // ----------------------------------
            if (fd == socket_) {

                std::vector<core::SocketIdentifier> accepted;
                (void)AcceptNewClientConnection(socket_, poller_, connections_, &accepted);

                // Logging (temporary)
                for (auto a : accepted) {
                    std::string ip;
                    uint16_t port = 0;

                    auto now = std::chrono::system_clock::now();
                    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                    std::tm tm_buf{};
                #ifdef _WIN32
                    localtime_s(&tm_buf, &now_c);
                #else
                    localtime_r(&now_c, &now_c);
                #endif
                    std::ostringstream timestamp;
                    timestamp << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

                    if (core::SocketToAddress(a, ip, port)) {
                        std::cout << "[" << timestamp.str() 
                                  << "] [+] New connection " << a
                                  << " from " << ip << ":" << port 
                                  << std::endl;
                    }
                }
                continue;
            }

            // -----------------------------
            // Retrieve connection
            // -----------------------------
            auto it = connections_.find(fd);
            if (it == connections_.end()) continue;

            core::Connection* connection = &it->second;

            core::Connection* peer_connection = nullptr;
            if (connection->peer_socket_id_ != -1) {
                auto pit = connections_.find(connection->peer_socket_id_);
                if (pit != connections_.end()) {
                    peer_connection = &pit->second;
                } else {
                    connection->peer_socket_id_ = -1;
                }
            }

            // ----------------------------------
            // 2. READABLE
            // ----------------------------------
            if (mask & core::kEventReadable) {

                if (connection->Read() != core::ConnectionResult::OK) {
                    if (connection->closed_) {
                        connections_.erase(it);
                    }
                    continue;
                }

                if (connection->role_ == core::ConnectionRole::Client) {   
                    if (connection->protocol_) {
                        connection->protocol_->OnReadable(
                            connection, 
                            peer_connection, 
                            poller_
                    );

                    // If SOCKS4 established, create upstream connection
                    if (connection->protocol_->type() 
                                == core::protocol::ProtocolType::kSocks4) {
                            
                            auto* socks4 = dynamic_cast<core::protocol::Socks4*>(
                                connection->protocol_.get());

                            if (socks4 &&
                                connection->peer_socket_id_ == -1 &&
                                socks4->state() == core::protocol::Socks4::State::Established) 
                            {
                                core::CreateUpstreamTCPConnection(
                                    poller_,
                                    connections_,
                                    *connection,
                                    socks4->destination_ip(),
                                    socks4->destination_port()
                                );
                            }
                        }
                    }

                } else if (connection->role_ == core::ConnectionRole::Upstream) {

                    if (peer_connection && peer_connection->protocol_) {
                        peer_connection->protocol_->OnReadable(
                            connection,
                            peer_connection,
                            poller_
                        );
                    }
                }

                (void)core::UpdateEventInterest(
                    poller_,
                    fd,
                    true,
                    connection->want_write_
                );

                if (peer_connection) {
                    (void)core::UpdateEventInterest(
                        poller_,
                        peer_connection->id_,
                        true,
                        peer_connection->want_write_
                    );
                }
            }

            // ----------------------------------
            // 3. WRITABLE
            // ----------------------------------
            if (mask & core::kEventWritable) {

                if (connection->protocol_) {
                    connection->protocol_->OnWritable(
                        connection,
                        peer_connection,
                        poller_
                    );
                }

                (void)connection->Write();

                (void)core::UpdateEventInterest(
                    poller_,
                    fd,
                    true,
                    connection->want_write_
                );
            }

        } // for(n events)
    } // while(running)

    CleanUpResources();
}

void ProxyServer::CleanUpResources() 
{
    // Remove all connections
    for (auto it = connections_.begin(); it != connections_.end();) {
        it = connections_.erase(it);
    }

    if (poller_ > 0) {
        core::CloseSocket(poller_);
        poller_ = 0;
    }

    if (socket_ > 0) {
        core::CloseSocket(socket_);
        socket_ = 0;
    }

    core::ShutDownNetwork();
}

void ProxyServer::HealthResponse(core::Connection& connection) 
{
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

} // namespace server

