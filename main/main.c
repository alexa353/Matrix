#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Wir deklarieren die Funktionen nur, damit der Compiler sie kennt
extern void SYSTEM_init_system(void *gs);
extern void bm1370_set_nonce_range(uint32_t min, uint32_t max);

// Wir nutzen einen neutralen Pointer
static void* matrix_gs = NULL;

void matrix_worker(void *pvParameters) {
    uint32_t id = (uint32_t)(uintptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        bm1370_set_nonce_range(start, end);
        vTaskDelay(pdMS_TO_TICKS(550));
    }
}

// Wir markieren app_main als "weak", falls AxeOS sie woanders auch definiert hat
void __attribute__((weak)) app_main(void) {
    // 1. Matrix-Tasks starten
    for (uint32_t i = 0; i < 16; i++) {
        xTaskCreate(matrix_worker, "Mx", 2048, (void*)(uintptr_t)i, 2, NULL);
    }

    // 2. Das Hauptsystem von AxeOS rufen
    SYSTEM_init_system(matrix_gs);
}
