#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Direkte Deklaration der Hardware-Funktionen (keine Header-Dateien nötig!)
extern void bm1370_set_nonce_range(uint32_t min, uint32_t max);
extern void SYSTEM_init_system(void *gs);

// Ein leerer Speicherblock für das System
uint32_t dummy_gs[256]; 

void matrix_worker(void *pvParameters) {
    uint32_t id = (uint32_t)(uintptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // Wir ballern den Befehl direkt in den Chip
        bm1370_set_nonce_range(start, end);
        vTaskDelay(pdMS_TO_TICKS(550)); 
    }
}

void app_main(void) {
    // 1. Matrix-Tasks starten
    for (uint32_t i = 0; i < 16; i++) {
        xTaskCreate(matrix_worker, "Mx", 2048, (void*)(uintptr_t)i, 2, NULL);
    }

    // 2. Den AxeOS-Rest starten
    SYSTEM_init_system(dummy_gs);
    
    printf("MATRIX-CORE: 16 Einheiten aktiv.\n");
}
