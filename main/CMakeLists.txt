idf_component_register(
    SRCS
        "main.c"
        "system.c"
        "i2c_bitaxe.c"
        "nvs_config.c"
        "device_config.c"
        "adc.c"
        "display.c"
        "./tasks/stratum_task.c"
        "./tasks/create_jobs_task.c"
        "./tasks/asic_result_task.c"
        "./http_server/http_server.c"
        "./power/power.c"
    INCLUDE_DIRS
        "." "tasks" "power" "http_server"
        "../components/asic/include"
        "../components/stratum/include"
    REQUIRES
        "connect" "asic" "stratum" "esp_http_server" "nvs_flash"
        "esp_netif" "esp_event" "esp_psram" "esp_lcd"
    PRIV_REQUIRES
        "driver" "esp_adc" "esp_driver_i2c" "json" "spiffs"
    EMBED_FILES 
        "./http_server/recovery_page.html"
)

# Erzwinge den Bau der Binary
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=all" APPEND)
