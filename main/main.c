#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_psram.h"

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
static const char * TAG = "MATRIX_16_1";

/**
 * Matrix-Worker Task
 * Wechselt alle 100ms den Nonce-Bereich des ASICs, um 16 Einheiten 
 * quasi-parallel auf einem Chip rechnen zu lassen.
 */
void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t my_start = id * step;
    uint32_t my_end = (id == 15) ? 0xFFFFFFFF : (my_start + step - 1);

    while (1) {
        // Nutze das Flag ASIC_initalized aus deinem Header
        if (GLOBAL_STATE.ASIC_initalized && GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            
            // DIREKTER ZUGRIFF: Wir setzen die Range direkt im Job-Speicher.
            // Das ist der sicherste Weg in AxeOS 2.13.x
            if (GLOBAL_STATE.ASIC_TASK_MODULE.current_job != NULL) {
                GLOBAL_STATE.ASIC_TASK_MODULE.current_job->nonce_range_min = my_start;
                GLOBAL_STATE.ASIC_TASK_MODULE.current_job->nonce_range_max = my_end;
            }
            
            vTaskDelay(pdMS_TO_TICKS(100)); 
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}


void app_main(void) {
    ESP_LOGI(TAG, "Bitaxe Matrix Edition (16+1) startet...");

    // 1. PSRAM & I2C Basis
    if (esp_psram_is_initialized()) {
        GLOBAL_STATE.psram_is_available = true;
    }

    ESP_ERROR_CHECK(i2c_bitaxe_init());
    ADC_init();
    
    // 2. Konfiguration & System
    nvs_config_init();
    device_config_init(&GLOBAL_STATE);
    SYSTEM_init_system(&GLOBAL_STATE);
    display_init(&GLOBAL_STATE);
    wifi_init(&GLOBAL_STATE);

    // 3. Hardware-Schutz (Lüfter & Power)
    xTaskCreate(POWER_MANAGEMENT_task, "power", 4096, (void *)&GLOBAL_STATE, 10, NULL);
    xTaskCreate(FAN_CONTROLLER_task, "fan", 4096, (void *)&GLOBAL_STATE, 5, NULL);

    // Warten auf WiFi Verbindung
    while (!GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // 4. ASIC Initialisierung (laut deiner asic.h)
    ASIC_init(&GLOBAL_STATE);

    // 5. Mining Infrastruktur (Der 17. Share / Bündelung)
    xTaskCreate(stratum_task, "stratum", 8192, (void *)&GLOBAL_STATE, 5, NULL);
    xTaskCreate(create_jobs_task, "miner", 8192, (void *)&GLOBAL_STATE, 20, NULL);
    xTaskCreate(ASIC_result_task, "collector", 8192, (void *)&GLOBAL_STATE, 15, NULL);
    
    xTaskCreateWithCaps(hashrate_monitor_task, "hash_mon", 4096, (void *)&GLOBAL_STATE, 5, NULL, MALLOC_CAP_SPIRAM);
    xTaskCreateWithCaps(statistics_task, "stats", 4096, (void *)&GLOBAL_STATE, 3, NULL, MALLOC_CAP_SPIRAM);

    // 6. Start der 16 Matrix-Worker (Gleichverteilt auf Core 0 und 1)
    for (int i = 0; i < 16; i++) {
        char tname[16];
        snprintf(tname, sizeof(tname), "Matx_%d", i);
        xTaskCreatePinnedToCore(matrix_worker, tname, 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    // 7. Webinterface starten
    start_rest_server((void *)&GLOBAL_STATE);
    
    ESP_LOGI(TAG, "System stabil. Matrix-Mining aktiv.");
}
