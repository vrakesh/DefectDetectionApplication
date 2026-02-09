cmake_minimum_required(VERSION 3.16)
project(dda_backend_cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Crow (header-only, fetched via FetchContent)
include(FetchContent)
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.0+5
)
FetchContent_MakeAvailable(Crow)

# SQLite3
find_package(SQLite3 REQUIRED)

# Aravis for camera support
pkg_check_modules(ARAVIS REQUIRED aravis-0.8)

# GStreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

# JSON library (nlohmann_json)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# spdlog for logging
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# c-periphery for GPIO (same as python-periphery used by Flask backend)
FetchContent_Declare(
    c_periphery
    GIT_REPOSITORY https://github.com/vsergeev/c-periphery.git
    GIT_TAG v2.4.1
)
FetchContent_MakeAvailable(c_periphery)

# OpenSSL for SSL/TLS support
find_package(OpenSSL REQUIRED)

# zlib for compression
find_package(ZLIB REQUIRED)

# Source files - all implementations enabled
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/config/app_config.cpp
    src/database/db_manager.cpp
    src/camera/aravis_camera.cpp
    src/camera/camera_manager.cpp
    src/gpio/gpio_controller.cpp
    src/triton/triton_client.cpp
    src/endpoints/endpoints.cpp
    src/endpoints/workflow_endpoint.cpp
    src/endpoints/camera_endpoint.cpp
    src/endpoints/model_endpoint.cpp
    src/utils/utils.cpp
)

# Optional EdgeML SDK integration
set(EDGEML_SDK_PATH "" CACHE PATH "Path to EdgeML SDK installation")
if(EXISTS "${EDGEML_SDK_PATH}/src/include/Panorama")
    message(STATUS "Found EdgeML SDK at ${EDGEML_SDK_PATH}")
    set(HAVE_EDGEML_SDK ON)
    set(EDGEML_INCLUDE_DIR "${EDGEML_SDK_PATH}/src/include")
else()
    message(STATUS "EdgeML SDK not found, building without Triton integration")
    set(HAVE_EDGEML_SDK OFF)
endif()

# Check for system-wide Panorama SDK installation
if(NOT HAVE_EDGEML_SDK)
    find_path(PANORAMA_INCLUDE_DIR Panorama/mlops.h
        PATHS /usr/local/include /usr/include
    )
    if(PANORAMA_INCLUDE_DIR)
        message(STATUS "Found system Panorama SDK at ${PANORAMA_INCLUDE_DIR}")
        set(HAVE_EDGEML_SDK ON)
        set(EDGEML_INCLUDE_DIR "${PANORAMA_INCLUDE_DIR}")
    endif()
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ARAVIS_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_APP_INCLUDE_DIRS}
    ${c_periphery_SOURCE_DIR}/src
)

# EdgeML SDK include directories (if available)
if(HAVE_EDGEML_SDK)
    target_include_directories(${PROJECT_NAME} PRIVATE ${EDGEML_INCLUDE_DIR})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_EDGEML_SDK)
    
    # Find and link all required Panorama SDK libraries
    # Order matters: core first (base symbols), then triton, then app
    find_library(PANORAMA_CORE_LIB panorama.core PATHS /usr/local/lib /usr/lib)
    find_library(PANORAMA_TRITON_LIB panorama.triton PATHS /usr/local/lib /usr/lib)
    find_library(PANORAMA_APP_LIB panorama.app PATHS /usr/local/lib /usr/lib)
    
    if(PANORAMA_CORE_LIB)
        message(STATUS "Found Panorama core library: ${PANORAMA_CORE_LIB}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PANORAMA_CORE_LIB})
    else()
        message(WARNING "Panorama core library not found")
    endif()
    
    if(PANORAMA_TRITON_LIB)
        message(STATUS "Found Panorama triton library: ${PANORAMA_TRITON_LIB}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PANORAMA_TRITON_LIB})
    else()
        message(WARNING "Panorama triton library not found")
    endif()
    
    if(PANORAMA_APP_LIB)
        message(STATUS "Found Panorama app library: ${PANORAMA_APP_LIB}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PANORAMA_APP_LIB})
    else()
        message(WARNING "Panorama app library not found")
    endif()
    
    # Link Triton Server library (required by panorama.triton)
    find_library(TRITONSERVER_LIB tritonserver
        PATHS /usr/local/lib /usr/lib /dependencies/server/build/tritonserver/install/lib/stubs
    )
    if(TRITONSERVER_LIB)
        message(STATUS "Found Triton Server library: ${TRITONSERVER_LIB}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${TRITONSERVER_LIB})
    else()
        message(WARNING "Triton Server library not found")
    endif()
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Crow::Crow
    Threads::Threads
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    ${ARAVIS_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    periphery
)

# Link directories
target_link_directories(${PROJECT_NAME} PRIVATE
    ${ARAVIS_LIBRARY_DIRS}
    ${GSTREAMER_LIBRARY_DIRS}
    ${GSTREAMER_APP_LIBRARY_DIRS}
)

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CROW_ENABLE_SSL
    CROW_ENABLE_COMPRESSION
)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
