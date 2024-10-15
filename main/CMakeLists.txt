idf_component_register(
    SRCS
    "main.c"
    "zigbee.c"
    "co2_task.c"
    INCLUDE_DIRS "."
    REQUIRES "esp-idf-scd4x" "nvs_flash"
)
