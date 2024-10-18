#pragma once

typedef enum {
    CO2_MEASUREMENT_PENDING,
    CO2_MEASUREMENT_DONE
} SensorState;

// globally tracked task handles
extern TaskHandle_t xSdc41Task;
extern TaskHandle_t xZigbeeTask;

void sdc41_task(void *pvParameters);
void esp_zb_task(void *pvParameters);