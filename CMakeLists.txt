cmake_minimum_required(VERSION 3.15)
project(GTAReplayMod VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force 32-bit build (GTA SA is 32-bit)
set(CMAKE_GENERATOR_PLATFORM Win32)

# Download Plugin-SDK automatically
include(FetchContent)

message(STATUS "Downloading GTA SA Plugin-SDK...")
FetchContent_Declare(
    plugin-sdk
    GIT_REPOSITORY https://github.com/DK22Pac/plugin-sdk.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(plugin-sdk)

# Define our source files
set(SOURCES
    Plugin.cpp
    DataLogger.cpp
    PlaybackEngine.cpp
    Logger.cpp
)

set(HEADERS
    Plugin.h
    DataLogger.h
    PlaybackEngine.h
    Logger.h
)

# Create the .asi library (it's a DLL with .asi extension)
add_library(GTAReplayMod SHARED ${SOURCES} ${HEADERS})

# Set output name and extension
set_target_properties(GTAReplayMod PROPERTIES
    OUTPUT_NAME "GTAReplayMod"
    SUFFIX ".asi"
    PREFIX ""
)

# Plugin-SDK include directories
target_include_directories(GTAReplayMod PRIVATE
    ${plugin-sdk_SOURCE_DIR}/plugin_sa
    ${plugin-sdk_SOURCE_DIR}/plugin_sa/game_sa
    ${plugin-sdk_SOURCE_DIR}/shared
    ${plugin-sdk_SOURCE_DIR}/shared/game
)

# Preprocessor definitions for GTA SA
target_compile_definitions(GTAReplayMod PRIVATE
    _CRT_SECURE_NO_WARNINGS
    GTASA
    PLUGIN_SGV_10US
    _USING_V110_SDK71_
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# Compiler flags
if(MSVC)
    target_compile_options(GTAReplayMod PRIVATE
        /MT          # Static runtime library
        /W3          # Warning level 3
        /Zi          # Debug info
        /EHsc        # Exception handling
        /permissive- # Standards conformance
    )
    
    # Linker flags
    target_link_options(GTAReplayMod PRIVATE
        /SAFESEH:NO  # Disable safe exception handlers (required for GTA SA)
        /MACHINE:X86 # 32-bit
    )
endif()

# Link Plugin-SDK library
target_link_libraries(GTAReplayMod PRIVATE
    ${plugin-sdk_SOURCE_DIR}/output/lib/plugin.lib
)

# Copy output to a convenient location
install(TARGETS GTAReplayMod
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

message(STATUS "GTAReplayMod configuration complete!")
message(STATUS "Build this project to generate GTAReplayMod.asi")
