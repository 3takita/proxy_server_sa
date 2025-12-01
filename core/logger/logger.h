#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <fstream>
#include <string_view>
#include <mutex>

namespace core::logger {

class Logger {
public:

    explicit Logger(std::string_view filename);

    void debug(std::string_view msg);
    void info(std::string_view msg);
    void warning(std::string_view msg);
    void error(std::string_view msg);
    void critical(std::string_view msg);

private:
    std::ofstream file;
    std::mutex lock;

    bool verbose_logging{true};
    bool debug_logging{false};

    void write(std::string_view level, std::string_view msg);
};

}

#endif
