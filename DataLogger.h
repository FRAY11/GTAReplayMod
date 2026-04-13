#pragma once

#include <vector>
#include <fstream>
#include <CVector.h>

// Structure to hold a single entity's state at one moment in time
struct EntitySnapshot {
    // Entity identification
    int entityType;      // 0 = Ped, 1 = Vehicle, 2 = Object
    int modelId;         // The model ID (e.g., CJ skin, Infernus car model)
    int entityPoolIndex; // Index in the game's entity pool
    
    // Position and rotation
    CVector position;    // X, Y, Z coordinates
    CVector rotation;    // Rotation angles (pitch, yaw, roll)
    
    // Animation state (for peds)
    int animGroup;
    int animId;
    float animTime;
    
    // Vehicle-specific data
    float health;
    float speed;
    int primaryColor;
    int secondaryColor;
    
    // Flags
    bool isInVehicle;
    int vehicleIndex;    // If ped is in vehicle, which one
};

// Structure to hold the entire world state at one moment
struct FrameSnapshot {
    unsigned int frameNumber;
    unsigned int gameTime;       // CTimer::m_snTimeInMilliseconds
    
    // World state
    float weatherType;
    float timeOfDay;             // Hour (0-24)
    
    // Camera position (for reference)
    CVector cameraPosition;
    CVector cameraLookAt;
    
    // All entities in this frame
    std::vector<EntitySnapshot> entities;
};

// Main data logger class - captures and saves game state
class DataLogger {
public:
    static void Initialize();
    static void Shutdown();
    
    static void StartRecording();
    static void StopRecording();
    static void CaptureFrame();
    
private:
    static bool s_IsRecording;
    static unsigned int s_FrameCounter;
    static std::vector<FrameSnapshot> s_RecordedFrames;
    static std::ofstream s_ReplayFile;
    
    // Helper functions
    static void CaptureWorldState(FrameSnapshot& frame);
    static void CapturePeds(FrameSnapshot& frame);
    static void CaptureVehicles(FrameSnapshot& frame);
    static void SaveReplayToFile();
};
