#include "proxy_server.h"

#include "../core/network.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace server {

    void ProxyServer::Run() {
        
        (void)core::InitializeNetwork();

        socket_ = core::CreateListeningSocket("", port_, backlog_);

        if (socket_ < 0) {
            // TODO: Log failed to create a listening socket on port_
            return; // We can't do anything without a listening socket
        }

        (void)core::SetNonBlocking(socket_); // This can fail but we will handle the logging and consequences later

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

        epoll_event events[max_events_];

        while (true) {
            int n = core::WaitForEvents(poller_, events, max_events_, -1); 
            if (n < 0) {
                // TODO: Log epoll_wait error
                // This is super rare but if it happens we may want to re-create epoll
                // For now just exit
                break;
            } else if (n == 0) {
                // Not expected with timout = -1
                // Keep for future use but we will probably
                // never need this branch
            }

            for (int i = 0; i < n; ++i) {
                if (events[i].data.fd == socket_) {
                    // TODO: Accept connection and handle data
                }
            }

        }

        CleanUpResources();
    }


    void ProxyServer::CleanUpResources() {
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


} //namespace server