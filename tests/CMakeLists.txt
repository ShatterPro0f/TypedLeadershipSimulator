# Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.22)

# Create an alias for the main library that tests can use
if(NOT TARGET TLSCore)
    set(TLSCore TypedLeadershipLib)
endif()

# Phase 1 GoogleTest executable (if it exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase1Tests.cpp")
    add_executable(Phase1Tests Phase1Tests.cpp)
    target_include_directories(Phase1Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )
    target_link_libraries(Phase1Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )
    add_test(NAME Phase1Tests COMMAND Phase1Tests)
endif()

# Phase 2 Integration Tests
add_executable(Phase2IntegrationTests Phase2IntegrationTests.cpp)

target_link_libraries(Phase2IntegrationTests
    PRIVATE
        TypedLeadershipLib
        GTest::gtest_main
        GTest::gtest
)

target_include_directories(Phase2IntegrationTests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# Register test
add_test(NAME Phase2Integration COMMAND Phase2IntegrationTests)

# Set test output options
set_tests_properties(Phase2Integration
    PROPERTIES
    TIMEOUT 30
)

# Phase 3 System Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase3Tests.cpp")
    add_executable(Phase3Tests Phase3Tests.cpp)

    target_link_libraries(Phase3Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase3Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase3Systems COMMAND Phase3Tests)

    # Set test output options
    set_tests_properties(Phase3Systems
        PROPERTIES
        TIMEOUT 60
    )
endif()

# Phase 5 Action Registry Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase5_ActionRegistryTests.cpp")
    add_executable(Phase5ActionRegistryTests Phase5_ActionRegistryTests.cpp)

    target_link_libraries(Phase5ActionRegistryTests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase5ActionRegistryTests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase5ActionRegistry COMMAND Phase5ActionRegistryTests)

    # Set test output options
    set_tests_properties(Phase5ActionRegistry
        PROPERTIES
        TIMEOUT 30
    )
endif()

# Phase 5 Fuzzy Parser Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase5_FuzzyParserTests.cpp")
    add_executable(Phase5FuzzyParserTests Phase5_FuzzyParserTests.cpp)

    target_link_libraries(Phase5FuzzyParserTests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase5FuzzyParserTests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase5FuzzyParser COMMAND Phase5FuzzyParserTests)

    # Set test output options
    set_tests_properties(Phase5FuzzyParser
        PROPERTIES
        TIMEOUT 30
    )
endif()

# Phase 5 Parameter Extractor Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase5_ParameterExtractorTests.cpp")
    add_executable(Phase5ParameterExtractorTests Phase5_ParameterExtractorTests.cpp)

    target_link_libraries(Phase5ParameterExtractorTests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase5ParameterExtractorTests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase5ParameterExtractor COMMAND Phase5ParameterExtractorTests)

    # Set test output options
    set_tests_properties(Phase5ParameterExtractor
        PROPERTIES
        TIMEOUT 30
    )
endif()

# Phase 5 Command Validator Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase5_CommandValidatorTests.cpp")
    add_executable(Phase5CommandValidatorTests Phase5_CommandValidatorTests.cpp)

    target_link_libraries(Phase5CommandValidatorTests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase5CommandValidatorTests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase5CommandValidator COMMAND Phase5CommandValidatorTests)

    # Set test output options
    set_tests_properties(Phase5CommandValidator
        PROPERTIES
        TIMEOUT 30
    )
endif()

# Phase 5 Input UI Formatter Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase5_InputUIFormatterTests.cpp")
    add_executable(Phase5InputUIFormatterTests Phase5_InputUIFormatterTests.cpp)

    target_link_libraries(Phase5InputUIFormatterTests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase5InputUIFormatterTests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase5InputUIFormatter COMMAND Phase5InputUIFormatterTests)

    # Set test output options
    set_tests_properties(Phase5InputUIFormatter
        PROPERTIES
        TIMEOUT 30
    )
endif()

# Phase 6 Dialogue System Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase6_DialogueSystemTests.cpp")
    add_executable(Phase6DialogueSystemTests Phase6_DialogueSystemTests.cpp)

    target_link_libraries(Phase6DialogueSystemTests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase6DialogueSystemTests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase6DialogueSystem COMMAND Phase6DialogueSystemTests)

    # Set test output options
    set_tests_properties(Phase6DialogueSystem
        PROPERTIES
        TIMEOUT 30
    )
endif()

# Phase 7 LLM Infrastructure Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase7Tests.cpp")
    add_executable(Phase7Tests Phase7Tests.cpp)

    target_link_libraries(Phase7Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase7Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase7LLMInfrastructure COMMAND Phase7Tests)

    # Set test output options
    set_tests_properties(Phase7LLMInfrastructure
        PROPERTIES
        TIMEOUT 120
    )
endif()

# Phase 8 Decision Interpretation Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase8Tests.cpp")
    add_executable(Phase8Tests Phase8Tests.cpp)

    target_link_libraries(Phase8Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase8Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase8DecisionInterpretation COMMAND Phase8Tests)

    # Set test output options
    set_tests_properties(Phase8DecisionInterpretation
        PROPERTIES
        TIMEOUT 120
    )
endif()

# Phase 9 Narrative Generation Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase9Tests_Fixed.cpp")
    add_executable(Phase9Tests Phase9Tests_Fixed.cpp)

    target_link_libraries(Phase9Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase9Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase9NarrativeGeneration COMMAND Phase9Tests)

    # Set test output options
    set_tests_properties(Phase9NarrativeGeneration
        PROPERTIES
        TIMEOUT 120
    )
endif()

# Phase 10 Ambient Dialogue Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase10Tests.cpp")
    add_executable(Phase10Tests Phase10Tests.cpp)

    target_link_libraries(Phase10Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase10Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase10AmbientDialogue COMMAND Phase10Tests)

    # Set test output options
    set_tests_properties(Phase10AmbientDialogue
        PROPERTIES
        TIMEOUT 120
    )
endif()

# Phase 10 Integration Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase10IntegrationTests.cpp")
    add_executable(Phase10IntegrationTests Phase10IntegrationTests.cpp)

    target_link_libraries(Phase10IntegrationTests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase10IntegrationTests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase10Integration COMMAND Phase10IntegrationTests)

    # Set test output options
    set_tests_properties(Phase10Integration
        PROPERTIES
        TIMEOUT 120
    )
endif()

# Phase 11 Pathfinding and Movement Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase11Tests.cpp")
    add_executable(Phase11Tests Phase11Tests.cpp)

    target_link_libraries(Phase11Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase11Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase11PathfindingMovement COMMAND Phase11Tests)

    # Set test output options
    set_tests_properties(Phase11PathfindingMovement
        PROPERTIES
        TIMEOUT 120
    )
endif()

# Phase 12 Main Game Loop & Integration Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase12Tests.cpp")
    add_executable(Phase12Tests Phase12Tests.cpp)

    target_link_libraries(Phase12Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase12Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase12GameLoopIntegration COMMAND Phase12Tests)

    # Set test output options
    set_tests_properties(Phase12GameLoopIntegration
        PROPERTIES
        TIMEOUT 180
    )
endif()

# Phase 13 LLM Integration Tests (Ollama Backend)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase13Tests.cpp")
    add_executable(Phase13Tests Phase13Tests.cpp)

    target_link_libraries(Phase13Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase13Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase13LLMIntegration COMMAND Phase13Tests)

    # Set test output options
    set_tests_properties(Phase13LLMIntegration
        PROPERTIES
        TIMEOUT 180
    )
endif()

# Phase 14 Save/Load & Data Persistence Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Phase14Tests.cpp")
    add_executable(Phase14Tests Phase14Tests.cpp)

    target_link_libraries(Phase14Tests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(Phase14Tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase14SaveLoadPersistence COMMAND Phase14Tests)

    # Set test output options
    set_tests_properties(Phase14SaveLoadPersistence
        PROPERTIES
        TIMEOUT 180
    )
endif()

# Phase 15 Task 6: Conversation Queue Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ConversationQueueTests.cpp")
    add_executable(ConversationQueueTests ConversationQueueTests.cpp)

    target_link_libraries(ConversationQueueTests
        PRIVATE
            TypedLeadershipLib
            GTest::gtest_main
            GTest::gtest
    )

    target_include_directories(ConversationQueueTests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    # Register test
    add_test(NAME Phase15ConversationQueue COMMAND ConversationQueueTests)

    # Set test output options
    set_tests_properties(Phase15ConversationQueue
        PROPERTIES
        TIMEOUT 30
    )
endif()

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Test Suite Configuration Complete")
message(STATUS "========================================")
message(STATUS "Configured tests:")
message(STATUS "  - Phase 1: Core systems initialization")
message(STATUS "  - Phase 2: Integration basics")
message(STATUS "  - Phase 3: NPC/Advisor systems")
message(STATUS "  - Phase 5: Action Registry")
message(STATUS "  - Phase 6: Dialogue System")
message(STATUS "  - Phase 7: LLM Infrastructure")
message(STATUS "  - Phase 8: Decision Interpretation")
message(STATUS "  - Phase 9: Narrative Generation")
message(STATUS "  - Phase 10: Ambient Dialogue")
message(STATUS "  - Phase 11: Pathfinding & Movement (50+ test cases)")
message(STATUS "  - Phase 12: Game Loop & Integration (48 test cases)")
message(STATUS "  - Phase 15: Conversation Queue (27 test cases)")
message(STATUS "========================================")
message(STATUS "")
