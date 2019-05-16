#include "ota.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP specific includes
#include "esp_flash_partitions.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

// Other
#include "queues.h"

#define HASH_LEN 32
#define BUFFSIZE 1024
// OTA data write buffer ready to write to the flash
static char ota_write_data[BUFFSIZE + 1] = {0};

void print_sha256(const uint8_t *image_hash, const char *label) {
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI("OTA", "%s: %s", label, hash_print);
}

static void http_cleanup(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

// Simple routine that waits for a publish in MQTT ota topic to upgrade
// firmware.
#define STACK_SIZE 4096
StaticTask_t ota_upgrade_task_buffer;
StackType_t ota_upgrade_task_stack[STACK_SIZE];
static void ota_upgrade_task(void *pvParameter) {
    esp_err_t err;
    // update handle : set by esp_ota_begin(), must be freed via esp_ota_end()
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    // Wait for an OTA upgrade request to come.
    while (true) {
        static char ota_url[QUEUE_SIZE_OTA + 1] = {0};
        ota_url[QUEUE_SIZE_OTA] = '\0';
        if (xQueueReceive(dispatcher_queues[QUEUE_OTA], (void *)&ota_url,
                          portMAX_DELAY) != pdTRUE) {
            ESP_LOGI("OTA", "Queue is not available, ignoring message");
            continue;
        }

        esp_http_client_config_t config = {
            .url = ota_url,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL) {
            ESP_LOGE("OTA", "Failed to initialise HTTP connection");
            continue;
        }
        err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE("OTA", "Failed to open HTTP connection: %s",
                     esp_err_to_name(err));
            esp_http_client_cleanup(client);
            continue;
        }
        esp_http_client_fetch_headers(client);

        update_partition = esp_ota_get_next_update_partition(NULL);
        ESP_LOGI("OTA", "Writing to partition subtype %d at offset 0x%x",
                 update_partition->subtype, update_partition->address);
        assert(update_partition != NULL);

        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
        if (err != ESP_OK) {
            ESP_LOGE("OTA", "esp_ota_begin failed (%s)", esp_err_to_name(err));
            http_cleanup(client);
            continue;
        }
        ESP_LOGI("OTA", "esp_ota_begin succeeded");

        int binary_file_length = 0;
        // deal with all receive packet
        while (1) {
            int data_read =
                esp_http_client_read(client, ota_write_data, BUFFSIZE);
            if (data_read < 0) {
                ESP_LOGE("OTA", "Error: data read error");
                http_cleanup(client);
                continue;
            } else if (data_read > 0) {
                err = esp_ota_write(update_handle, (const void *)ota_write_data,
                                    data_read);
                if (err != ESP_OK) {
                    http_cleanup(client);
                    continue;
                }
                binary_file_length += data_read;
                ESP_LOGD("OTA", "Written image length %d", binary_file_length);
            } else if (data_read == 0) {
                ESP_LOGI("OTA", "Connection closed, all data received");
                break;
            }
        }
        ESP_LOGI("OTA", "Total Write binary data length : %d",
                 binary_file_length);

        if (esp_ota_end(update_handle) != ESP_OK) {
            ESP_LOGE("OTA", "esp_ota_end failed!");
            http_cleanup(client);
            continue;
        }

        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE("OTA", "esp_ota_set_boot_partition failed (%s)!",
                     esp_err_to_name(err));
            http_cleanup(client);
            continue;
        }
        ESP_LOGI("OTA", "Prepare to restart system!");
        esp_restart();
        return;
    }
}

void ota_details() {
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    uint8_t sha_256[HASH_LEN] = {0};
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address = ESP_PARTITION_TABLE_OFFSET;
    partition.size = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address = ESP_BOOTLOADER_OFFSET;
    partition.size = ESP_PARTITION_TABLE_OFFSET;
    partition.type = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");

    if (configured != running) {
        ESP_LOGW("OTA",
                 "Configured OTA boot partition at offset 0x%08x, but running "
                 "from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW("OTA",
                 "(This can happen if either the OTA boot data or preferred "
                 "boot image become corrupted somehow.)");
    }
    ESP_LOGI("OTA", "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);
}

void ota_init() {
    ota_details();

    xTaskCreateStatic(&ota_upgrade_task, "ota_upgrade_task", STACK_SIZE, NULL,
                      tskIDLE_PRIORITY + 1, ota_upgrade_task_stack,
                      &ota_upgrade_task_buffer);
}
