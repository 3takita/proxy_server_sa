#ifndef PROXY_SERVER_SERVER_PROXY_SERVER_H_
#define PROXY_SERVER_SERVER_PROXY_SERVER_H_

#include "network.h"
#include "connection.h"

// I know the header define looks long and weird
// but it follows the format
// Project_Name_Namespace_File_Name

namespace server {

    class ProxyServer final {
    public:
        void Run();

    private:
    // TODO: Let the user specify these later
    // with command line arguments
        int port_{ 8080 };
        int backlog_{ 128 };
        int max_events_{ 16 };
        core::SocketIdentifier socket_{};
        core::EventPollerIdentifier poller_{};
        core::ConnectionMap connections_;

        void CleanUpResources();
        void HealthResponse(core::Connection& connection);
    };
} // namespace server

#endif // PROXY_SERVER_SERVER_PROXY_SERVER_H_