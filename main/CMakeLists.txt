idf_component_register(
    SRCS
    "esp_zb_co2.c"
    INCLUDE_DIRS "."
    REQUIRES "esp-idf-scd4x" "nvs_flash"
)
