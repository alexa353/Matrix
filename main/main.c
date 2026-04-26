#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// AxeOS v2.13 Komponenten
#include "asic.h"
#include "bm1370.h"
#include "system.h"

// Wir definieren die GLOBAL_STATE hier lokal, damit der Compiler nicht abbricht
static GlobalState GLOBAL_STATE;

// Direkte Hardware-Ansteuerung ohne Umwege
void asic_set_nonce_range(uint32_t min, uint32_t max) {
    bm1370_set_nonce_range(min, max);
}

void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // Wir prüfen nur, ob das System generell bereit ist
        if (GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
            asic_set_nonce_range(start, end);
            vTaskDelay(pdMS_TO_TICKS(550)); 
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void app_main(void) {
    // 1. NVS Speicher für WiFi-Daten vorbereiten
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 2. 16 Matrix-Einheiten starten
    for (int i = 0; i < 16; i++) {
        xTaskCreatePinnedToCore(matrix_worker, "Matrix", 3072, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    // 3. Das Hauptsystem von AxeOS initialisieren
    // Wir nutzen die lokale GLOBAL_STATE
    SYSTEM_init_system(&GLOBAL_STATE);
}
