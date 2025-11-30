#ifndef PROXY_SERVER_SERVER_PROXY_SERVER_H_
#define PROXY_SERVER_SERVER_PROXY_SERVER_H_

#include "network.h"
#include "connection.h"
#include "dns.h"
#include "config.h"
#include "logger/logger.h"

#include <memory>
#include <thread>

// I know the header define looks long and weird
// but it follows the format
// Project_Name_Namespace_File_Name

namespace server {

class ProxyServer final {
public:
    explicit ProxyServer(const Config& config);

    void Run();

private:
    core::SocketIdentifier socket_;
    core::EventPollerIdentifier poller_;
    core::ConnectionMap connections_;
    server::Config config_;
    core::logger::Logger logger_;

    void CleanUpResources();
    void HealthResponse(core::Connection& connection);
};

} // namespace server

#endif // PROXY_SERVER_SERVER_PROXY_SERVER_H_

