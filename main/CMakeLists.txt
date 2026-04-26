idf_component_register(
    SRCS
        "adc.c"
        "i2c_bitaxe.c"
        "main.c"
        "nvs_config.c"
        "display.c"
        "screen.c"
        "input.c"
        "system.c"
        "work_queue.c"
        "lv_font_portfolio-6x8.c"
        "logo.c"
        "./http_server/http_server.c"
        "./http_server/theme_api.c"
        "./self_test/self_test.c"
        "./thermal/thermal.c"
        "./thermal/PID.c"
        "./power/power.c"
        "./power/vcore.c"
    INCLUDE_DIRS
        "."
        "tasks"
        "thermal"
        "power"
        "http_server"
        "../components/dns_server/include"
    REQUIRES
        "connect"
        "asic"
        "stratum"
        "esp_http_server"
        "nvs_flash"
        "esp_netif"
        "esp_event"
        "dns_server"
        "esp_psram"  # HIER KORRIGIERT: PSRAM Unterstützung hinzugefügt
        "esp_lcd"    # Für das Display-Modul
    PRIV_REQUIRES
        "driver"
        "esp_adc"
        "esp_driver_i2c"
        "json"
        "spiffs"
        "app_update"
)

# Pfad-Fix für LVGL
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_PATH=\"${CMAKE_SOURCE_DIR}/main/lv_conf.h\"" APPEND)
