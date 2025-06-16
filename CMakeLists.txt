cmake_minimum_required(VERSION 3.10)
project(wayru-os-services C)

# Definitions
add_definitions(-Wall)
if(CMAKE_C_COMPILER_VERSION VERSION_GREATER 6)
    add_definitions(-Wextra)
endif()
add_definitions(-Os -std=gnu99 -g3 -Wmissing-declarations -Wno-unused-parameter)

# Custom ubus paths via environment variables or cmake variables
set(ubus_root "" CACHE PATH "Root directory for ubus libraries")
if(ubus_root)
    list(APPEND cmake_prefix_path ${ubus_root})
endif()

# Find include directories
find_path(ubus_include_dir names libubus.h)
if(ubus_include_dir)
    include_directories(${ubus_include_dir})
endif()

# Find required libraries
find_library(ubus_library names ubus REQUIRED)
find_library(ubox_library names ubox REQUIRED)
find_library(blobmsg_json_library names blobmsg_json REQUIRED)
find_library(curl_library names curl REQUIRED)
find_library(json_c_library names json-c REQUIRED)
find_library(ssl_library names ssl REQUIRED)
find_library(crypto_library names crypto REQUIRED)
find_library(mosquitto_library names mosquitto REQUIRED)
find_library(lua_library names lua REQUIRED)

# Create granular static libraries by functionality

# Core utilities (no external deps)
add_library(wayru-core STATIC
    lib/core/console.c
    lib/core/result.c
    lib/core/retry.c
    lib/core/script_runner.c
    lib/core/stats.c
    lib/core/scheduler.c
    lib/core/uloop_scheduler.c
)
target_include_directories(wayru-core PUBLIC
    lib/core
    lib
)

# HTTP/networking utilities
add_library(wayru-http STATIC
    lib/http/curl_helpers.c
    lib/http/http-requests.c
)
target_include_directories(wayru-http PUBLIC
    lib/http
    lib
)
target_link_libraries(wayru-http
    PUBLIC
    wayru-core
    ${curl_library}
)

# Cryptography utilities
add_library(wayru-crypto STATIC
    lib/crypto/cert_audit.c
    lib/crypto/csr.c
    lib/crypto/key_pair.c
)
target_include_directories(wayru-crypto PUBLIC
    lib/crypto
    lib
)
target_link_libraries(wayru-crypto
    PUBLIC
    wayru-core
    ${ssl_library}
    ${crypto_library}
    ${json_c_library}
)

# Agent app - Full featured with all services
file(GLOB_RECURSE AGENT_SERVICE_SOURCES
    "apps/agent/services/*.c"
)
add_executable(agent
    apps/agent/main.c
    ${AGENT_SERVICE_SOURCES}
)
target_include_directories(agent PRIVATE
    apps/agent
    lib/core
    lib/http
    lib/crypto
    lib
)
target_link_libraries(agent
    PRIVATE
    wayru-core
    wayru-http
    wayru-crypto
    ${ubus_library}
    ${ubox_library}
    ${blobmsg_json_library}
    ${mosquitto_library}
    ${lua_library}
)

# Config app - Renderer and applier
add_executable(config
    apps/config/main.c
)
target_include_directories(config PRIVATE
    lib/core
    lib
)
target_link_libraries(config
    PRIVATE
    wayru-core
)

# Collector app - Log collection and forwarding
add_executable(collector
    apps/collector/main.c
    apps/collector/ubus.c
    apps/collector/collect.c
)
target_include_directories(collector PRIVATE
    apps/collector
    lib/core
    lib/http
    lib
)
target_link_libraries(collector
    PRIVATE
    wayru-core
    wayru-http
    ${ubus_library}
    ${ubox_library}
    ${blobmsg_json_library}
    ${json_c_library}
    ${curl_library}
)

# Install targets
install(TARGETS wayru-core wayru-http wayru-crypto ARCHIVE DESTINATION lib)
install(TARGETS agent config collector RUNTIME DESTINATION bin)

enable_testing()
