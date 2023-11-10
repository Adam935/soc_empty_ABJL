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

        //from documentation we return 0x8000 if we got a absude temp
        return 0x8000;
    }

    return (int16_t)(temperatureCelsius * 100);
}


sl_status_t getAndConvertTemperatureToBLE(int16_t *bleTemperature) {
    uint32_t rh; // Parameter for sl_sensor
    int32_t t; // the resolution 0.01 °C (cf.documentation)

    sl_status_t status = sl_sensor_rht_get(&rh, &t);

    if (status == SL_STATUS_OK) {
         //convert the temperature to a BLE format
        *bleTemperature = convertTemperatureToBLE((float)t / 1000.0);
    } else {
        //from documentation we return 0x8000 if we got a error
        *bleTemperature = 0x8000;
    }

    return status;
}

