#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Wir binden die Original-Header ein, damit alle Tasks zufrieden sind
#include "asic.h"
#include "system.h"
#include "bm1370.h"

// WICHTIG: AxeOS definiert GLOBAL_STATE oft in system.c. 
// Wir greifen hier nur darauf zu, ohne sie neu zu erstellen.
extern GlobalState GLOBAL_STATE;

void matrix_worker(void *pvParameters) {
    uintptr_t id = (uintptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = (uint32_t)id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);

    while (1) {
        // AxeOS v2.13.x Check
        if (GLOBAL_STATE.ASIC_initalized) {
            bm1370_set_nonce_range(start, end);
        }
        vTaskDelay(pdMS_TO_TICKS(550)); 
    }
}

void app_main(void) {
    // 1. Basis-Initialisierung (Muss für AxeOS sein)
    nvs_flash_init();

    // 2. Start der 16 Matrix-Worker
    for (uintptr_t i = 0; i < 16; i++) {
        xTaskCreate(matrix_worker, "Mx", 3072, (void*)i, 2, NULL);
    }

    // 3. Der offizielle AxeOS System-Start
    // Da system.c in deinen SRCS steht, wird dies alles verknüpfen
    SYSTEM_init_system(&GLOBAL_STATE);
    
    ESP_LOGI("MATRIX", "Matrix-OS erfolgreich injiziert.");
}
