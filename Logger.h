#pragma once

#include <string>
#include <fstream>

// Simple debug logging system - writes messages to a text file
class Logger {
public:
    static void Initialize(const std::string& filename);
    static void Log(const std::string& message);
    static void Shutdown();
    
private:
    static std::ofstream s_LogFile;
    static bool s_IsInitialized;
};
