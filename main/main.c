#include "nvs_flash.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "zigbee.h"
#include "tasks.h"
#include "esp_pm.h"
#include "esp_private/esp_clk.h"
#include "esp_sleep.h"
#include "config.h"

static const char *TAG = "CO2";

TaskHandle_t xSdc41Task;
TaskHandle_t xZigbeeTask;


static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee bdb commissioning");
}

void start_sensor_measurements()
{
    xTaskCreate(sdc41_task, "sdc41_task", 4096, NULL, 5, &xSdc41Task);
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    static bool allow_sleep = false;
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    uint32_t sensor_state = 0;
    switch (sig_type)
    {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Zigbee stack initialized");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK)
        {
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            if (esp_zb_bdb_is_factory_new()) {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                ESP_LOGI(TAG, "Device rebooted");
                start_sensor_measurements();
            }
        }
        else
        {
            /* commissioning failed */
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s). Resetting the esp after 2s", esp_err_to_name(err_status));
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            esp_restart();
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK)
        {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
            zb_zdo_pim_set_long_poll_interval(ED_KEEP_ALIVE);
        }
        else
        {
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
        BaseType_t xResult = xTaskNotifyWait( pdFALSE,          /* Don't clear bits on entry. */
                                 ULONG_MAX,        /* Clear all bits on exit. */
                                 &sensor_state,
                                 0 ); /* do not wait for the flag to be set, simply skip going to sleep in this case */
        if( xResult == pdPASS )
        {
            switch(sensor_state) {
                case CO2_MEASUREMENT_PENDING:
                    allow_sleep = false;
                    break;
                case CO2_MEASUREMENT_DONE:
                    allow_sleep = true;
                    break;
            }
        }
        if(allow_sleep) {
            // sensor values have been read and sending via zigbee was requested
            ESP_LOGI(TAG, "Going to sleep");
            esp_zb_sleep_now();
            ESP_LOGI(TAG, "Zigbee woke up");
            esp_sleep_source_t wake_up_cause = esp_sleep_get_wakeup_cause();
            ESP_LOGI(TAG, "Woke up because: %d", wake_up_cause);
        }
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

static esp_err_t esp_zb_power_save_init(void)
{
    esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = 40,
        .light_sleep_enable = true
    };
    // uart_set_wakeup_threshold(UART_NUM_0, 3);
    // enable uart as a wakeup source to enable a better experience when trying to flash the firmware
    // esp_sleep_enable_uart_wakeup(UART_NUM_0);
    // ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(MEASURE_INTERVAL_MS * 1000));
    return esp_pm_configure(&pm_config);
}

void configure_internal_antenna(void) {
    gpio_reset_pin(GPIO_NUM_3);
    gpio_reset_pin(GPIO_NUM_14);
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_3, 0);//turn on antenna selection
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_direction(GPIO_NUM_14, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_14, 0);//use internal antenna
}

void pm_dump(void *pvParameters) {
    while (1) {
        esp_pm_dump_locks(stdout);
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    configure_internal_antenna();
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    /* enable power saving */
    ESP_ERROR_CHECK(esp_zb_power_save_init());
    // zb_deep_sleep_init();
    /* load Zigbee light_bulb platform config to initialization */
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    /* hardware related and device init */
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, &xZigbeeTask);
    xTaskCreate(pm_dump, "pm_dump", 4096, NULL, 5, NULL);
}
