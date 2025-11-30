#include "proxy_server.h"
#include "config.h"

int main(int argc, char* argv[]) {
    // Parse command-line arguments into Config
    server::Config config = server::ParseConfig(argc, argv);

    // Run the proxy
    server::ProxyServer proxy{config};
    proxy.Run();

    return 0;
}

