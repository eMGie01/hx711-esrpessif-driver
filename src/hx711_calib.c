#include "hx711.h"

#include "esp_rom_sys.h"
#include "esp_err.h"
#include "esp_log.h"


/**
 * procedura kalibracji
 * set_offset
 * get_offset
 * set scaleX1k
 * get scaleX1k
 */

# define HX711_CHECK_INIT(dev)              \
    do {                                    \
        if (!dev->initialized)              \
            return HX711_NOT_INITIALIZED;   \
    } while (0)


hx711_status_t
hx711_set_offset_raw(hx711_t * dev, const int32_t offset)
{
    if ( !dev )
        return HX711_INVALID_ARG;
    HX711_CHECK_INIT(dev);

    dev->calib.offset = offset;
    return HX711_OK;
}


hx711_status_t
hx711_get_offset_raw(const hx711_t * dev, int32_t * value)
{
    if ( !dev || !value )
        return HX711_INVALID_ARG;
    HX711_CHECK_INIT(dev);

    *value = dev->calib.offset;
    return HX711_OK;
}


hx711_status_t
hx711_set_scale_raw(hx711_t * dev, const int32_t scale)
{
    if ( !dev )
        return HX711_INVALID_ARG;
    HX711_CHECK_INIT(dev);

    dev->calib.scale = scale;
    return HX711_OK;
}


hx711_status_t
hx711_get_scale_raw(const hx711_t * dev, int32_t * value)
{
    if ( !dev || !value )
        return HX711_INVALID_ARG;
    HX711_CHECK_INIT(dev);

    *value = dev->calib.scale;
    return HX711_OK;
}