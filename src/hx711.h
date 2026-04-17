#ifndef HX711_NEW_H
#define HX711_NEW_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif


// Enums
typedef enum {
    HX711_MODE_MIN   = 25,
    HX711_MODE_A_128 = 25,
    HX711_MODE_B_32  = 26,
    HX711_MODE_A_64  = 27,
    HX711_MODE_MAX   = 27
} hx711_mode_t;

typedef enum
{
    HX711_UNEXPECTED_ERR = -1,
    HX711_OK = 0,
    HX711_INVALID_ARG,
    HX711_NOT_INITIALIZED,
    HX711_TIMEOUT,
    HX711_HW_ERR,
    HX711_ISR_ERR,
    HX711_NOT_READY
} hx711_status_t;

// Structures
typedef struct 
{
    int io_sck;
    int io_dout;
} hx711_hw_t;

typedef struct
{
    hx711_mode_t mode;
    uint32_t     timeout_ms;
} hx711_set_t;

typedef struct
{
    int32_t scale;
    int32_t offset;
} hx711_cal_t;

typedef struct 
{
    volatile bool data_ready;
    hx711_hw_t    ios;
    hx711_set_t   settings;
    hx711_cal_t   calib;
    int32_t       last_raw;
    bool          initialized;
} hx711_t;


// Functions
hx711_status_t hx711_init(hx711_t * dev, hx711_hw_t * gpios, const hx711_set_t * settings);
hx711_status_t hx711_init_with_isr(hx711_t * dev, hx711_hw_t * gpios, const hx711_set_t * settings);
hx711_status_t hx711_init_default(hx711_t * dev, hx711_hw_t * gpios);
hx711_status_t hx711_deinit(hx711_t * dev);

hx711_status_t hx711_is_ready(const hx711_t * dev);
hx711_status_t hx711_read_raw(hx711_t * dev, int32_t * value);
hx711_status_t hx711_read_raw_with_timeout(hx711_t * dev, int32_t * value);
hx711_status_t hx711_read_raw_isr(hx711_t * dev, int32_t * value);

hx711_status_t hx711_set_offset_raw(hx711_t * dev, const int32_t offset);
hx711_status_t hx711_get_offset_raw(const hx711_t * dev, int32_t * value);
hx711_status_t hx711_set_scale_raw(hx711_t * dev, const int32_t scale);
hx711_status_t hx711_get_scale_raw(const hx711_t * dev, int32_t * value);


#ifdef __cplusplus
}
#endif

#endif /* HX711_H */
