#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "asic.h"
#include "bm1370.h"
#include "nvs_config.h"
#include "system.h"
#include "connect.h"
#include "i2c_bitaxe.h"
#include "adc.h"
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

void asic_set_nonce_range(uint32_t min, uint32_t max) {
    bm1370_set_nonce_range(min, max);
}

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
            asic_set_nonce_range(start, end);
        }
        vTaskDelay(pdMS_TO_TICKS(550));
    }
}

void app_main(void) {
    nvs_flash_init();
    i2c_bitaxe_init();
    ADC_init();
    nvs_config_init();
    device_config_init(&GLOBAL_STATE);
    SYSTEM_init_system(&GLOBAL_STATE);
    display_init(&GLOBAL_STATE);
    wifi_init(&GLOBAL_STATE);
    ASIC_init(&GLOBAL_STATE);
    xTaskCreate(POWER_MANAGEMENT_task, "p", 4096, &GLOBAL_STATE, 10, NULL);
    xTaskCreate(FAN_CONTROLLER_task, "f", 4096, &GLOBAL_STATE, 5, NULL);
    xTaskCreate(stratum_task, "s", 8192, &GLOBAL_STATE, 5, NULL);
    xTaskCreate(create_jobs_task, "m", 8192, &GLOBAL_STATE, 20, NULL);
    xTaskCreate(ASIC_result_task, "c", 8192, &GLOBAL_STATE, 15, NULL);
    for (int i = 0; i < 16; i++) {
        xTaskCreatePinnedToCore(matrix_worker, "Mx", 3072, (void*)(intptr_t)i, 2, NULL, i % 2);
    }
    start_rest_server((void*)&GLOBAL_STATE);
}
