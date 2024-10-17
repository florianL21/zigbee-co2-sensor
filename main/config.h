#pragma once

#include <stdint.h>
#include "driver/gpio.h"


#define SDC4X_SDA_PIN GPIO_NUM_22
#define SDC4X_SCL_PIN  GPIO_NUM_23
// Recommened measurement interval from sensirion is 5 minutes as the default ASC settings are assuming 5 minutes
#define MEASURE_INTERVAL_MS (30000)


// Sanity values for measurement bounds
#define TEMP_MIN -10
#define TEMP_MAX 60

#define HUMID_MIN 0
#define HUMID_MAX 95

#define CO2_MIN 0.0
#define CO2_MAX 5000.0

// Misc settings, for very fine tuning
// time in ms before the zigbee stack sends a CAN_SLEEP signal
#define ZIGBEE_SLEEP_THRESHOLD 20
