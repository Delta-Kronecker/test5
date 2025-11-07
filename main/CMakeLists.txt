cmake_minimum_required(VERSION 3.16)
project(ConfigCollector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings and treat warnings as errors for better code quality
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Network Test)

# Source files
set(SOURCES
    src/main.cpp
    src/ConfigManager.cpp
    src/Utils.cpp
    src/HttpHelper.cpp
    src/ProxyBean.cpp
    src/SubParser.cpp
    3rdparty/base64.cpp
)

# Test source files
set(TEST_SOURCES
    tests/test_configmanager.cpp
    tests/test_utils.cpp
    tests/test_http_helper.cpp
    tests/test_proxy_bean.cpp
)

# Executable for main program
add_executable(ConfigCollector ${SOURCES})

# Test executable
add_executable(ConfigCollectorTests
    tests/main_test.cpp
    ${SOURCES}
    ${TEST_SOURCES}
)

# Include directories
target_include_directories(ConfigCollector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

target_include_directories(ConfigCollectorTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# Link Qt libraries
target_link_libraries(ConfigCollector
    Qt6::Core
    Qt6::Network
)

target_link_libraries(ConfigCollectorTests
    Qt6::Core
    Qt6::Network
    Qt6::Test
)

# Enable testing
enable_testing()

# Set CTest configuration
set(BUILD_TESTING ON CACHE BOOL "Build tests" FORCE)

# Add test
add_test(NAME ConfigCollectorTests COMMAND ConfigCollectorTests)

# Set test properties
set_tests_properties(ConfigCollectorTests PROPERTIES
    TIMEOUT 60
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    # Environment variables for tests
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# Add custom test targets with different names to avoid conflict with CTest
add_custom_target(run-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ConfigCollectorTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running all tests"
)

add_custom_target(run-tests-verbose
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS ConfigCollectorTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running tests with verbose output"
)

# Custom target for running specific tests
add_custom_target(test-unit
    COMMAND ${CMAKE_CTEST_COMMAND} -R "ConfigCollectorTests" --output-on-failure
    DEPENDS ConfigCollectorTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running unit tests only"
)