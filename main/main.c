#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Wir greifen auf die vorhandene Hardware-Steuerung zu
extern void bm1370_set_nonce_range(uint32_t min, uint32_t max);

// Die Matrix-Logik
void matrix_logic_worker(void *pvParameters) {
    uintptr_t id = (uintptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = (uint32_t)id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // Direkter Hardware-Befehl an den ASIC
        bm1370_set_nonce_range(start, end);
        vTaskDelay(pdMS_TO_TICKS(550)); 
    }
}

// Dieser Trick sorgt dafür, dass deine 16 Tasks gestartet werden,
// noch bevor AxeOS seine eigene app_main ausführt.
void __attribute__((constructor)) register_matrix_tasks(void) {
    for (uintptr_t i = 0; i < 16; i++) {
        xTaskCreate(matrix_logic_worker, "Mx", 3072, (void*)i, 2, NULL);
    }
}

// Wir lassen die app_main hier KOMPLETT weg, damit der Linker 
// die app_main aus deiner 'system.c' oder 'connect.c' nimmt.
