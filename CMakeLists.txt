cmake_minimum_required(VERSION 3.10)
project(TypedLeadershipSimulator VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set MSVC runtime library to dynamic (MD) for test compatibility
if(MSVC)
    # Use dynamic runtime to match test executables
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Set optimization and warning flags
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O2)
    if(MINGW)
        add_compile_options(-static-libgcc -static-libstdc++)
        # cpp-httplib requires Windows 10+ API
        add_compile_definitions(_WIN32_WINNT=0x0A00)
    endif()
endif()

# Add project root include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add external dependencies include
include_directories(${PROJECT_SOURCE_DIR}/external)

# Source files (organize by subsystem)
set(CORE_SOURCES
    src/core/Enums.cpp
    src/core/Vector3.cpp
    src/core/Core.cpp
    src/core/Registries.cpp
    src/WorldState.cpp
)

set(PHASE2_SOURCES
    src/core/LLM.cpp
    src/core/EntityFactory.cpp
    src/core/Serialization.cpp
)

set(PHASE3_SOURCES
    src/phase3/World.cpp
    src/phase3/Collision.cpp
    src/phase3/Player.cpp
    src/phase3/Pathfinding.cpp
    src/phase3/ProximityDetection.cpp
    src/phase3/NPCMovement.cpp
    src/phase3/ActivitySystem.cpp
)

set(PHASE4_SOURCES
    src/phase4/SimulationManager.cpp
    src/phase4/InputParser.cpp
    src/phase4/GameTime.cpp
    src/phase4/DialogueSystem.cpp
    src/phase4/UIFormatter.cpp
    src/phase4/ReplaySystem.cpp
)

set(PHASE5_SOURCES
    src/core/ActionRegistry.cpp
    src/core/FuzzyParser.cpp
    src/core/ParameterExtractor.cpp
    src/core/CommandValidator.cpp
    src/core/InputUIFormatter.cpp
)

set(PHASE6_SOURCES
    src/core/DialogueState.cpp
    src/core/ProximityDialogueSystem.cpp
    src/core/NpcProblemSystem.cpp
    src/core/DialogueUI.cpp
    src/core/DialogueController.cpp
)

set(PHASE7_SOURCES
    src/LLMProvider.cpp
    src/OllamaProvider.cpp
    src/LLMConfigManager.cpp
    src/LLMRequestQueue.cpp
    src/LLMResponseCache.cpp
    src/LLMErrorRecovery.cpp
    src/LLMDeterministicReplay.cpp
    src/LLMTokenTracker.cpp
    src/GameDataLoader.cpp
    src/ConsoleRenderer.cpp
)

set(PHASE8_SOURCES
    src/DecisionInterpreter.cpp
    src/NarrativeGeneration.cpp
    src/SimulationManager.cpp
)

set(PHASE15_SOURCES
    src/ConversationQueue.cpp
)

set(PHASE9_SOURCES
    src/interpreters/NarrativeGeneration.cpp
)

set(PHASE10_SOURCES
    src/core/AmbientDialogueSystem.cpp
)

set(PHASE11_SOURCES
    src/core/PathfindingEngine.cpp
    src/core/MovementController.cpp
)

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${PHASE2_SOURCES}
    ${PHASE3_SOURCES}
    ${PHASE4_SOURCES}
    ${PHASE5_SOURCES}
    ${PHASE6_SOURCES}
    ${PHASE7_SOURCES}
    ${PHASE8_SOURCES}
    ${PHASE10_SOURCES}
    ${PHASE11_SOURCES}
    ${PHASE15_SOURCES}
)

# Main library (if any global initialization needed)
add_library(TypedLeadershipLib STATIC ${ALL_SOURCES})
target_include_directories(TypedLeadershipLib PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Link Windows socket library for HTTP client (cpp-httplib)
if(WIN32)
    target_link_libraries(TypedLeadershipLib PUBLIC ws2_32)
endif()

# ============================================================================
# Game Executable
# ============================================================================
add_executable(TypedLeadershipGame src/main.cpp)
target_link_libraries(TypedLeadershipGame PRIVATE TypedLeadershipLib)
target_include_directories(TypedLeadershipGame PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Copy data files to build output directory
add_custom_command(TARGET TypedLeadershipGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:TypedLeadershipGame>/data"
    COMMENT "Copying data files to output directory"
)

# GoogleTest integration via FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)

# Force GoogleTest to use shared C++ runtime (MD) on Windows
if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)
