#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bm1370.h"

void matrix_worker(void *pvParameters) {
    uintptr_t id = (uintptr_t)pvParameters;
    uint32_t step = 0xFFFFFFFF / 16;
    uint32_t start = (uint32_t)id * step;
    uint32_t end = (id == 15) ? 0xFFFFFFFF : (start + step - 1);
    while (1) {
        bm1370_set_nonce_range(start, end);
        vTaskDelay(pdMS_TO_TICKS(550));
    }
}

void __attribute__((constructor)) register_matrix(void) {
    for (uintptr_t i = 0; i < 16; i++) {
        xTaskCreate(matrix_worker, "Mx", 2048, (void*)i, 2, NULL);
    }
}
