#ifndef PROXY_SERVER_CORE_CONFIG_H_
#define PROXY_SERVER_CORE_CONFIG_H_

#include <string>

namespace server {

    struct Config {
        int port = 8080;
        std::string bind_host;   
        int backlog = 128;
        int max_events = 16;
        bool verbose = false;
        std::string log_file_name = "proxy_server.log";
           
    };

    Config ParseConfig(int argc, char* argv[]);

}  // namespace server

#endif  // PROXY_SERVER_CORE_CONFIG_H_

