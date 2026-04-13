#include "Plugin.h"
#include "DataLogger.h"
#include "PlaybackEngine.h"
#include "Logger.h"
#include <CTimer.h>
#include <extensions/KeyCheck.h>

// Static member initialization
bool GTAReplayMod::s_IsRecording = false;
bool GTAReplayMod::s_IsPlaying = false;
bool GTAReplayMod::s_IsInitialized = false;

void GTAReplayMod::Initialize() {
    if (s_IsInitialized) return;
    
    // Initialize our debug logger
    Logger::Initialize("GTAReplayMod.log");
    Logger::Log("GTA Replay Mod - Phase 1 & 2: Recording and Playback");
    Logger::Log("Press F9 to start/stop recording");
    Logger::Log("Press F10 to start/stop playback");
    
    // Initialize the data logger and playback engine
    DataLogger::Initialize();
    PlaybackEngine::Initialize();
    
    s_IsInitialized = true;
    Logger::Log("Initialization complete!");
}

void GTAReplayMod::Shutdown() {
    if (s_IsRecording) {
        DataLogger::StopRecording();
        s_IsRecording = false;
    }
    
    if (s_IsPlaying) {
        PlaybackEngine::StopPlayback();
        s_IsPlaying = false;
    }
    
    DataLogger::Shutdown();
    PlaybackEngine::Shutdown();
    Logger::Log("Mod shutdown complete");
    Logger::Shutdown();
}

void GTAReplayMod::OnGameProcess() {
    // This runs every single frame of the game
    if (!s_IsInitialized) return;
    
    // Check for F9 key press to toggle recording
    if (KeyPressed(VK_F9)) {
        // Can't record while playing
        if (s_IsPlaying) {
            Logger::Log("Cannot record while playback is active!");
        } else {
            s_IsRecording = !s_IsRecording;
            
            if (s_IsRecording) {
                DataLogger::StartRecording();
                Logger::Log("Recording STARTED - capturing game state...");
            } else {
                DataLogger::StopRecording();
                Logger::Log("Recording STOPPED - replay saved!");
            }
        }
    }
    
    // Check for F10 key press to toggle playback
    if (KeyPressed(VK_F10)) {
        // Can't play while recording
        if (s_IsRecording) {
            Logger::Log("Cannot playback while recording!");
        } else {
            s_IsPlaying = !s_IsPlaying;
            
            if (s_IsPlaying) {
                // Load the most recent replay file
                // For now, hardcode a test file - we'll improve this later
                if (PlaybackEngine::LoadReplayFile("test_replay.gtareplay")) {
                    PlaybackEngine::StartPlayback();
                    Logger::Log("Playback STARTED - replaying recorded session...");
                } else {
                    s_IsPlaying = false;
                    Logger::Log("ERROR: Failed to load replay file!");
                }
            } else {
                PlaybackEngine::StopPlayback();
                Logger::Log("Playback STOPPED");
            }
        }
    }
    
    // If we're recording, capture the current frame
    if (s_IsRecording) {
        DataLogger::CaptureFrame();
    }
    
    // If we're playing, update playback
    if (s_IsPlaying) {
        PlaybackEngine::UpdatePlayback();
        
        // Check if playback finished
        if (!PlaybackEngine::IsPlaying()) {
            s_IsPlaying = false;
        }
    }
}

// Plugin-SDK required functions
class ReplayModPlugin {
public:
    ReplayModPlugin() {
        // Hook into the game's main loop
        Events::gameProcessEvent += []() {
            GTAReplayMod::OnGameProcess();
        };
        
        // Initialize when the game starts
        Events::initGameEvent += []() {
            GTAReplayMod::Initialize();
        };
        
        // Cleanup when game shuts down
        Events::shutdownRwEvent += []() {
            GTAReplayMod::Shutdown();
        };
    }
} replayModPlugin;
