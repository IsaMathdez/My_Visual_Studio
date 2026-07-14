

#ifndef BUOY_DATA_H
#define BUOY_DATA_H

#include <stdbool.h>
#include <stdint.h>

/*==============================================================
                Estado de un sensor tipo float
==============================================================*/

typedef struct
{
    float value;

    bool valid;

    uint32_t last_update_ms;

} sensor_float_t;


/*==============================================================
                    Datos completos de la boya
==============================================================*/

typedef struct
{
    uint32_t timestamp;

    sensor_float_t temperature;

    sensor_float_t tds;

    sensor_float_t salinity;

    sensor_float_t wave_height;

    sensor_float_t wave_period;

} buoy_data_t;


/*==============================================================
                    Variable global
==============================================================*/

extern buoy_data_t buoy_data;

#endif