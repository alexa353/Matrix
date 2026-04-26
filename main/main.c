#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_psram.h"

// AxeOS 2.13.x Komponenten
#include "asic.h"
#include "asic_result_task.h"
#include "stratum_task.h"
#include "nvs_config.h"
#include "system.h"
#include "connect.h"
#include "i2c_bitaxe.h"
#include "adc.h"
#include "device_config.h"
#include "power_management_task.h"
#include "fan_controller_task.h"
#include "statistics_task.h"
#include "hashrate_monitor_task.h"
#include "create_jobs_task.h"
#include "http_server.h"

static GlobalState GLOBAL_STATE;
static const char * TAG = "bitaxe_matrix";

#define MATRIX_UNITS 16

/**
 * Matrix-Worker: Sucht im Einzel-Segment.
 */
void matrix_mining_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / MATRIX_UNITS;
    uint32_t my_start = id * step;
    uint32_t my_end = (id == MATRIX_UNITS - 1) ? 0xFFFFFFFF : (my_start + step - 1);

    while (1) {
        if (GLOBAL_STATE.ASIC_MODULE.is_initialized && GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            // Jede Matrix setzt ihren Suchbereich im Chip
            asic_set_nonce_range(my_start, my_end);
            
            // Logge die Aktivität der Einzel-Matrix (optional)
            if (id == 0) ESP_LOGD("MATRIX", "Matrix-Einheiten scannen parallel...");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Bitaxe Matrix Edition - Initialisierung...");

    // Basis-Setup (I2C, NVS, Power)
    i2c_bitaxe_init();
    nvs_config_init();
    ADC_init();
    device_config_init(&GLOBAL_STATE);
    SYSTEM_init_system(&GLOBAL_STATE);
    wifi_init(&GLOBAL_STATE);

    // Hardware-Tasks (Lüfter, Strom)
    xTaskCreate(POWER_MANAGEMENT_task, "power", 4096, (void *)&GLOBAL_STATE, 10, NULL);
    xTaskCreate(FAN_CONTROLLER_task, "fan", 4096, (void *)&GLOBAL_STATE, 5, NULL);

    // Warten auf Verbindung
    while (!GLOBAL_STATE.SYSTEM_MODULE.is_connected) vTaskDelay(100 / portTICK_PERIOD_MS);

    // ASIC Start
    asic_initialize(&GLOBAL_STATE, ASIC_INIT_COLD_BOOT, 0);

    // Zentrales Management (Bündelt die Shares aller Matrizen)
    xTaskCreate(stratum_task, "stratum", 8192, (void *)&GLOBAL_STATE, 5, NULL);
    xTaskCreate(create_jobs_task, "miner", 8192, (void *)&GLOBAL_STATE, 20, NULL);
    
    // WICHTIG: Sammelt alle Nonces ein und sendet gebündelte Shares
    xTaskCreate(ASIC_result_task, "result_collector", 8192, (void *)&GLOBAL_STATE, 15, NULL);

    // Start der 16 Matrix-Worker
    for (int i = 0; i < MATRIX_UNITS; i++) {
        char tname[16];
        snprintf(tname, sizeof(tname), "Matx_%d", i);
        xTaskCreatePinnedToCore(matrix_mining_worker, tname, 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    start_rest_server((void *)&GLOBAL_STATE);
    ESP_LOGI(TAG, "Matrix-System online. 16 Einheiten bündeln Shares.");
}
