#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Diese Header kommen aus deinen Komponenten
#include "asic.h"
#include "bm1370.h"
#include "system.h"

// WICHTIG: Keine neue GLOBAL_STATE erstellen, sondern die aus system.c nutzen!
extern GlobalState GLOBAL_STATE;

// Diese Brücken-Funktionen werden von den Tasks in 'main/tasks' gesucht
void asic_set_nonce_range(uint32_t min, uint32_t max) {
    bm1370_set_nonce_range(min, max);
}

uint8_t asic_initialize(GlobalState * gs, uint8_t mode, uint32_t val) {
    return ASIC_init(gs);
}

// Die Matrix-Logik
void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // Nutze das Flag aus der vorhandenen GLOBAL_STATE
        if (GLOBAL_STATE.ASIC_initalized && GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            asic_set_nonce_range(start, end);
            vTaskDelay(pdMS_TO_TICKS(550)); 
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// Das ist der Einstiegspunkt, den der Compiler sucht
void app_main(void) {
    // 1. NVS muss für das System bereit sein
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 2. Wir starten NUR unsere Matrix-Tasks
    for (int i = 0; i < 16; i++) {
        xTaskCreatePinnedToCore(matrix_worker, "Matrix", 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    // 3. Wir überlassen AxeOS die restliche Hardware-Initialisierung
    // Das verhindert den Absturz, weil system.c das Display/WLAN/ASIC schon verwaltet
    SYSTEM_init_system(&GLOBAL_STATE);
}
