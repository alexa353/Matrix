//Test
#include "esp_event.h"
#include "esp_log.h"
#include "esp_psram.h"
#include <inttypes.h>

#include "asic_result_task.h"
#include "create_jobs_task.h"
#include "hashrate_monitor_task.h"
#include "fan_controller_task.h"
#include "statistics_task.h"
#include "system.h"
#include "http_server.h"
#include "serial.h"
#include "stratum_task.h"
#include "i2c_bitaxe.h"
#include "adc.h"
#include "nvs_config.h"
#include "self_test.h"
#include "asic.h"
#include "bap/bap.h"
#include "device_config.h"
#include "connect.h"
#include "asic_reset.h"
#include "asic_init.h"

static GlobalState GLOBAL_STATE;
static const char * TAG = "bitaxe";

// --- MATRIX KONFIGURATION ---
#define MATRIX_UNITS 16

/**
 * Matrix-Worker Task
 * Jede Einheit scannt einen exklusiven Bereich.
 * Ergebnisse werden über das asic_module zurück an den Stratum-Task gereicht.
 */
void matrix_mining_worker(void *pvParameters) {
    int id = (int)(intptr_t)pvParameters;
    GlobalState *gs = &GLOBAL_STATE;
    
    uint32_t step = 0xFFFFFFFF / MATRIX_UNITS;
    uint32_t my_start = id * step;
    uint32_t my_end = (id == MATRIX_UNITS - 1) ? 0xFFFFFFFF : (my_start + step - 1);

    ESP_LOGI("MATRIX", "Einheit %d initialisiert: [0x%08" PRIx32 " - 0x%08" PRIx32 "]", id, my_start, my_end);

    while (1) {
        // Nur agieren, wenn der ASIC bereit ist und ein Job vom Pool vorliegt
        if (gs->SYSTEM_MODULE.is_connected && gs->ASIC_MODULE.is_initialized) {
            
            // Setze den Nonce-Bereich für diese Matrix-Einheit im ASIC
            // Da AxeOS intern Nonces sammelt, werden Funde aus allen 
            // Bereichen in der asic_result_task gebündelt.
            asic_set_nonce_range(my_start, my_end);
        }
        
        // Da wir parallel arbeiten, geben wir anderen Tasks Zeit
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Welcome to the bitaxe - MATRIX EDITION");

    if (!esp_psram_is_initialized()) {
        ESP_LOGE(TAG, "No PSRAM available on ESP32 device!");
        GLOBAL_STATE.psram_is_available = false;
    } else {
        GLOBAL_STATE.psram_is_available = true;
    }

    ESP_ERROR_CHECK(i2c_bitaxe_init());
    ESP_ERROR_CHECK(asic_hold_reset_low());
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ADC_init();

    if (nvs_config_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init NVS");
        return;
    }

    GLOBAL_STATE.SYSTEM_MODULE.ssid = nvs_config_get_string(NVS_CONFIG_WIFI_SSID);
    if (GLOBAL_STATE.SYSTEM_MODULE.ssid == NULL) {
        GLOBAL_STATE.SYSTEM_MODULE.ssid = strdup("");
    }

    if (device_config_init(&GLOBAL_STATE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init device config");
        return;
    }

    if (self_test(&GLOBAL_STATE)) return;

    SYSTEM_init_system(&GLOBAL_STATE);
    wifi_init(&GLOBAL_STATE);
    SYSTEM_init_peripherals(&GLOBAL_STATE);

    // Standard Management Tasks
    xTaskCreate(POWER_MANAGEMENT_task, "power mgmt", 8192, (void *) &GLOBAL_STATE, 10, NULL);
    xTaskCreate(FAN_CONTROLLER_task, "fan_ctrl", 8192, (void *) &GLOBAL_STATE, 5, NULL);

    start_rest_server((void *) &GLOBAL_STATE);
    SYSTEM_init_versions(&GLOBAL_STATE);
    BAP_init(&GLOBAL_STATE);

    // Warten auf WiFi
    while (!GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    queue_init(&GLOBAL_STATE.stratum_queue);

    // ASIC Kaltstart
    if (asic_initialize(&GLOBAL_STATE, ASIC_INIT_COLD_BOOT, 0) == 0) {
        ESP_LOGE(TAG, "ASIC Init failed!");
        return;
    }

    // --- CORE MINING TASKS ---
    xTaskCreate(stratum_task, "stratum_admin", 8192, (void *) &GLOBAL_STATE, 5, NULL);
    xTaskCreate(create_jobs_task, "stratum_miner", 8192, (void *) &GLOBAL_STATE, 20, NULL);
    
    // Dieser Task bündelt alle gefundenen Nonces der Matrix-Einheiten zu gemeinsamen Shares:
    xTaskCreate(ASIC_result_task, "asic_result", 8192, (void *) &GLOBAL_STATE, 15, NULL);

    xTaskCreateWithCaps(hashrate_monitor_task, "hash_mon", 8192, (void *) &GLOBAL_STATE, 5, NULL, MALLOC_CAP_SPIRAM);
    xTaskCreateWithCaps(statistics_task, "stats", 8192, (void *) &GLOBAL_STATE, 3, NULL, MALLOC_CAP_SPIRAM);

    // --- START DER MATRIX EINHEITEN ---
    ESP_LOGI("MATRIX", "Aktiviere %d parallele Matrix-Einheiten...", MATRIX_UNITS);
    for (int i = 0; i < MATRIX_UNITS; i++) {
        char tname[16];
        snprintf(tname, sizeof(tname), "Matrix_%d", i);
        // Gleichverteilung auf Core 0 und 1
        xTaskCreatePinnedToCore(matrix_mining_worker, tname, 4096, (void *)(intptr_t)i, 2, NULL, i % 2);
    }
}
