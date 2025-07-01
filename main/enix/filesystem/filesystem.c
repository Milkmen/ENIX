#include "filesystem.h"

#include <stdio.h>
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_spiffs.h"

bool fs_initialize(size_t* rootfs_size, size_t* mountfs_size)
{
	ESP_LOGI("SPIFFS", "Mounting SPIFFS...");

    esp_vfs_spiffs_conf_t conf = 
    {
      .base_path = "/spiffs",
      .partition_label = "rootfs",  // Match the partition name in partitions.csv
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    
    if (ret != ESP_OK) 
    {
        ESP_LOGE("SPIFFS", "Failed to mount or format filesystem (%s)", esp_err_to_name(ret));
        return false;
    }
    
    const esp_partition_t* spiffs_partition = esp_partition_find_first
    (
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
        "rootfs"  // Match partition label
    );

    if (spiffs_partition) 
    {
        *rootfs_size = spiffs_partition->size;
        size_t total = 0, used = 0;
        ret = esp_spiffs_info("rootfs", &total, &used);
        if (ret == ESP_OK) {
            *mountfs_size = used;
            ESP_LOGI("SPIFFS", "Partition size: total: %d, used: %d", total, used);
        } else {
            *mountfs_size = 0;
        }
    }
    else 
    {
        ESP_LOGE("SPIFFS", "SPIFFS partition not found");
        return false;
    }
    
    return true;
}

void fs_shutdown()
{
	esp_vfs_spiffs_unregister("rootfs");  // Match partition label
}