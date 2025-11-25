#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>
#include <mutex>

class Logger {
public:
    explicit Logger(const std::string& filename);

    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);
    void critical(const std::string& msg)

private:
    std::ofstream file;
    std::mutex lock;

    void write(const std::string& level, const std::string& msg);
};

#endif
