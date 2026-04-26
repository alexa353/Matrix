#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_psram.h"

#include "asic.h"
#include "bm1370.h"
#include "nvs_config.h"
#include "system.h"
#include "connect.h"
#include "i2c_bitaxe.h"
#include "adc.h"
#include "device_config.h"
#include "display.h"
#include "power_management_task.h"
#include "fan_controller_task.h"
#include "stratum_task.h"
#include "create_jobs_task.h"
#include "asic_result_task.h"
#include "http_server.h"
#include "statistics_task.h"
#include "hashrate_monitor_task.h"

static GlobalState GLOBAL_STATE;
static const char * TAG = "MATRIX_FIX";

// Wir deklarieren die Initialisierung manuell, um den Linker-Fehler in power_management zu umgehen
uint8_t asic_initialize(GlobalState * gs, uint8_t mode, uint32_t val) {
    return ASIC_init(gs);
}

void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        if (GLOBAL_STATE.ASIC_initalized && GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            // Wir nutzen ASIC_read_registers, was wir in deiner asic.h gesehen haben, 
            // um den Chip-Status wach zu halten, während AxeOS die Jobs verteilt.
            ASIC_read_registers(&GLOBAL_STATE);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    if (esp_psram_is_initialized()) GLOBAL_STATE.psram_is_available = true;
    i2c_bitaxe_init();
    ADC_init();
    nvs_config_init();
    device_config_init(&GLOBAL_STATE);
    SYSTEM_init_system(&GLOBAL_STATE);
    display_init(&GLOBAL_STATE);
    wifi_init(&GLOBAL_STATE);

    xTaskCreate(POWER_MANAGEMENT_task, "power", 4096, (void *)&GLOBAL_STATE, 10, NULL);
    xTaskCreate(FAN_CONTROLLER_task, "fan", 4096, (void *)&GLOBAL_STATE, 5, NULL);

    while (!GLOBAL_STATE.SYSTEM_MODULE.is_connected) vTaskDelay(100 / portTICK_PERIOD_MS);

    ASIC_init(&GLOBAL_STATE);

    xTaskCreate(stratum_task, "stratum", 8192, (void *)&GLOBAL_STATE, 5, NULL);
    xTaskCreate(create_jobs_task, "miner", 8192, (void *)&GLOBAL_STATE, 20, NULL);
    xTaskCreate(ASIC_result_task, "collector", 8192, (void *)&GLOBAL_STATE, 15, NULL);
    
    xTaskCreateWithCaps(hashrate_monitor_task, "hash_mon", 4096, (void *)&GLOBAL_STATE, 5, NULL, MALLOC_CAP_SPIRAM);
    xTaskCreateWithCaps(statistics_task, "stats", 4096, (void *)&GLOBAL_STATE, 3, NULL, MALLOC_CAP_SPIRAM);

    for (int i = 0; i < 16; i++) {
        char tname[16];
        snprintf(tname, sizeof(tname), "Matx_%d", i);
        xTaskCreatePinnedToCore(matrix_worker, tname, 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }
    start_rest_server((void *)&GLOBAL_STATE);
}
