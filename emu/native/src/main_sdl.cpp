/**
 * @file main_sdl.cpp
 * Native SDL2 Desktop Emulator runner for Apex-Dash (LVGL v9)
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <cmath>
#include <vector>
#include <unistd.h>

#include <lvgl.h>
#include <SDL2/SDL.h>

#include "ui/ui_manager.h"
#include "telemetry_data.h"

using namespace ApexUi;

static TelemetrySnapshot g_telemetry;
static SystemSettings    g_settings;
static std::vector<LapRecord> g_lap_history;

static void init_sim_state() {
    g_settings.drive_type = DRIVE_SHIFTER_6SPEED;
    g_settings.max_rpm = 16000;
    g_settings.shift_rpm = 14200;
    g_settings.water_temp_alarm_c = 65.0f;
    g_settings.exhaust_temp_alarm_c = 640.0f;

    g_telemetry.chassis.rpm = 8500;
    g_telemetry.chassis.speed_kmh = 78.0f;
    g_telemetry.chassis.gear = 3;
    g_telemetry.chassis.current_sector = 1;
    g_telemetry.chassis.total_sectors = 3;
    g_telemetry.chassis.lap_number = 2;
    g_telemetry.chassis.current_lap_time_ms = 18450;
    g_telemetry.chassis.last_lap_time_ms = 48680;
    g_telemetry.chassis.best_lap_time_ms = 48420;
    g_telemetry.chassis.predictive_delta_s = -0.18f;
    g_telemetry.chassis.water_temp_c = 58.4f;
    g_telemetry.chassis.exhaust_temp_c = 580.0f;
    g_telemetry.chassis.connected = true;
    g_telemetry.chassis.link_rssi = -64;
    g_telemetry.chassis.satellites_visible = 18;
    g_telemetry.chassis.gps_fix = 3;
    g_telemetry.chassis.hdop = 0.85f;
    g_telemetry.chassis.engine_total_hours_sec = 14 * 3600 + 25 * 60;
    g_telemetry.chassis.piston_hours_sec = 2 * 3600 + 15 * 60;

    g_telemetry.local.battery_percent = 98;
    g_telemetry.local.battery_voltage = 4.12f;
    g_telemetry.local.ambient_temp_c = 24.5f;
    g_telemetry.local.ambient_humidity_pct = 52.0f;
    strncpy(g_telemetry.local.current_track_name, "South Garda (Lonato)", sizeof(g_telemetry.local.current_track_name) - 1);

    g_telemetry.syncFlatFields();

    // Sample top 3 laps
    LapRecord r1;
    r1.lap_number = 2;
    r1.lap_time_ms = 48420;
    r1.sector_count = 3;
    r1.sector_times_ms[0] = 16080;
    r1.sector_times_ms[1] = 16150;
    r1.sector_times_ms[2] = 16190;
    r1.max_speed_kmh = 125.1f;
    r1.max_rpm = 15850;
    g_lap_history.push_back(r1);

    LapRecord r2;
    r2.lap_number = 1;
    r2.lap_time_ms = 48680;
    r2.sector_count = 3;
    r2.sector_times_ms[0] = 16180;
    r2.sector_times_ms[1] = 16300;
    r2.sector_times_ms[2] = 16200;
    r2.max_speed_kmh = 122.4f;
    r2.max_rpm = 15600;
    g_lap_history.push_back(r2);

    LapRecord r3;
    r3.lap_number = 3;
    r3.lap_time_ms = 48750;
    r3.sector_count = 3;
    r3.sector_times_ms[0] = 16220;
    r3.sector_times_ms[1] = 16290;
    r3.sector_times_ms[2] = 16240;
    r3.max_speed_kmh = 121.8f;
    r3.max_rpm = 15500;
    g_lap_history.push_back(r3);
}

static void update_sim_physics(float dt_s) {
    static float s_time_s = 18.45f;
    const float LAP_DURATION = 48.42f;

    s_time_s += dt_s;
    if (s_time_s >= LAP_DURATION) {
        s_time_s = 0.0f;
        g_telemetry.chassis.lap_number++;
    }

    g_telemetry.chassis.current_lap_time_ms = (uint32_t)(s_time_s * 1000.0f);

    // Dynamic 3-sector calculation
    float sec_len = LAP_DURATION / (float)g_telemetry.chassis.total_sectors;
    g_telemetry.chassis.current_sector = (uint8_t)(s_time_s / sec_len) + 1;
    if (g_telemetry.chassis.current_sector > g_telemetry.chassis.total_sectors) {
        g_telemetry.chassis.current_sector = g_telemetry.chassis.total_sectors;
    }

    // Lonato simulation curve
    float progress = s_time_s / LAP_DURATION;
    float omega = 2.0f * M_PI * progress;

    bool is_braking = std::sin(omega * 4.0f) < -0.45f;
    bool is_cornering = std::abs(std::sin(omega * 3.0f)) > 0.5f;

    if (is_braking) {
        g_telemetry.chassis.longitudinal_g = -1.65f - 0.2f * std::sin(omega * 10.0f);
        g_telemetry.chassis.speed_kmh = std::max(44.0f, g_telemetry.chassis.speed_kmh - 80.0f * dt_s);
        g_telemetry.chassis.gear = std::max(2, (int)(g_telemetry.chassis.speed_kmh / 22.0f));
        g_telemetry.chassis.rpm = (uint16_t)(6000.0f + g_telemetry.chassis.speed_kmh * 72.0f);
        g_telemetry.chassis.lateral_g = 0.25f * std::sin(omega * 5.0f);
    } else if (is_cornering) {
        g_telemetry.chassis.longitudinal_g = 0.15f;
        g_telemetry.chassis.speed_kmh = 68.0f + 14.0f * std::sin(omega * 6.0f);
        g_telemetry.chassis.gear = 3;
        g_telemetry.chassis.rpm = (uint16_t)(8600.0f + 1800.0f * std::sin(omega * 8.0f));
        g_telemetry.chassis.lateral_g = 1.85f * std::sin(omega * 3.0f);
    } else {
        // Straight line acceleration
        g_telemetry.chassis.longitudinal_g = 0.85f;
        g_telemetry.chassis.speed_kmh = std::min(128.5f, g_telemetry.chassis.speed_kmh + 52.0f * dt_s);
        g_telemetry.chassis.gear = std::min(6, std::max(2, (int)(g_telemetry.chassis.speed_kmh / 21.0f)));
        g_telemetry.chassis.rpm = (uint16_t)(8000.0f + ((float)(g_telemetry.chassis.rpm % 3000)) + 65.0f * g_telemetry.chassis.speed_kmh);
        if (g_telemetry.chassis.rpm > 15850) g_telemetry.chassis.rpm = 15850;
        g_telemetry.chassis.lateral_g = 0.05f * std::sin(omega * 2.0f);
    }

    g_telemetry.chassis.predictive_delta_s = -0.22f + 0.15f * std::sin(omega * 1.5f);
    g_telemetry.syncFlatFields();
}

int main(int argc, char **argv) {
    bool smoke_test = (argc > 1 && strcmp(argv[1], "--smoke") == 0);

    printf("[Apex-Dash] Initializing LVGL v9 native SDL2 runner...\n");
    lv_init();

    lv_display_t *disp = lv_sdl_window_create(400, 240);
    if (!disp) {
        fprintf(stderr, "[Apex-Dash] Failed to create SDL2 window!\n");
        return 1;
    }
    lv_sdl_window_set_title(disp, "Apex-Dash | ST7305 RLCD Simulator (LVGL v9)");
    lv_sdl_window_set_zoom(disp, 2.0f); // 800 x 480 window

    init_sim_state();

    UiManager ui_mgr;
    ui_mgr.init(lv_screen_active(), true);

    printf("[Apex-Dash] UI Manager initialized. Available views: 5.\n");
    printf("[Apex-Dash] Controls:\n");
    printf("   [Down / Right] : Next View\n");
    printf("   [Up   / Left ] : Prev View\n");
    printf("   [1 .. 5]       : Switch directly to View 1-5\n");
    printf("   [Space]        : Trigger Finish Line / Reset Lap\n");
    printf("   [Esc]          : Quit\n");

    if (smoke_test) {
        printf("[Apex-Dash] Running headless smoke validation ticks...\n");
        for (int i = 0; i < 20; i++) {
            update_sim_physics(0.04f);
            ui_mgr.update(g_telemetry, g_settings, g_lap_history.data(), g_lap_history.size());
            lv_timer_handler();
        }
        printf("[Apex-Dash] Smoke validation passed successfully!\n");
        return 0;
    }

    auto last_tick = std::chrono::steady_clock::now();
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        running = false;
                        break;
                    case SDLK_RIGHT:
                    case SDLK_DOWN:
                        ui_mgr.nextView();
                        printf("[Apex-Dash] View changed to: %u\n", ui_mgr.getViewMode());
                        break;
                    case SDLK_LEFT:
                    case SDLK_UP:
                        ui_mgr.prevView();
                        printf("[Apex-Dash] View changed to: %u\n", ui_mgr.getViewMode());
                        break;
                    case SDLK_1:
                        ui_mgr.setViewMode(VIEW_LIVE_RACE);
                        break;
                    case SDLK_2:
                        ui_mgr.setViewMode(VIEW_SHUMACHER);
                        break;
                    case SDLK_3:
                        ui_mgr.setViewMode(VIEW_TELEMETRY);
                        break;
                    case SDLK_4:
                        ui_mgr.setViewMode(VIEW_GPS_PADDOCK);
                        break;
                    case SDLK_5:
                        ui_mgr.setViewMode(VIEW_DATA_RECALL);
                        break;
                    case SDLK_SPACE:
                        g_telemetry.chassis.lap_number++;
                        g_telemetry.chassis.current_lap_time_ms = 0;
                        printf("[Apex-Dash] Lap marked: L%02u\n", g_telemetry.chassis.lap_number);
                        break;
                    default:
                        break;
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        float dt_s = std::chrono::duration<float>(now - last_tick).count();
        if (dt_s >= 0.04f) { // ~25 Hz telemetry tick
            update_sim_physics(dt_s);
            ui_mgr.update(g_telemetry, g_settings, g_lap_history.data(), g_lap_history.size());
            last_tick = now;
        }

        uint32_t sleep_ms = lv_timer_handler();
        if (sleep_ms < 1) sleep_ms = 1;
        if (sleep_ms > 20) sleep_ms = 20;
        usleep(sleep_ms * 1000);
    }

    printf("[Apex-Dash] Terminating runner.\n");
    return 0;
}
