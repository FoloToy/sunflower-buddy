idf_component_register(
    SRCS "main.cpp"
         "demo_record_playback.cpp"
         "demo_blufi.c"
         "demo_blufi_security.c"
    INCLUDE_DIRS "."
    REQUIRES bsp joltwallet__littlefs bt esp_wifi esp_netif nvs_flash mbedtls
)

littlefs_create_partition_image(storage data FLASH_IN_PROJECT)
