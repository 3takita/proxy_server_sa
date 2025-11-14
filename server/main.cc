#include "proxy_server.h"
#include "config.h"

int main(int argc, char* argv[]) {
    // Parse command-line arguments into Config
    core::Config cfg = core::Config::FromArgs(argc, argv);

    // Initialize global config singleton
    core::InitializeConfig(cfg);

    // Run the proxy
    server::ProxyServer proxy;
    proxy.Run();

    return 0;
}

