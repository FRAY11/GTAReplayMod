#pragma once

#include "plugin.h"
#include <windows.h>

// Main plugin class - this is the entry point for our mod
class GTAReplayMod {
public:
    static void Initialize();
    static void Shutdown();
    
    // Called every game frame
    static void OnGameProcess();
    
    // Keyboard input handler
    static void OnKeyPress(int key);
    
private:
    static bool s_IsRecording;
    static bool s_IsPlaying;
    static bool s_IsInitialized;
};
