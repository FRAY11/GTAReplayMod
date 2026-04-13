#include "Logger.h"
#include <ctime>
#include <iomanip>
#include <sstream>

std::ofstream Logger::s_LogFile;
bool Logger::s_IsInitialized = false;

void Logger::Initialize(const std::string& filename) {
    if (s_IsInitialized) return;
    
    // Open log file in the game directory
    s_LogFile.open(filename, std::ios::out | std::ios::trunc);
    s_IsInitialized = true;
    
    if (s_LogFile.is_open()) {
        Log("=== GTA Replay Mod Log Started ===");
    }
}

void Logger::Log(const std::string& message) {
    if (!s_IsInitialized || !s_LogFile.is_open()) return;
    
    // Get current timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "[%H:%M:%S] ");
    oss << message;
    
    s_LogFile << oss.str() << std::endl;
    s_LogFile.flush(); // Write immediately so we can see logs even if game crashes
}

void Logger::Shutdown() {
    if (s_LogFile.is_open()) {
        Log("=== Log Ended ===");
        s_LogFile.close();
    }
    s_IsInitialized = false;
}
