#pragma once

#include <vector>
#include <fstream>
#include <map>
#include "DataLogger.h"

// Tracks a spawned entity during playback
struct PlaybackEntity {
    int gameEntityIndex;     // Index in CPools (our spawned entity)
    int recordedEntityIndex; // Index from the replay file
    int entityType;          // 0 = Ped, 1 = Vehicle
    int modelId;
    bool isActive;
    unsigned int lastUpdateFrame;
};

// Main playback engine - loads and plays back recorded replays
class PlaybackEngine {
public:
    static void Initialize();
    static void Shutdown();
    
    static bool LoadReplayFile(const char* filename);
    static void StartPlayback();
    static void StopPlayback();
    static void UpdatePlayback();
    
    static bool IsPlaying() { return s_IsPlaying; }
    static unsigned int GetCurrentFrame() { return s_CurrentFrame; }
    static unsigned int GetTotalFrames() { return static_cast<unsigned int>(s_LoadedFrames.size()); }
    
private:
    static bool s_IsPlaying;
    static unsigned int s_CurrentFrame;
    static std::vector<FrameSnapshot> s_LoadedFrames;
    static std::map<int, PlaybackEntity> s_SpawnedEntities;
    
    // Helper functions
    static bool ReadReplayFile(const char* filename);
    static void ProcessFrame(const FrameSnapshot& frame);
    static void ApplyWorldState(const FrameSnapshot& frame);
    static void UpdateEntities(const FrameSnapshot& frame);
    static void CleanupDistantEntities(const CVector& cameraPos);
    static PlaybackEntity* FindOrCreateEntity(const EntitySnapshot& snapshot);
    static void UpdateEntityTransform(PlaybackEntity* playbackEnt, const EntitySnapshot& snapshot);
    static void DisableGameAI();
    static void EnableGameAI();
};
