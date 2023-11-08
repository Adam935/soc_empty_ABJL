/*
 * temperature.h
 *
 *  Created on: 7 nov. 2023
 *      Author: bessa
 */

#ifndef TEMPERATURE_H_
#define TEMPERATURE_H_

#include "sl_sensor_rht.h"
#include <stdint.h>

int16_t convertTemperatureToBLE(float temperatureCelsius);
sl_status_t getAndConvertTemperatureToBLE(int16_t *bleTemperature);

#endif /* TEMPERATURE_H_ */
