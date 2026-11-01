# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB app_sources ${CMAKE_SOURCE_DIR}/src/*.c)
list(APPEND app_sources ${CMAKE_SOURCE_DIR}/src/madgwick_filter_bridge.cpp)
list(APPEND app_sources ${CMAKE_SOURCE_DIR}/src/apex_logger.cpp)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "../inc"
    REQUIRES driver fatfs esp_event esp_netif esp_wifi nvs_flash esp_http_server mdns filters)
