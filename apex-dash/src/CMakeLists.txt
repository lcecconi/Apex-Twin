idf_component_register(
    SRCS "main.cpp"
         "display_st7305.cpp"
         "apex_led_strip.cpp"
         "esp_now_transport.cpp"
         "ui/ui_theme.cpp"
         "ui/ui_manager.cpp"
         "ui/page_live_race.cpp"
         "ui/page_shumacher.cpp"
         "ui/page_telemetry.cpp"
         "ui/page_gps_paddock.cpp"
         "ui/page_data_recall.cpp"
    INCLUDE_DIRS "." "../include"
    REQUIRES lvgl led_strip esp_wifi esp_event nvs_flash driver esp_timer esp_netif
)
