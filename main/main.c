#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

// AxeOS v2.13.x Komponenten
#include "asic.h"
#include "bm1370.h"
#include "system.h"

// Wir nutzen die globale Instanz, die AxeOS in system.c bereitstellt
extern GlobalState GLOBAL_STATE;

// Linker-Brücken für die Hardware-Ansteuerung
void asic_set_nonce_range(uint32_t min, uint32_t max) {
    bm1370_set_nonce_range(min, max);
}

// Die Matrix-Logik: 16 Worker teilen sich den Nonce-Bereich
void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t my_start = id * step;
    uint32_t my_end = (id == 15) ? 0xFFFFFFFF : (my_start + step - 1);

    // Warten bis ASIC bereit ist (AxeOS v2.13 Schreibweise)
    while(!GLOBAL_STATE.ASIC_initalized) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI("MATRIX", "Einheit %d aktiv (0x%08X - 0x%08X)", id, (unsigned int)my_start, (unsigned int)my_end);

    while (1) {
        if (GLOBAL_STATE.ASIC_initalized && GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            asic_set_nonce_range(my_start, my_end);
            vTaskDelay(pdMS_TO_TICKS(550)); // Deine gewählten 550ms
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void app_main(void) {
    // 1. NVS Initialisierung (Wichtig für WiFi und Settings)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Start der 16 Matrix-Worker
    for (int i = 0; i < 16; i++) {
        xTaskCreatePinnedToCore(matrix_worker, "Matrix", 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    // 3. Übergabe an das Hauptsystem von AxeOS
    // Initialisiert Display, I2C, ADC, WLAN und Webserver
    SYSTEM_init_system(&GLOBAL_STATE);
    
    ESP_LOGI("MATRIX", "Matrix-System (16 Kerne) erfolgreich gestartet.");
}
