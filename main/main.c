#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Wir greifen auf die AxeOS-Funktionen zu, ohne Header-Konflikte zu riskieren
extern void SYSTEM_init_system(void *gs);
extern void bm1370_set_nonce_range(uint32_t min, uint32_t max);

// Wir nutzen einen neutralen Pointer, damit der Compiler nicht über die GlobalState-Struktur stolpert
void* global_gs_ptr = NULL;

void matrix_worker(void *pvParameters) {
    uintptr_t id = (uintptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = (uint32_t)id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // Direkter Hardware-Befehl an den BM1370
        bm1370_set_nonce_range(start, end);
        vTaskDelay(pdMS_TO_TICKS(550)); 
    }
}

void app_main(void) {
    ESP_LOGI("MATRIX", "Starte 16 Matrix-Worker...");

    // 1. Matrix-Tasks starten
    for (uintptr_t i = 0; i < 16; i++) {
        xTaskCreate(matrix_worker, "Mx", 3072, (void*)i, 2, NULL);
    }

    // 2. Den Rest von AxeOS laden (Webserver, WLAN, etc.)
    // Wir übergeben NULL, da AxeOS die GLOBAL_STATE meist intern statisch verwaltet
    SYSTEM_init_system(global_gs_ptr);
    
    printf("MATRIX: Handover an AxeOS abgeschlossen.\n");
}
