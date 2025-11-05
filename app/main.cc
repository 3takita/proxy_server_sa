#include "proxy_server.h"

int main(int argc, char* argv[]) {

    (void)argc;
    (void)argv;

    server::ProxyServer proxy;
    proxy.Run();
 
    return 0;
}
