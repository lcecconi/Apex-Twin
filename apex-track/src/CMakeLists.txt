# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB app_sources ${CMAKE_SOURCE_DIR}/src/*.c)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "../inc"
    REQUIRES driver esp_event esp_netif esp_wifi nvs_flash esp_http_server mdns)
