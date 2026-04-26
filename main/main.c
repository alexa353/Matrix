#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Wir greifen auf die AxeOS-Funktionen zu
extern void bm1370_set_nonce_range(uint32_t min, uint32_t max);

// Wir erstellen eine Funktion, die KEINEN Namen-Konflikt hat
void matrix_logic_start(void *pvParameters) {
    uintptr_t id = (uintptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = (uint32_t)id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // Direkter Hardware-Befehl
        bm1370_set_nonce_range(start, end);
        vTaskDelay(pdMS_TO_TICKS(550)); 
    }
}

// Wir nutzen diesen Trick: Der Compiler führt dies beim Starten aus
void __attribute__((constructor)) register_matrix(void) {
    for (uintptr_t i = 0; i < 16; i++) {
        xTaskCreate(matrix_logic_start, "Mx", 3072, (void*)i, 2, NULL);
    }
}

// KEINE app_main hier! Die kommt aus der system.c von AxeOS.
