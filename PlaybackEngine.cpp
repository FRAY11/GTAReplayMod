#include "PlaybackEngine.h"
#include "Logger.h"
#include <CTimer.h>
#include <CWeather.h>
#include <CClock.h>
#include <CCamera.h>
#include <CWorld.h>
#include <CPools.h>
#include <CPed.h>
#include <CVehicle.h>
#include <CStreaming.h>
#include <CPopulation.h>
#include <CCarCtrl.h>
#include <CModelInfo.h>
#include <RenderWare.h>

// Static member initialization
bool PlaybackEngine::s_IsPlaying = false;
unsigned int PlaybackEngine::s_CurrentFrame = 0;
std::vector<FrameSnapshot> PlaybackEngine::s_LoadedFrames;
std::map<int, PlaybackEntity> PlaybackEngine::s_SpawnedEntities;

void PlaybackEngine::Initialize() {
    s_LoadedFrames.clear();
    s_SpawnedEntities.clear();
    s_CurrentFrame = 0;
    Logger::Log("PlaybackEngine initialized");
}

void PlaybackEngine::Shutdown() {
    if (s_IsPlaying) {
        StopPlayback();
    }
    s_LoadedFrames.clear();
    s_SpawnedEntities.clear();
}

bool PlaybackEngine::LoadReplayFile(const char* filename) {
    Logger::Log("Loading replay file: " + std::string(filename));
    
    if (!ReadReplayFile(filename)) {
        Logger::Log("ERROR: Failed to load replay file!");
        return false;
    }
    
    Logger::Log("Replay loaded successfully!");
    Logger::Log("Total frames: " + std::to_string(s_LoadedFrames.size()));
    return true;
}

bool PlaybackEngine::ReadReplayFile(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    
    if (!file.is_open()) {
        Logger::Log("ERROR: Could not open replay file!");
        return false;
    }
    
    // Read and verify file header
    char magic[5] = {0};
    file.read(magic, 4);
    
    if (strcmp(magic, "GTAR") != 0) {
        Logger::Log("ERROR: Invalid replay file format!");
        file.close();
        return false;
    }
    
    unsigned int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (version != 1) {
        Logger::Log("ERROR: Unsupported replay version!");
        file.close();
        return false;
    }
    
    unsigned int frameCount;
    file.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));
    
    s_LoadedFrames.clear();
    s_LoadedFrames.reserve(frameCount);
    
    // Read all frames
    for (unsigned int i = 0; i < frameCount; i++) {
        FrameSnapshot frame;
        
        // Read frame header
        file.read(reinterpret_cast<char*>(&frame.frameNumber), sizeof(frame.frameNumber));
        file.read(reinterpret_cast<char*>(&frame.gameTime), sizeof(frame.gameTime));
        file.read(reinterpret_cast<char*>(&frame.weatherType), sizeof(frame.weatherType));
        file.read(reinterpret_cast<char*>(&frame.timeOfDay), sizeof(frame.timeOfDay));
        file.read(reinterpret_cast<char*>(&frame.cameraPosition), sizeof(frame.cameraPosition));
        file.read(reinterpret_cast<char*>(&frame.cameraLookAt), sizeof(frame.cameraLookAt));
        
        // Read entity count
        unsigned int entityCount;
        file.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));
        
        frame.entities.reserve(entityCount);
        
        // Read all entities
        for (unsigned int j = 0; j < entityCount; j++) {
            EntitySnapshot entity;
            file.read(reinterpret_cast<char*>(&entity), sizeof(EntitySnapshot));
            frame.entities.push_back(entity);
        }
        
        s_LoadedFrames.push_back(frame);
    }
    
    file.close();
    return true;
}

void PlaybackEngine::StartPlayback() {
    if (s_IsPlaying) {
        Logger::Log("Playback already running!");
        return;
    }
    
    if (s_LoadedFrames.empty()) {
        Logger::Log("ERROR: No replay loaded!");
        return;
    }
    
    s_IsPlaying = true;
    s_CurrentFrame = 0;
    s_SpawnedEntities.clear();
    
    // Disable game's AI systems to prevent interference
    DisableGameAI();
    
    Logger::Log("Playback started!");
}

void PlaybackEngine::StopPlayback() {
    if (!s_IsPlaying) return;
    
    s_IsPlaying = false;
    
    // Clean up all spawned entities
    for (auto& pair : s_SpawnedEntities) {
        PlaybackEntity& ent = pair.second;
        
        if (!ent.isActive) continue;
        
        // Safely remove entity from game world
        if (ent.entityType == 0) { // Ped
            CPed* ped = CPools::ms_pPedPool->GetAt(ent.gameEntityIndex);
            if (ped) {
                CWorld::Remove(ped);
                delete ped;
            }
        } else if (ent.entityType == 1) { // Vehicle
            CVehicle* vehicle = CPools::ms_pVehiclePool->GetAt(ent.gameEntityIndex);
            if (vehicle) {
                CWorld::Remove(vehicle);
                delete vehicle;
            }
        }
    }
    
    s_SpawnedEntities.clear();
    
    // Re-enable game's AI systems
    EnableGameAI();
    
    Logger::Log("Playback stopped!");
}

void PlaybackEngine::UpdatePlayback() {
    if (!s_IsPlaying) return;
    
    // Check if we've reached the end
    if (s_CurrentFrame >= s_LoadedFrames.size()) {
        Logger::Log("Playback finished - reached end of replay");
        StopPlayback();
        return;
    }
    
    // Get current frame data
    const FrameSnapshot& frame = s_LoadedFrames[s_CurrentFrame];
    
    // Process this frame
    ProcessFrame(frame);
    
    // Advance to next frame
    s_CurrentFrame++;
    
    // Log progress every 60 frames
    if (s_CurrentFrame % 60 == 0) {
        Logger::Log("Playback progress: " + std::to_string(s_CurrentFrame) + " / " + 
                    std::to_string(s_LoadedFrames.size()) + " frames");
    }
}

void PlaybackEngine::ProcessFrame(const FrameSnapshot& frame) {
    // Apply world state (weather, time)
    ApplyWorldState(frame);
    
    // Update all entities
    UpdateEntities(frame);
    
    // Clean up entities that are too far from camera
    CleanupDistantEntities(frame.cameraPosition);
}

void PlaybackEngine::ApplyWorldState(const FrameSnapshot& frame) {
    // Set weather
    CWeather::OldWeatherType = static_cast<unsigned short>(frame.weatherType);
    CWeather::NewWeatherType = static_cast<unsigned short>(frame.weatherType);
    
    // Set time of day
    int hours = static_cast<int>(frame.timeOfDay);
    int minutes = static_cast<int>((frame.timeOfDay - hours) * 60.0f);
    
    CClock::ms_nGameClockHours = static_cast<unsigned char>(hours);
    CClock::ms_nGameClockMinutes = static_cast<unsigned char>(minutes);
}

void PlaybackEngine::UpdateEntities(const FrameSnapshot& frame) {
    // Process each entity in the frame
    for (const auto& snapshot : frame.entities) {
        // Find or create the playback entity
        PlaybackEntity* playbackEnt = FindOrCreateEntity(snapshot);
        
        if (!playbackEnt) continue; // Failed to create/find entity
        
        // Update the entity's transform (position, rotation)
        UpdateEntityTransform(playbackEnt, snapshot);
    }
}

PlaybackEntity* PlaybackEngine::FindOrCreateEntity(const EntitySnapshot& snapshot) {
    // Check if we already have this entity spawned
    int recordedIndex = snapshot.entityPoolIndex;
    
    auto it = s_SpawnedEntities.find(recordedIndex);
    if (it != s_SpawnedEntities.end()) {
        // Entity already exists
        it->second.lastUpdateFrame = s_CurrentFrame;
        return &it->second;
    }
    
    // Need to spawn a new entity
    PlaybackEntity newEntity;
    newEntity.recordedEntityIndex = recordedIndex;
    newEntity.entityType = snapshot.entityType;
    newEntity.modelId = snapshot.modelId;
    newEntity.isActive = false;
    newEntity.gameEntityIndex = -1;
    newEntity.lastUpdateFrame = s_CurrentFrame;
    
    // Request model to be loaded
    CStreaming::RequestModel(snapshot.modelId, STREAMING_KEEP_IN_MEMORY);
    CStreaming::LoadAllRequestedModels(false);
    
    // Check if model is loaded
    if (!CStreaming::HasModelLoaded(snapshot.modelId)) {
        Logger::Log("WARNING: Model " + std::to_string(snapshot.modelId) + " not loaded yet");
        return nullptr;
    }
    
    // Spawn the entity based on type
    if (snapshot.entityType == 0) { // Ped
        // DEFENSIVE: Validate model is a ped model
        CBaseModelInfo* modelInfo = CModelInfo::GetModelInfo(snapshot.modelId);
        if (!modelInfo || modelInfo->GetModelType() != MODEL_INFO_PED) {
            Logger::Log("ERROR: Invalid ped model ID: " + std::to_string(snapshot.modelId));
            return nullptr;
        }
        
        // Create ped at position
        CPed* ped = new CPed(PEDTYPE_CIVMALE);
        if (!ped) {
            Logger::Log("ERROR: Failed to allocate ped");
            return nullptr;
        }
        
        ped->SetModelIndex(snapshot.modelId);
        ped->SetPosn(snapshot.position);
        ped->SetOrientation(0.0f, 0.0f, snapshot.rotation.z);
        
        // Add to world
        CWorld::Add(ped);
        
        // Get pool index
        int poolIndex = CPools::ms_pPedPool->GetIndex(ped);
        if (poolIndex == -1) {
            Logger::Log("ERROR: Failed to get ped pool index");
            delete ped;
            return nullptr;
        }
        
        newEntity.gameEntityIndex = poolIndex;
        newEntity.isActive = true;
        
    } else if (snapshot.entityType == 1) { // Vehicle
        // DEFENSIVE: Validate model is a vehicle model
        CBaseModelInfo* modelInfo = CModelInfo::GetModelInfo(snapshot.modelId);
        if (!modelInfo || modelInfo->GetModelType() != MODEL_INFO_VEHICLE) {
            Logger::Log("ERROR: Invalid vehicle model ID: " + std::to_string(snapshot.modelId));
            return nullptr;
        }
        
        // Create vehicle at position
        CVehicle* vehicle = new CAutomobile(snapshot.modelId, RANDOM_VEHICLE, true);
        if (!vehicle) {
            Logger::Log("ERROR: Failed to allocate vehicle");
            return nullptr;
        }
        
        vehicle->SetPosn(snapshot.position);
        vehicle->SetOrientation(0.0f, 0.0f, snapshot.rotation.z);
        vehicle->m_nPrimaryColor = snapshot.primaryColor;
        vehicle->m_nSecondaryColor = snapshot.secondaryColor;
        
        // Add to world
        CWorld::Add(vehicle);
        
        // Get pool index
        int poolIndex = CPools::ms_pVehiclePool->GetIndex(vehicle);
        if (poolIndex == -1) {
            Logger::Log("ERROR: Failed to get vehicle pool index");
            delete vehicle;
            return nullptr;
        }
        
        newEntity.gameEntityIndex = poolIndex;
        newEntity.isActive = true;
    }
    
    // Add to our tracking map
    s_SpawnedEntities[recordedIndex] = newEntity;
    return &s_SpawnedEntities[recordedIndex];
}

void PlaybackEngine::UpdateEntityTransform(PlaybackEntity* playbackEnt, const EntitySnapshot& snapshot) {
    if (!playbackEnt || !playbackEnt->isActive) return;
    
    // DEFENSIVE: Validate entity still exists in pool
    if (playbackEnt->entityType == 0) { // Ped
        CPed* ped = CPools::ms_pPedPool->GetAt(playbackEnt->gameEntityIndex);
        if (!ped) {
            playbackEnt->isActive = false;
            return;
        }
        
        // DEFENSIVE: Check if ped is valid before modifying
        if (ped->m_nPedFlags.bFadeOut || ped->m_nPedFlags.bRemoveFromWorld) {
            return; // Don't touch entities being removed
        }
        
        // Update position using safe setter
        ped->SetPosn(snapshot.position);
        
        // Update rotation
        ped->SetOrientation(0.0f, 0.0f, snapshot.rotation.z);
        
        // Update health
        ped->m_fHealth = snapshot.health;
        
        // Freeze ped (disable AI)
        ped->m_nPedFlags.bStayInSamePlace = true;
        
    } else if (playbackEnt->entityType == 1) { // Vehicle
        CVehicle* vehicle = CPools::ms_pVehiclePool->GetAt(playbackEnt->gameEntityIndex);
        if (!vehicle) {
            playbackEnt->isActive = false;
            return;
        }
        
        // DEFENSIVE: Check if vehicle is valid
        if (vehicle->m_nVehicleFlags.bFadeOut) {
            return; // Don't touch vehicles being removed
        }
        
        // Update position using safe setter
        vehicle->SetPosn(snapshot.position);
        
        // Update rotation
        vehicle->SetOrientation(0.0f, 0.0f, snapshot.rotation.z);
        
        // Update health
        vehicle->m_fHealth = snapshot.health;
        
        // Zero out velocity (keep vehicle stationary until next frame)
        vehicle->m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
        vehicle->m_vecTurnSpeed = CVector(0.0f, 0.0f, 0.0f);
    }
}

void PlaybackEngine::CleanupDistantEntities(const CVector& cameraPos) {
    const float cleanupRadius = 300.0f; // Remove entities beyond 300 units
    
    std::vector<int> toRemove;
    
    for (auto& pair : s_SpawnedEntities) {
        PlaybackEntity& ent = pair.second;
        
        if (!ent.isActive) continue;
        
        // Check if entity hasn't been updated recently (not in current frame)
        if (ent.lastUpdateFrame < s_CurrentFrame - 60) { // Not updated for 1 second
            toRemove.push_back(pair.first);
            continue;
        }
        
        // Check distance from camera
        CVector entityPos;
        bool shouldRemove = false;
        
        if (ent.entityType == 0) { // Ped
            CPed* ped = CPools::ms_pPedPool->GetAt(ent.gameEntityIndex);
            if (ped) {
                entityPos = ped->GetPosition();
                float distance = (entityPos - cameraPos).Magnitude();
                if (distance > cleanupRadius) {
                    shouldRemove = true;
                }
            } else {
                shouldRemove = true; // Entity no longer exists
            }
        } else if (ent.entityType == 1) { // Vehicle
            CVehicle* vehicle = CPools::ms_pVehiclePool->GetAt(ent.gameEntityIndex);
            if (vehicle) {
                entityPos = vehicle->GetPosition();
                float distance = (entityPos - cameraPos).Magnitude();
                if (distance > cleanupRadius) {
                    shouldRemove = true;
                }
            } else {
                shouldRemove = true; // Entity no longer exists
            }
        }
        
        if (shouldRemove) {
            toRemove.push_back(pair.first);
        }
    }
    
    // Remove distant entities
    for (int index : toRemove) {
        auto it = s_SpawnedEntities.find(index);
        if (it != s_SpawnedEntities.end()) {
            PlaybackEntity& ent = it->second;
            
            // Safely remove from world
            if (ent.entityType == 0) {
                CPed* ped = CPools::ms_pPedPool->GetAt(ent.gameEntityIndex);
                if (ped) {
                    CWorld::Remove(ped);
                    delete ped;
                }
            } else if (ent.entityType == 1) {
                CVehicle* vehicle = CPools::ms_pVehiclePool->GetAt(ent.gameEntityIndex);
                if (vehicle) {
                    CWorld::Remove(vehicle);
                    delete vehicle;
                }
            }
            
            s_SpawnedEntities.erase(it);
        }
    }
}

void PlaybackEngine::DisableGameAI() {
    // Disable population generation (prevents random peds/vehicles from spawning)
    CPopulation::m_AllRandomPedsThisType = -1;
    CPopulation::PedDensityMultiplier = 0.0f;
    CCarCtrl::CarDensityMultiplier = 0.0f;
    
    Logger::Log("Game AI disabled for playback");
}

void PlaybackEngine::EnableGameAI() {
    // Re-enable population generation
    CPopulation::m_AllRandomPedsThisType = -1;
    CPopulation::PedDensityMultiplier = 1.0f;
    CCarCtrl::CarDensityMultiplier = 1.0f;
    
    Logger::Log("Game AI re-enabled");
}
