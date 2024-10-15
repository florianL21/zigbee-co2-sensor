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
#include "config.h"


static const char *TAG = "sdc41_task";

void sdc41_task(void *pvParameters)
{
    int16_t error = 0;
    sensirion_i2c_hal_init(SDC4X_SDA_PIN, SDC4X_SCL_PIN);
    // Clean up potential SCD40 states
    ESP_LOGI(TAG, "Resetting SCD4x sensor state, some i2c errors are expected");
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

    uint16_t co2;
    int32_t temperature;
    int32_t humidity;
    int16_t zb_temperature;
    int16_t zb_humidity;
    float_t zb_co2;
    float_t sanity_temperature;
    float_t sanity_humidity;
    bool plausible;

    while (1)
    {
        bool data_ready_flag = false;
        error = scd4x_measure_single_shot();
        if (error) {
            ESP_LOGE(TAG, "Error executing scd4x_measure_single_shot(): %i", error);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
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
        } else {
            plausible = true;
            zb_temperature = temperature/10;
            zb_humidity = humidity/10;
            zb_co2 = (float_t)co2/1000000.0f;
            sanity_temperature = zb_temperature/100.0;
            sanity_humidity = zb_humidity/100.0;
            ESP_LOGI(TAG, "Hum: %.1f %%; Tmp: %.1f °C; CO2: %d ppm", sanity_humidity, sanity_temperature, co2);
            if(sanity_temperature >= TEMP_MIN && sanity_temperature <= TEMP_MAX) {
                reportAttribute(HA_ESP_CO2_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &zb_temperature, 2);
            } else {
                ESP_LOGW(TAG, "Temperature value is implausible and no Zigbee update will be sent");
                plausible = false;
            }
            if(sanity_humidity >= HUMID_MIN && sanity_humidity <= HUMID_MAX) {
                reportAttribute(HA_ESP_CO2_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, &zb_humidity, 2);
            } else {
                ESP_LOGW(TAG, "Humidity value is implausible and no Zigbee update will be sent");
                plausible = false;
            }
            if(co2 >= CO2_MIN && co2 <= CO2_MAX) {
                reportAttribute(HA_ESP_CO2_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_CARBON_DIOXIDE_MEASUREMENT, ESP_ZB_ZCL_ATTR_CARBON_DIOXIDE_MEASUREMENT_MEASURED_VALUE_ID, &zb_co2, 4);
            } else {
                ESP_LOGW(TAG, "CO2 value is implausible and no Zigbee update will be sent");
                plausible = false;
            }
            if(plausible) {
                vTaskDelay(MEASURE_INTERVAL_MS / portTICK_PERIOD_MS);
            }
        }
    }
}