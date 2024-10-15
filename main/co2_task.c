#include "nvs_flash.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "zigbee.h"
#include "string.h"
#include "driver/gpio.h"
#include "scd4x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

#include "esp_pm.h"
#include "esp_private/esp_clk.h"
#include "esp_sleep.h"


#define SDC4X_SDA_PIN GPIO_NUM_22
#define SDC4X_SCL_PIN GPIO_NUM_23
#define MEASURE_INTERVAL_MS 30000

static const char *TAG = "sdc41_task";

void sdc41_task(void *pvParameters)
{
    int16_t error = 0;
    sensirion_i2c_hal_init(SDC4X_SDA_PIN, SDC4X_SCL_PIN);
    // Clean up potential SCD40 states
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();

    uint16_t serial_0;
    uint16_t serial_1;
    uint16_t serial_2;
    error = scd4x_get_serial_number(&serial_0, &serial_1, &serial_2);
    if (error) {
        ESP_LOGE(TAG, "Error executing scd4x_get_serial_number(): %i", error);
    } else {
        ESP_LOGI(TAG, "serial: 0x%04x%04x%04x", serial_0, serial_1, serial_2);
    }

    error = scd4x_start_periodic_measurement();
    if (error) {
        ESP_LOGE(TAG, "Error executing scd4x_start_periodic_measurement(): %i", error);
    }

    ESP_LOGI(TAG, "Waiting for first measurement... (5 sec)");
    sensirion_i2c_hal_sleep_usec(100000);

    uint16_t co2;
    int32_t temperature;
    int32_t humidity;
    int16_t zb_temperature;
    int16_t zb_humidity;
    float_t zb_co2;

    while (1)
    {
        bool data_ready_flag = false;
        error = scd4x_get_data_ready_flag(&data_ready_flag);
        if (error) {
            ESP_LOGE(TAG, "Error executing scd4x_get_data_ready_flag(): %i", error);
            continue;
        }
        if (!data_ready_flag) {
            continue;
        }


        error = scd4x_read_measurement(&co2, &temperature, &humidity);
        if (error) {
            ESP_LOGE(TAG, "Error executing scd4x_read_measurement(): %i", error);
        } else if (co2 == 0) {
            ESP_LOGE(TAG, "Invalid sample detected, skipping.");
        } else {
            zb_temperature = temperature/10;
            zb_humidity = humidity/10;
            zb_co2 = (float_t)co2/1000000.0f;
            ESP_LOGI(TAG, "Hum: %.1f %%; Tmp: %.1f °C; CO2: %d ppm", zb_humidity/100.0, zb_temperature/100.0, co2);
            reportAttribute(HA_ESP_CO2_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &zb_temperature, 2);
            reportAttribute(HA_ESP_CO2_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, &zb_humidity, 2);
            reportAttribute(HA_ESP_CO2_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_CARBON_DIOXIDE_MEASUREMENT, ESP_ZB_ZCL_ATTR_CARBON_DIOXIDE_MEASUREMENT_MEASURED_VALUE_ID, &zb_co2, 4);
        }
        vTaskDelay(MEASURE_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}