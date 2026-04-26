#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"

#include "asic.h"
#include "bm1370.h" // Wichtig für BM1370_set_nonce_range
#include "asic_result_task.h"
#include "stratum_task.h"
#include "nvs_config.h"
#include "system.h"
#include "connect.h"
#include "i2c_bitaxe.h"
#include "adc.h"
#include "device_config.h"
#include "fan_controller_task.h"
#include "statistics_task.h"
#include "hashrate_monitor_task.h"
#include "create_jobs_task.h"
#include "http_server.h"
#include "power_management_task.h"

static GlobalState GLOBAL_STATE;
static const char * TAG = "bitaxe_matrix";

#define MATRIX_UNITS 16

void matrix_mining_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / MATRIX_UNITS;
    uint32_t my_start = id * step;
    uint32_t my_end = (id == MATRIX_UNITS - 1) ? 0xFFFFFFFF : (my_start + step - 1);

    while (1) {
        // Wir prüfen nur die Verbindung, um den ASIC nicht zu überlasten
        if (GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            // Direkter Aufruf des Hardware-Treibers für die Matrix-Range
            BM1370_set_nonce_range(my_start, my_end);
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Bitaxe Matrix Edition - Start");

    i2c_bitaxe_init();
    nvs_config_init();
    ADC_init();
    device_config_init(&GLOBAL_STATE);
    SYSTEM_init_system(&GLOBAL_STATE);
    wifi_init(&GLOBAL_STATE);

    xTaskCreate(POWER_MANAGEMENT_task, "power", 4096, (void *)&GLOBAL_STATE, 10, NULL);
    xTaskCreate(FAN_CONTROLLER_task, "fan", 4096, (void *)&GLOBAL_STATE, 5, NULL);

    while (!GLOBAL_STATE.SYSTEM_MODULE.is_connected) vTaskDelay(100 / portTICK_PERIOD_MS);

    // Initialisierung laut asic.h
    ASIC_init(&GLOBAL_STATE);

    xTaskCreate(stratum_task, "stratum", 8192, (void *)&GLOBAL_STATE, 5, NULL);
    xTaskCreate(create_jobs_task, "miner", 8192, (void *)&GLOBAL_STATE, 20, NULL);
    xTaskCreate(ASIC_result_task, "res_coll", 8192, (void *)&GLOBAL_STATE, 15, NULL);

    for (int i = 0; i < MATRIX_UNITS; i++) {
        char tname[16];
        snprintf(tname, sizeof(tname), "Matx_%d", i);
        xTaskCreatePinnedToCore(matrix_mining_worker, tname, 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    start_rest_server((void *)&GLOBAL_STATE);
}
