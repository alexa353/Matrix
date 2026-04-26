#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Wir deklarieren nur, was wir wirklich zum Überleben brauchen
// Das verhindert Konflikte mit den vielen Dateien in der CMakeLists
extern void SYSTEM_init_system(void *gs);
extern void bm1370_set_nonce_range(uint32_t min, uint32_t max);

// Ein Speicherbereich für das System (AxeOS nutzt diesen global)
uint8_t system_storage[2048]; 

void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // Direkter Hardware-Befehl
        bm1370_set_nonce_range(start, end);
        vTaskDelay(pdMS_TO_TICKS(550)); 
    }
}

void app_main(void) {
    // 1. Matrix-Einheiten sofort starten (bevor das System laden kann)
    for (int i = 0; i < 16; i++) {
        xTaskCreatePinnedToCore(matrix_worker, "Mx", 2048, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    // 2. AxeOS Haupt-Initialisierung aufrufen
    // Wir nutzen den Namen, den AxeOS intern für GLOBAL_STATE reserviert
    SYSTEM_init_system(&system_storage);

    printf("MATRIX-OS: 16 Einheiten online. System-Handover abgeschlossen.\n");
}
