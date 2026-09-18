cmake_minimum_required(VERSION 3.20)
project(apex_dash_emulator C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Provide LVGL config before including it
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
add_definitions(-DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG release/v9.2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(lvgl)

# Remove any assembly (.S) files from lvgl target when building on desktop
get_target_property(LVGL_SRCS lvgl SOURCES)
list(FILTER LVGL_SRCS EXCLUDE REGEX "\\.S$")
set_target_properties(lvgl PROPERTIES SOURCES "${LVGL_SRCS}")

find_package(SDL2 REQUIRED)

set(SHARED_UI_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apex-dash/src/ui/ui_theme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apex-dash/src/ui/ui_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apex-dash/src/ui/page_live_race.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apex-dash/src/ui/page_shumacher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apex-dash/src/ui/page_telemetry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apex-dash/src/ui/page_gps_paddock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apex-dash/src/ui/page_data_recall.cpp
)

add_executable(apex_emulator_native
    src/main_sdl.cpp
    ${SHARED_UI_SRCS}
)

target_include_directories(apex_emulator_native PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../apex-dash/include
)

target_link_libraries(apex_emulator_native PRIVATE
    lvgl
    ${SDL2_LIBRARIES}
    m
    pthread
)
