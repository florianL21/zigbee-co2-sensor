#pragma once

#include <stdint.h>
#include "driver/gpio.h"


#define SDC4X_SDA_PIN GPIO_NUM_22
#define SDC4X_SCL_PIN  GPIO_NUM_23
// Recommened measurement interval from sensirion is 5 minutes as the default ASC settings are assuming 5 minutes
#define MEASURE_INTERVAL_MS (60000*5)


// Sanity values for measurement bounds
#define TEMP_MIN -10
#define TEMP_MAX 60

#define HUMID_MIN 0
#define HUMID_MAX 95

#define CO2_MIN 0.0
#define CO2_MAX 5000.0

