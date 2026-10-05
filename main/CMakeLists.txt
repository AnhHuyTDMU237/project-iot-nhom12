idf_component_register(
    SRCS
        "main.c"
        "dht22.c"
        "ssd1306.c"

    INCLUDE_DIRS
        "."

    PRIV_REQUIRES
        driver
        esp_adc
        esp_event
        esp_wifi
        esp_netif
        nvs_flash
        mqtt
        esp_timer
        esp_driver_i2s

    EMBED_FILES
        "gas_warning.wav"
        "fire_warning.wav"
)