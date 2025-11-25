#include "config.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace server {

    Config ParseConfig(int argc, char* argv[]) {
        Config cfg;

        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
                cfg.port = std::atoi(argv[++i]);
            } else if (std::strcmp(argv[i], "--backlog") == 0 && i + 1 < argc) {
                cfg.backlog = std::atoi(argv[++i]);
            } else if (std::strcmp(argv[i], "--max-events") == 0 && i + 1 < argc) {
                cfg.max_events = std::atoi(argv[++i]);
            } else if (std::strcmp(argv[i], "--bind-host") == 0 && i + 1 < argc) {
                cfg.bind_host = argv[++i];
            } else if (std::strcmp(argv[i], "--verbose") == 0) {
                cfg.verbose = true;
            } else if (std::strcmp(argv[i], "--help") == 0) {
                std::cout << "Usage: proxy_server [--port N] [--backlog N] [--max-events N] [--bind-host IP] [--verbose]\n";
                std::exit(0);
            } else {
                std::cerr << "Unknown option: " << argv[i] << "\n";
            }
        }
        return cfg;
    }

}  // namespace server

