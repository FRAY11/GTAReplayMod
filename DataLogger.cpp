#include "DataLogger.h"
#include "Logger.h"
#include <CTimer.h>
#include <CWeather.h>
#include <CClock.h>
#include <CCamera.h>
#include <CWorld.h>
#include <CPools.h>
#include <CPed.h>
#include <CVehicle.h>
#include <CPlayerPed.h>
#include <CStreaming.h>
#include <RenderWare.h>

// Static member initialization
bool DataLogger::s_IsRecording = false;
unsigned int DataLogger::s_FrameCounter = 0;
std::vector<FrameSnapshot> DataLogger::s_RecordedFrames;
std::ofstream DataLogger::s_ReplayFile;

void DataLogger::Initialize() {
    s_RecordedFrames.clear();
    s_FrameCounter = 0;
    Logger::Log("DataLogger initialized");
}

void DataLogger::Shutdown() {
    if (s_IsRecording) {
        StopRecording();
    }
    s_RecordedFrames.clear();
}

void DataLogger::StartRecording() {
    if (s_IsRecording) return;
    
    s_IsRecording = true;
    s_FrameCounter = 0;
    s_RecordedFrames.clear();
    s_RecordedFrames.reserve(18000); // Reserve space for ~5 minutes at 60fps
    
    Logger::Log("Recording started - capturing game state every frame");
}

void DataLogger::StopRecording() {
    if (!s_IsRecording) return;
    
    s_IsRecording = false;
    Logger::Log("Recording stopped - saving replay file...");
    
    SaveReplayToFile();
    
    Logger::Log("Replay saved successfully!");
    Logger::Log("Total frames captured: " + std::to_string(s_FrameCounter));
}

void DataLogger::CaptureFrame() {
    if (!s_IsRecording) return;
    
    // Create a new frame snapshot
    FrameSnapshot frame;
    frame.frameNumber = s_FrameCounter++;
    frame.gameTime = CTimer::m_snTimeInMilliseconds;
    
    // Capture world state (weather, time, camera)
    CaptureWorldState(frame);
    
    // Capture all peds (people) near the player
    CapturePeds(frame);
    
    // Capture all vehicles near the player
    CaptureVehicles(frame);
    
    // Add this frame to our recording
    s_RecordedFrames.push_back(frame);
    
    // Log progress every 60 frames (roughly every second)
    if (s_FrameCounter % 60 == 0) {
        Logger::Log("Captured " + std::to_string(s_FrameCounter) + " frames, " + 
                    std::to_string(frame.entities.size()) + " entities in current frame");
    }
}

void DataLogger::CaptureWorldState(FrameSnapshot& frame) {
    // Capture weather
    frame.weatherType = static_cast<float>(CWeather::OldWeatherType);
    
    // Capture time of day
    frame.timeOfDay = static_cast<float>(CClock::ms_nGameClockHours) + 
                      (static_cast<float>(CClock::ms_nGameClockMinutes) / 60.0f);
    
    // Capture camera position and direction
    CCamera* camera = &TheCamera;
    if (camera) {
        frame.cameraPosition = camera->GetPosition();
        
        // Calculate look-at point from camera front vector
        CVector front = camera->m_mCameraMatrix.GetForward();
        frame.cameraLookAt = frame.cameraPosition + front * 10.0f;
    }
}

void DataLogger::CapturePeds(FrameSnapshot& frame) {
    // Get the player's position to calculate distance
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;
    
    CVector playerPos = player->GetPosition();
    const float captureRadius = 150.0f; // Capture peds within 150 units
    
    // Iterate through all peds in the game's ped pool
    for (int i = 0; i < CPools::ms_pPedPool->m_nSize; i++) {
        CPed* ped = CPools::ms_pPedPool->GetAt(i);
        
        // Skip if ped doesn't exist or is the player
        if (!ped || ped == player) continue;
        
        CVector pedPos = ped->GetPosition();
        float distance = (pedPos - playerPos).Magnitude();
        
        // Only capture peds within our radius
        if (distance > captureRadius) continue;
        
        // Create entity snapshot for this ped
        EntitySnapshot entity;
        entity.entityType = 0; // Ped
        entity.modelId = ped->m_nModelIndex;
        entity.entityPoolIndex = i;
        
        // Position and rotation
        entity.position = pedPos;
        
        // Get rotation from ped's matrix
        CMatrix* matrix = ped->GetMatrix();
        if (matrix) {
            // Convert matrix to euler angles (simplified)
            entity.rotation.z = atan2(matrix->GetRight().y, matrix->GetRight().x);
            entity.rotation.x = 0.0f;
            entity.rotation.y = 0.0f;
        }
        
        // Animation state
        if (ped->m_pRwObject) {
            // Capture current animation (simplified - full anim system is complex)
            entity.animGroup = 0;
            entity.animId = 0;
            entity.animTime = 0.0f;
        }
        
        // Check if ped is in vehicle
        entity.isInVehicle = (ped->m_pVehicle != nullptr);
        if (entity.isInVehicle && ped->m_pVehicle) {
            // Find vehicle index in pool
            entity.vehicleIndex = CPools::ms_pVehiclePool->GetIndex(ped->m_pVehicle);
        } else {
            entity.vehicleIndex = -1;
        }
        
        // Health
        entity.health = ped->m_fHealth;
        entity.speed = ped->m_vecMoveSpeed.Magnitude();
        
        frame.entities.push_back(entity);
    }
}

void DataLogger::CaptureVehicles(FrameSnapshot& frame) {
    // Get the player's position
    CPlayerPed* player = FindPlayerPed();
    if (!player) return;
    
    CVector playerPos = player->GetPosition();
    const float captureRadius = 200.0f; // Capture vehicles within 200 units
    
    // Iterate through all vehicles in the game's vehicle pool
    for (int i = 0; i < CPools::ms_pVehiclePool->m_nSize; i++) {
        CVehicle* vehicle = CPools::ms_pVehiclePool->GetAt(i);
        
        if (!vehicle) continue;
        
        CVector vehPos = vehicle->GetPosition();
        float distance = (vehPos - playerPos).Magnitude();
        
        // Only capture vehicles within our radius
        if (distance > captureRadius) continue;
        
        // Create entity snapshot for this vehicle
        EntitySnapshot entity;
        entity.entityType = 1; // Vehicle
        entity.modelId = vehicle->m_nModelIndex;
        entity.entityPoolIndex = i;
        
        // Position and rotation
        entity.position = vehPos;
        
        // Get rotation from vehicle's matrix
        CMatrix* matrix = vehicle->GetMatrix();
        if (matrix) {
            // Convert matrix to euler angles
            entity.rotation.z = atan2(matrix->GetRight().y, matrix->GetRight().x);
            entity.rotation.x = asin(-matrix->GetRight().z);
            entity.rotation.y = 0.0f;
        }
        
        // Vehicle-specific data
        entity.health = vehicle->m_fHealth;
        entity.speed = vehicle->m_vecMoveSpeed.Magnitude();
        entity.primaryColor = vehicle->m_nPrimaryColor;
        entity.secondaryColor = vehicle->m_nSecondaryColor;
        
        entity.isInVehicle = false;
        entity.vehicleIndex = -1;
        
        frame.entities.push_back(entity);
    }
}

void DataLogger::SaveReplayToFile() {
    // Generate filename with timestamp
    time_t now = time(0);
    tm* ltm = localtime(&now);
    
    char filename[256];
    sprintf_s(filename, "replay_%04d%02d%02d_%02d%02d%02d.gtareplay",
              1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
              ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    
    // Open binary file for writing
    s_ReplayFile.open(filename, std::ios::binary | std::ios::out);
    
    if (!s_ReplayFile.is_open()) {
        Logger::Log("ERROR: Could not create replay file!");
        return;
    }
    
    // Write file header
    const char* magic = "GTAR"; // File signature
    s_ReplayFile.write(magic, 4);
    
    unsigned int version = 1;
    s_ReplayFile.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    unsigned int frameCount = static_cast<unsigned int>(s_RecordedFrames.size());
    s_ReplayFile.write(reinterpret_cast<const char*>(&frameCount), sizeof(frameCount));
    
    // Write all frames
    for (const auto& frame : s_RecordedFrames) {
        // Write frame header
        s_ReplayFile.write(reinterpret_cast<const char*>(&frame.frameNumber), sizeof(frame.frameNumber));
        s_ReplayFile.write(reinterpret_cast<const char*>(&frame.gameTime), sizeof(frame.gameTime));
        s_ReplayFile.write(reinterpret_cast<const char*>(&frame.weatherType), sizeof(frame.weatherType));
        s_ReplayFile.write(reinterpret_cast<const char*>(&frame.timeOfDay), sizeof(frame.timeOfDay));
        s_ReplayFile.write(reinterpret_cast<const char*>(&frame.cameraPosition), sizeof(frame.cameraPosition));
        s_ReplayFile.write(reinterpret_cast<const char*>(&frame.cameraLookAt), sizeof(frame.cameraLookAt));
        
        // Write entity count
        unsigned int entityCount = static_cast<unsigned int>(frame.entities.size());
        s_ReplayFile.write(reinterpret_cast<const char*>(&entityCount), sizeof(entityCount));
        
        // Write all entities
        for (const auto& entity : frame.entities) {
            s_ReplayFile.write(reinterpret_cast<const char*>(&entity), sizeof(EntitySnapshot));
        }
    }
    
    s_ReplayFile.close();
    Logger::Log("Replay file saved: " + std::string(filename));
}
