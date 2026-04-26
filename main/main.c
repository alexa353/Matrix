#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "nvs_flash.h"

// AxeOS v2.13.x Komponenten
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
static const char * TAG = "MATRIX_OS";

// ====================================================================
// HARDWARE-LINKER-FIXES
// ====================================================================
void asic_set_nonce_range(uint32_t min, uint32_t max) {
    bm1370_set_nonce_range(min, max);
}

uint8_t asic_initialize(GlobalState * gs, uint8_t mode, uint32_t val) {
    return ASIC_init(gs);
}

// ====================================================================
// MATRIX LOGIK
// ====================================================================
void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t my_start = id * step;
    uint32_t my_end = (id == 15) ? 0xFFFFFFFF : (my_start + step - 1);

    while(!GLOBAL_STATE.ASIC_initalized) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI("MATRIX", "Einheit %d aktiv (0x%08X - 0x%08X)", id, (unsigned int)my_start, (unsigned int)my_end);

    while (1) {
        if (GLOBAL_STATE.ASIC_initalized && GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            asic_set_nonce_range(my_start, my_end);
            vTaskDelay(pdMS_TO_TICKS(550)); 
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void app_main(void) {
    // 1. NVS Initialisierung (WICHTIG für Boot)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (esp_psram_is_initialized()) {
        GLOBAL_STATE.psram_is_available = true;
    }

    // 2. Hardware Treiber
    i2c_bitaxe_init();
    ADC_init();
    device_config_init(&GLOBAL_STATE);
    SYSTEM_init_system(&GLOBAL_STATE);
    
    // 3. Display Start
    display_init(&GLOBAL_STATE);

    // 4. Netzwerk & Schutz
    wifi_init(&GLOBAL_STATE);
    xTaskCreate(POWER_MANAGEMENT_task, "power", 4096, (void *)&GLOBAL_STATE, 10, NULL);
    xTaskCreate(FAN_CONTROLLER_task, "fan", 4096, (void *)&GLOBAL_STATE, 5, NULL);

    // 5. ASIC & Mining
    ASIC_init(&GLOBAL_STATE);
    xTaskCreate(stratum_task, "stratum", 8192, (void *)&GLOBAL_STATE, 5, NULL);
    xTaskCreate(create_jobs_task, "miner", 8192, (void *)&GLOBAL_STATE, 20, NULL);
    xTaskCreate(ASIC_result_task, "collector", 8192, (void *)&GLOBAL_STATE, 15, NULL);

    // 6. Matrix-Worker (FIX: tname ist jetzt ein Array)
    for (int i = 0; i < 16; i++) {
        char tname[16];
        snprintf(tname, sizeof(tname), "Matx_%d", i);
        xTaskCreatePinnedToCore(matrix_worker, tname, 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    start_rest_server((void *)&GLOBAL_STATE);
    ESP_LOGI(TAG, "System stabil. Matrix-Mining (16+1) aktiv.");
}
