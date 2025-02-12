#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Arduino.h"
#include "Zigbee.h"
#include "driver/rtc_io.h"
// #include "iot_button.h"
#include <rom/rtc.h>
#include "driver/gpio.h"
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_check.h"
#include "freertos/task.h"
#include "scd4x_i2c.h"
#include "sensirion_i2c_hal.h"
#include "config.h"

#define S_TO_MS_FACTOR 1000
#define uS_TO_S_FACTOR 1000000ULL


/* Zigbee carbon dioxide sensor configuration */
#define CARBON_DIOXIDE_SENSOR_ENDPOINT_NUMBER 10
#define TEMP_SENSOR_ENDPOINT_NUMBER 11

ZigbeeCarbonDioxideSensor zbCarbonDioxideSensor = ZigbeeCarbonDioxideSensor(CARBON_DIOXIDE_SENSOR_ENDPOINT_NUMBER);
ZigbeeTempSensor zbTempSensor = ZigbeeTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);
static const char *TAG = "sdc41_task";
RTC_DATA_ATTR bool scd4x_initialized = false;
RTC_DATA_ATTR bool zigbee_reporting_configured = false;
RTC_DATA_ATTR uint16_t wakeup_counter = 0;
esp_timer_handle_t hold_timer;
esp_timer_handle_t cancel_timer;


float get_battery_percent() {
  uint32_t Vbatt = 0;
  for(int i = 0; i < 16; i++) {
    Vbatt += analogReadMilliVolts(A0); // Read and accumulate ADC voltage
  }
  long Vbattf = 2 * Vbatt / 16 / 1000.0;     // Adjust for 1:2 divider and convert to volts
  return (float)map(Vbattf, 3.8, 4.2, 0, 100);
}

void sdc41_task(void *pvParameters)
{
    int16_t error = 0;
    sensirion_i2c_hal_init(SDC4X_SDA_PIN, SDC4X_SCL_PIN);
    if(!scd4x_initialized) {
        // Clean up potential SCD40 states
        ESP_LOGI(TAG, "First boot after power cycle. Resetting SCD4x sensor state, some i2c errors are expected");
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
        scd4x_initialized = true;
    } else {
        ESP_LOGI(TAG, "Wakeup from deep sleep. Skipping initialization of SCD4x sensor");
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
    bool measurement_running = false;
    bool data_ready_flag;

    while (1)
    {
        data_ready_flag = false;
        if(!measurement_running) {
            ESP_LOGI(TAG, "Triggering SCD4x single shot measurement...");
            measurement_running = true;
            error = scd4x_measure_single_shot();
            if (error) {
                ESP_LOGE(TAG, "Error executing scd4x_measure_single_shot(): %i", error);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                measurement_running = false;
                continue;
            }
        }
        error = scd4x_get_data_ready_flag(&data_ready_flag);
        if (error) {
            ESP_LOGE(TAG, "Error executing scd4x_get_data_ready_flag(): %i", error);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            measurement_running = false;
            continue;
        }
        if (!data_ready_flag) {
            continue;
        }
        measurement_running = false;

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
                zbTempSensor.setTemperature(sanity_temperature);
            } else {
                ESP_LOGW(TAG, "Temperature value is implausible and no Zigbee update will be sent");
                plausible = false;
            }
            if(sanity_humidity >= HUMID_MIN && sanity_humidity <= HUMID_MAX) {
                zbTempSensor.setHumidity(sanity_humidity);
            } else {
                ESP_LOGW(TAG, "Humidity value is implausible and no Zigbee update will be sent");
                plausible = false;
            }
            if(co2 >= CO2_MIN && co2 <= CO2_MAX) {
                zbCarbonDioxideSensor.setCarbonDioxide(co2);
            } else {
                ESP_LOGW(TAG, "CO2 value is implausible and no Zigbee update will be sent");
                plausible = false;
            }
            if(plausible) {
                if(!zigbee_reporting_configured) {
                  ESP_LOGI(TAG, "Configuring zigbee reporting options");
                  zbCarbonDioxideSensor.setReporting(0, MEASURE_INTERVAL_S, 0);
                  zbTempSensor.setReporting(0, MEASURE_INTERVAL_S, 0);
                  zbTempSensor.setHumidityReporting(0, MEASURE_INTERVAL_S, 0);
                  zigbee_reporting_configured = true;
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                #ifdef BATTERY_POWERED
                if(wakeup_counter++ >= MEASURE_BATTERY_EVERY_X_CYCLES) {
                  wakeup_counter = 0;
                  uint8_t battery = get_battery_percent();
                  zbCarbonDioxideSensor.setBatteryPercentage(battery);
                  zbTempSensor.setBatteryPercentage(battery);
                }
                #endif
                zbCarbonDioxideSensor.report();
                vTaskDelay(100 / portTICK_PERIOD_MS);
                zbTempSensor.report();
                vTaskDelay(100 / portTICK_PERIOD_MS);
                #ifdef BATTERY_POWERED
                Serial.println("Going to sleep now");
                esp_deep_sleep_start();
                #else
                vTaskDelay(MEASURE_INTERVAL_S * S_TO_MS_FACTOR / portTICK_PERIOD_MS);
                #endif
            }
        }
    }
}


void identify(uint16_t time) {
  static uint8_t blink = 1;
  log_d("Identify called for %d seconds", time);
  if (time == 0) {
    // If identify time is 0, stop blinking
    return;
  }
  digitalWrite(LED_BUILTIN, blink);
  blink = !blink;
}

static void button_hold_confirmation(void* arg)
{
  // if pin is still low it was held down the entire time
  if (digitalRead(BOOT_PIN) == LOW) {
    for(int i = 0; i < 5; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
    }
    Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
    delay(1000);
    Zigbee.factoryReset();
  }
}

static void button_hold_cancel(void* arg)
{
  // if pin is still high, assume user has let go of button
  if (digitalRead(BOOT_PIN) == HIGH) {
    esp_timer_stop(hold_timer);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}


void boot_button_handler() {
  if (digitalRead(BOOT_PIN) == LOW) {
    digitalWrite(LED_BUILTIN, LOW);
    if (!esp_timer_is_active(hold_timer)) {
      // if hold timer was not yet started, do it now
      // check back in 3s if the button is still being held down
      ESP_ERROR_CHECK(esp_timer_start_once(hold_timer, 3000000));
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    if (!esp_timer_is_active(cancel_timer)) {
      // if cancel timer was not yet started, do it now
      ESP_ERROR_CHECK(esp_timer_start_once(cancel_timer, 50000));
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Init button switch
  pinMode(BOOT_PIN, INPUT_PULLUP);
  attachInterrupt(BOOT_PIN, boot_button_handler, CHANGE);
  // led on esp
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // assume battery voltage input on A0
  pinMode(A0, INPUT);

  const esp_timer_create_args_t hold_timer_args = {
    .callback = &button_hold_confirmation,
    .arg = nullptr,
    .name = "button-hold-timer"
  };
  ESP_ERROR_CHECK(esp_timer_create(&hold_timer_args, &hold_timer));
  const esp_timer_create_args_t cancel_timer_args = {
    .callback = &button_hold_cancel,
    .arg = nullptr,
    .name = "cancel-timer"
  };
  ESP_ERROR_CHECK(esp_timer_create(&cancel_timer_args, &cancel_timer));

  #ifdef BATTERY_POWERED
  // Configure the wake up source
  esp_sleep_enable_timer_wakeup(MEASURE_INTERVAL_S * uS_TO_S_FACTOR);
  // configure the boot button as wake up source
  esp_sleep_enable_ext1_wakeup_io(1 << BOOT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
  #endif

  // Optional: set Zigbee device name and model
  zbCarbonDioxideSensor.setManufacturerAndModel("FlorianL", "CO2 Sensor");

  // Set minimum and maximum carbon dioxide measurement value in ppm
  zbCarbonDioxideSensor.setMinMaxValue(CO2_MIN, CO2_MAX);

  zbCarbonDioxideSensor.onIdentify(identify);

  // Add endpoints to Zigbee Core
  Zigbee.addEndpoint(&zbCarbonDioxideSensor);

  // Optional: set Zigbee device name and model
  zbTempSensor.setManufacturerAndModel("FlorianL", "CO2 Sensor");

  // Set minimum and maximum temperature measurement value (10-50°C is default range for chip temperature measurement)
  zbTempSensor.setMinMaxValue(TEMP_MIN, TEMP_MAX);

  // Set tolerance for temperature measurement in °C (lowest possible value is 0.01°C)
  zbTempSensor.setTolerance(0.8);
  #ifdef BATTERY_POWERED
  // reading of battery level is not implemented just always report 100
  float battery = get_battery_percent();
  zbCarbonDioxideSensor.setPowerSource(ZB_POWER_SOURCE_MAINS, battery);
  zbTempSensor.setPowerSource(ZB_POWER_SOURCE_MAINS, battery);
  #else
  zbCarbonDioxideSensor.setPowerSource(ZB_POWER_SOURCE_MAINS, 100);
  zbTempSensor.setPowerSource(ZB_POWER_SOURCE_MAINS, 100);
  #endif

  // Add humidity cluster to the temperature sensor device with min, max and tolerance values
  zbTempSensor.addHumiditySensor(HUMID_MIN, HUMID_MAX, 6);
  zbTempSensor.onIdentify(identify);

  // Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbTempSensor);

  #ifdef BATTERY_POWERED
  // Create a custom Zigbee configuration for End Device with keep alive 10s to avoid interference with reporting data
  esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
  zigbeeConfig.nwk_cfg.zed_cfg.keep_alive = 10000;
  #endif

  Serial.println("Starting Zigbee...");
  // When all EPs are registered, start Zigbee in End Device mode
  #ifdef BATTERY_POWERED
  if (!Zigbee.begin(&zigbeeConfig, false))
  #else
  if (!Zigbee.begin())
  #endif
  {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  } else {
    Serial.println("Zigbee started successfully!");
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  #ifdef BATTERY_POWERED
  // Delay approx 1s (may be adjusted) to allow establishing proper connection with coordinator, needed for sleepy devices
  delay(1000);
  #endif

  // start the emasurement task, reporting setup will be done before sending the first attribute update
  xTaskCreate(sdc41_task, "sdc41_task", 4096, NULL, 5, NULL);
}

void loop() {
  // does nothing. everything is handled via interrupts etc.
  delay(100);
}
