#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Wir deklarieren nur das Nötigste extern
extern void SYSTEM_init_system(void *gs);
extern void bm1370_set_nonce_range(uint32_t min, uint32_t max);

// Ein Puffer für den System-Status, ohne den komplexen Header zu brauchen
static uint8_t dummy_gs[4096]; 

void matrix_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // Wir setzen die Range einfach dauerhaft (Hardware-Ebene)
        bm1370_set_nonce_range(start, end);
        vTaskDelay(pdMS_TO_TICKS(550)); 
    }
}

void app_main(void) {
    // 1. Basis-Init
    nvs_flash_init();
    
    // 2. Start der 16 Matrix-Einheiten
    for (int i = 0; i < 16; i++) {
        xTaskCreatePinnedToCore(matrix_worker, "Mx", 2048, (void *)(intptr_t)i, 2, NULL, i % 2);
    }

    // 3. System-Start (Wir geben einen leeren Puffer mit, falls die Struktur crasht)
    // Das sorgt dafür, dass der Rest vom AxeOS (Webserver etc.) trotzdem startet
    SYSTEM_init_system(&dummy_gs);
    
    printf("MATRIX: 16 Einheiten aktiv. System-Handover abgeschlossen.\n");
}
