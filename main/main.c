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
static const char * TAG = "MATRIX_OS";

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
            vTaskDelay(pdMS_TO_TICKS(550)); 
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret==ESP_ERR_NVS_NO_FREE_PAGES||ret==ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    i2c_bitaxe_init();
    ADC_init();
    nvs_config_init();
    device_config_init(&GLOBAL_STATE);
    SYSTEM_init_system(&GLOBAL_STATE);
    display_init(&GLOBAL_STATE);
    wifi_init(&GLOBAL_STATE);
    ASIC_init(&GLOBAL_STATE);
    xTaskCreate(POWER_MANAGEMENT_task,"power",4096,&GLOBAL_STATE,10,NULL);
    xTaskCreate(FAN_CONTROLLER_task,"fan",4096,&GLOBAL_STATE,5,NULL);
    xTaskCreate(stratum_task,"stratum",8192,&GLOBAL_STATE,5,NULL);
    xTaskCreate(create_jobs_task,"miner",8192,&GLOBAL_STATE,20,NULL);
    xTaskCreate(ASIC_result_task,"collector",8192,&GLOBAL_STATE,15,NULL);
    xTaskCreate(hashrate_monitor_task,"hash_mon",4096,&GLOBAL_STATE,5,NULL);
    xTaskCreate(statistics_task,"stats",4096,&GLOBAL_STATE,3,NULL);
    for (int i=0; i<16; i++) {
        xTaskCreatePinnedToCore(matrix_worker,"Matrix",3072,(void*)(intptr_t)i,2,NULL,i%2);
    }
    start_rest_server((void*)&GLOBAL_STATE);
}
