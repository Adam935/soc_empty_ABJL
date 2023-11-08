/*
 * temperature.c
 *
 *  Created on: 7 nov. 2023
 *      Author: bessa
 */

#include "sl_sensor_rht.h"
#include <stdint.h>

int16_t convertTemperatureToBLE(float temperatureCelsius) {
    if (temperatureCelsius < -273.15 || temperatureCelsius > 327.67) {

        return 0x8000;
    }
    return (int16_t)(temperatureCelsius * 100);
}

sl_status_t getAndConvertTemperatureToBLE(int16_t *bleTemperature) {
    uint32_t rh;
    int32_t t;

    sl_status_t status = sl_sensor_rht_get(&rh, &t);

    if (status == SL_STATUS_OK) {

        *bleTemperature = convertTemperatureToBLE((float)t / 1000.0);
    } else {

        *bleTemperature = 0x8000;
    }

    return status;
}

