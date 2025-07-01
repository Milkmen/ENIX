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
      .base_path = "/rootfs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    
    if (ret != ESP_OK) 
    {
        ESP_LOGE("SPIFFS", "Failed to mount or format filesystem");
        return false;
    }
    
    const esp_partition_t* spiffs_partition = esp_partition_find_first
    (
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
        NULL
    );

    if (spiffs_partition) 
    {
        *rootfs_size = spiffs_partition->size;
    }
    else 
    {
        return false;
    }
    
    return true;
}

void fs_shutdown()
{
	esp_vfs_spiffs_unregister(NULL);
}