#include "logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

Logger::Logger(const std::string& filename) {
    file.open(filename, std::ios::app);
}

void Logger::write(const std::string& level, const std::string& msg) {
    std::lock_guard<std::mutex> guard(lock);

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::stringstream ts;
    ts << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (file.is_open()) {
        file << "[" << ts.str() << "] [" << level << "] " << msg << "\n";
        file.flush();
    }
}

void Logger::info(const std::string& msg) {
    write("INFO", msg);
}

void Logger::warning(const std::string& msg) {
    write("WARNING", msg);
}

void Logger::error(const std::string& msg) {
    write("ERROR", msg);
}
void Logger::critical(const std::string& msg) {
    write("CRITICAL", msg);
}
