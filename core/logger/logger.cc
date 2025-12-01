#include "logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace core::logger {

Logger::Logger(std::string_view filename)
    : file(std::string(filename), std::ios::app) {}

void Logger::write(std::string_view level, std::string_view msg) {
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

    std::cout << "[" << ts.str() << "] [" << level << "] " << msg << std::endl;

}

void Logger::debug(std::string_view msg) { if(debug_logging) { write("DEBUG", msg); } }
void Logger::info(std::string_view msg) { if(verbose_logging) { write("INFO", msg); } }
void Logger::warning(std::string_view msg) { write("WARNING", msg); }
void Logger::error(std::string_view msg) { write("ERROR", msg); }
void Logger::critical(std::string_view msg) { write("CRITICAL", msg); }

}
