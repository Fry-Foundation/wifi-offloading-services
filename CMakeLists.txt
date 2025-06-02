cmake_minimum_required(VERSION 3.10)

PROJECT(wayru-os-services C)

ADD_DEFINITIONS(-Wall)
IF(CMAKE_C_COMPILER_VERSION VERSION_GREATER 6)
    ADD_DEFINITIONS(-Wextra)
ENDIF()
ADD_DEFINITIONS(-Os -std=gnu99 -g3 -Wmissing-declarations -Wno-unused-parameter)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# Source files
SET(SERVICES_SOURCES
    source/services/config/config.c
    source/services/config/uci_parser.c
    source/services/config/defaults.c
    source/services/device_info.c
    source/services/device_status.c
    source/services/did-key.c
    source/services/end_report.c
    source/services/mqtt/cert.c
    source/services/mqtt/mqtt.c
    source/services/monitoring.c
    source/services/speedtest.c
    source/services/registration.c
    source/services/access_token.c
    source/services/firmware_upgrade.c
    source/services/package_update.c
    source/services/device-context.c
    source/services/nds.c
    source/services/site-clients.c
    source/services/commands.c
    source/services/gen_id.c
    source/services/reboot.c
    source/services/radsec_cert.c
    source/services/diagnostic/diagnostic.c
    source/services/exit_handler.c
    source/services/time_sync.c
    source/services/collector.c
)

SET(LIB_SOURCES
    source/lib/console.c
    source/lib/curl_helpers.c
    source/lib/key_pair.c
    source/lib/http-requests.c
    source/lib/scheduler.c
    source/lib/script_runner.c
    source/lib/cert_audit.c
    source/lib/retry.c
    source/lib/result.c
    source/lib/csr.c
)

SET(SOURCES
    source/main.c
    ${SERVICES_SOURCES}
    ${LIB_SOURCES}
)

# Find required libraries
FIND_LIBRARY(CURL_LIBRARY NAMES curl)
FIND_LIBRARY(JSON_C_LIBRARY NAMES json-c)
FIND_LIBRARY(SSL_LIBRARY NAMES ssl)
FIND_LIBRARY(CRYPTO_LIBRARY NAMES crypto)
FIND_LIBRARY(MOSQUITTO_LIBRARY NAMES mosquitto)

# Include directories
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/source)

# Create executable
ADD_EXECUTABLE(wayru-os-services ${SOURCES})

# Link libraries
TARGET_LINK_LIBRARIES(wayru-os-services
    ${CURL_LIBRARY}
    ${JSON_C_LIBRARY}
    ${SSL_LIBRARY}
    ${CRYPTO_LIBRARY}
    ${MOSQUITTO_LIBRARY}
)

# Installation
INSTALL(TARGETS wayru-os-services
    RUNTIME DESTINATION bin
) 