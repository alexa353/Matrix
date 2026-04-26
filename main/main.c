#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"

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

static GlobalState GLOBAL_STATE;
static const char * TAG = "MATRIX_BITAXE";

// Matrix-Worker: Teilt den Suchraum auf
void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t my_start = id * step;
    uint32_t my_end = (id == 15) ? 0xFFFFFFFF : (my_start + step - 1);

    while (1) {
        // Nutze das Flag aus deiner global_state.h (ASIC_initalized)
        if (GLOBAL_STATE.ASIC_initalized && GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            // Direkter Hardware-Aufruf für den BM1370 Treiber
            BM1370_set_nonce_range(my_start, my_end);
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Matrix Miner (16 Units) startet...");

    // Basis-Initialisierung
    i2c_bitaxe_init();
    nvs_config_init();
    ADC_init();
    device_config_init(&GLOBAL_STATE);
    SYSTEM_init_system(&GLOBAL_STATE);
    display_init(&GLOBAL_STATE);
    wifi_init(&GLOBAL_STATE);

    // Hardware-Management
    xTaskCreate(POWER_MANAGEMENT_task, "power", 4096, (void *)&GLOBAL_STATE, 10, NULL);
    xTaskCreate(FAN_CONTROLLER_task, "fan", 4096, (void *)&GLOBAL_STATE, 5, NULL);

    // Warten auf Netzwerk
    while (!GLOBAL_STATE.SYSTEM_MODULE.is_connected) vTaskDelay(100 / portTICK_PERIOD_MS);

    // ASIC Start (nutzt deine asic.h Signatur)
    ASIC_init(&GLOBAL_STATE);

    // Mining & Share-Bündelung
    xTaskCreate(stratum_task, "stratum", 8192, (void *)&GLOBAL_STATE, 5, NULL);
    xTaskCreate(create_jobs_task, "miner", 8192, (void *)&GLOBAL_STATE, 20, NULL);
    xTaskCreate(ASIC_result_task, "result_bndl", 8192, (void *)&GLOBAL_STATE, 15, NULL);

    // Start der 16 Matrix-Einheiten
    for (int i = 0; i < 16; i++) {
        char tname[16];
        snprintf(tname, sizeof(tname), "Matx_%d", i);
        xTaskCreatePinnedToCore(matrix_worker, tname, 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    start_rest_server((void *)&GLOBAL_STATE);
}
