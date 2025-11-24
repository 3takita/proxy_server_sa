#ifndef PROXY_SERVER_SERVER_PROXY_SERVER_H_
#define PROXY_SERVER_SERVER_PROXY_SERVER_H_

#include "network.h"
#include "connection.h"
#include "config.h"

namespace server {

class ProxyServer final {
public:
    void Run();

private:
    core::SocketIdentifier socket_{};
    core::EventPollerIdentifier poller_{};
    core::ConnectionMap connections_;
    core::Config config_{};

    void CleanUpResources();
    void HealthResponse(core::Connection& connection);
};

} // namespace server

#endif // PROXY_SERVER_SERVER_PROXY_SERVER_H_

