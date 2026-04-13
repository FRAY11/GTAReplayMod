# GTA San Andreas Replay Mod

A professional-grade replay system for GTA San Andreas that records gameplay and plays it back with cinematic camera control.

## Features

### Phase 1: Memory State Logging ✅
- Real-time game state capture (60 FPS)
- Records all entities within radius (peds, vehicles)
- Captures weather, time, camera position
- Efficient binary serialization (.gtareplay format)

### Phase 2: Playback & Reconstruction ✅
- Frame-perfect replay reconstruction
- Defensive memory architecture (crash-proof)
- Automatic entity pool management
- AI system override for clean playback

### Phase 3: Cinematic Camera (Coming Soon)
- Freecam with keyframe system
- Catmull-Rom spline interpolation
- Smooth camera paths

### Phase 4: Video Export (Coming Soon)
- Decoupled rendering pipeline
- FFmpeg integration
- 60 FPS export regardless of game performance

## Installation

1. Download the latest `GTAReplayMod.asi` from [Releases](../../releases) or [Actions](../../actions)
2. Copy `GTAReplayMod.asi` to your GTA San Andreas game folder
3. Ensure you have an ASI Loader installed (CLEO, Silent's ASI Loader, etc.)
4. Launch the game!

## Usage

### Recording (F9)
1. Load your game
2. Press **F9** to start recording
3. Play normally (drive around, watch traffic, etc.)
4. Press **F9** again to stop
5. A `.gtareplay` file will be created in your game folder

### Playback (F10)
1. Rename your replay file to `test_replay.gtareplay`
2. Load your game (same location as recording recommended)
3. Press **F10** to start playback
4. Watch your recorded gameplay come to life!
5. Press **F10** again to stop

## Controls

| Key | Action |
|-----|--------|
| F9  | Start/Stop Recording |
| F10 | Start/Stop Playback |

## Technical Details

- **Language**: C++17
- **SDK**: GTA SA Plugin-SDK
- **Architecture**: Modular, defensive memory management
- **Compatibility**: GTA SA v1.0 US
- **Build System**: CMake + GitHub Actions

## Building from Source

### Cloud Build (Recommended)
1. Fork this repository
2. Go to **Actions** tab
3. Click **Build GTA Replay Mod** → **Run workflow**
4. Download the artifact when complete

### Local Build
Requires: Visual Studio 2019+, CMake 3.15+, Plugin-SDK

```bash
cmake -B build -A Win32
cmake --build build --config Release
```

## Architecture

```
GTAReplayMod/
├── Plugin.cpp/h          # Main entry point
├── DataLogger.cpp/h      # Phase 1: Recording system
├── PlaybackEngine.cpp/h  # Phase 2: Playback system
├── Logger.cpp/h          # Debug logging
└── CMakeLists.txt        # Build configuration
```

## Safety Features

- ✅ Validation checks before all memory operations
- ✅ Graceful degradation (skip instead of crash)
- ✅ Safe setter functions (no raw pointer manipulation)
- ✅ Entity pool overflow protection
- ✅ Automatic cleanup of distant entities
- ✅ Compatible with other .asi mods

## Troubleshooting

**Game crashes on startup:**
- Ensure you're using GTA SA v1.0 US (not Steam version)
- Check that ASI Loader is installed
- Review `GTAReplayMod.log` in game folder

**F9/F10 doesn't work:**
- Check `GTAReplayMod.log` to verify mod loaded
- Ensure no other mods are using these keys

**Playback doesn't work:**
- Verify replay file is named `test_replay.gtareplay`
- Check log file for errors
- Try recording in a different location

## Development Roadmap

- [x] Phase 1: Memory State Logging
- [x] Phase 2: Playback & Reconstruction
- [ ] Phase 3: Cinematic Camera System
- [ ] Phase 4: Video Export Pipeline
- [ ] Phase 5: UI/Menu System
- [ ] Phase 6: Advanced Features (slow-mo, filters, etc.)

## Credits

- **Director**: Project vision and design
- **Lead Developer**: Technical implementation
- **Plugin-SDK**: DK22Pac and contributors
- **RenderWare**: Criterion Software

## License

This project is for educational and personal use only. GTA San Andreas is property of Rockstar Games.

---

**Status**: Phase 2 Complete ✅  
**Next**: Phase 3 - Cinematic Camera System
