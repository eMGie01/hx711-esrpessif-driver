#include "hx711.h"


#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_err.h"


#define ADC_BIT_COUNT 24


static hx711_status_t IRAM_ATTR
read_raw_(hx711_t * dev, int32_t * value)
{
    uint32_t raw_adc_value = 0;
    atomic_store_explicit(&dev->data_ready, false, memory_order_relaxed);
    
    portENTER_CRITICAL(&dev->mux);

    for (uint8_t i = 0; i < ADC_BIT_COUNT; ++i)
    {
        gpio_set_level(dev->ios.io_sck, 1);
        esp_rom_delay_us(1);
        raw_adc_value = (raw_adc_value << 1) | gpio_get_level(dev->ios.io_dout);
        gpio_set_level(dev->ios.io_sck, 0);
        esp_rom_delay_us(1);
    }

    for (uint8_t i = 0; i < ((uint8_t)dev->settings.mode - ADC_BIT_COUNT); ++i)
    {
        gpio_set_level(dev->ios.io_sck, 1);
        esp_rom_delay_us(1);
        gpio_set_level(dev->ios.io_sck, 0);
        esp_rom_delay_us(1);
    }

    portEXIT_CRITICAL(&dev->mux);

    *value = (raw_adc_value & 0x800000) ? (int32_t)(raw_adc_value | 0xFF000000) : (int32_t)raw_adc_value;
    dev->last_raw = *value;
    
    return HX711_OK;
}


hx711_status_t
hx711_is_ready(const hx711_t * dev)
{
    if ( !dev )
    {
        return HX711_INVALID_ARG;
    }

    if ( !dev->initialized )
    {
        return HX711_NOT_INITIALIZED;
    }

    if ( !gpio_get_level(dev->ios.io_dout) )
    {
        return HX711_OK;
    }

    return HX711_NOT_READY;
}


hx711_status_t
hx711_read_raw(hx711_t * dev, int32_t * value)
{

    if ( !dev || !value )
    {
        return HX711_INVALID_ARG;
    }

    if ( !dev->initialized )
    {
        return HX711_NOT_INITIALIZED;
    }

    if ( HX711_MODE_MIN > dev->settings.mode || HX711_MODE_MAX < dev->settings.mode )
    {
        return HX711_INVALID_ARG;
    }

    if ( 0 != gpio_get_level(dev->ios.io_sck) )
    {
        if ( ESP_OK != gpio_set_level(dev->ios.io_sck, 0) )
        {
            return HX711_HW_ERR;
        }
    }

    if ( HX711_OK != hx711_is_ready(dev) )
    {
        return HX711_NOT_READY;
    }

    return read_raw_(dev, value);
}


hx711_status_t
hx711_read_raw_with_timeout(hx711_t * dev, int32_t * value)
{
    if ( !dev || !value )
    {
        return HX711_INVALID_ARG;
    }

    if ( !dev->initialized )
    {
        return HX711_NOT_INITIALIZED;
    }

    if ( HX711_MODE_MIN > dev->settings.mode || HX711_MODE_MAX < dev->settings.mode )
    {
        return HX711_INVALID_ARG;
    }

    if ( 0 != gpio_get_level(dev->ios.io_sck) )
    {
        if ( ESP_OK != gpio_set_level(dev->ios.io_sck, 0) )
        {
            return HX711_HW_ERR;
        }
    }

    if ( dev->settings.timeout_ms == 0 )
    {
        if ( HX711_OK != hx711_is_ready(dev) )
        {
            return HX711_NOT_READY;
        }

        return read_raw_(dev, value);
    }

    uint64_t deadline_us = esp_timer_get_time() + 1000ULL * (uint64_t)dev->settings.timeout_ms;
    while ( esp_timer_get_time() < deadline_us )
    {

        if ( HX711_OK != hx711_is_ready(dev) )
        {
            esp_rom_delay_us(100);
            continue;
        }
        
        return read_raw_(dev, value);

    }

    return HX711_TIMEOUT;
}


hx711_status_t
hx711_read_raw_isr(hx711_t * dev, int32_t * value)
{
    if ( !dev || !value )
    {
        return HX711_INVALID_ARG;
    }

    if ( !dev->initialized )
    {
        return HX711_NOT_INITIALIZED;
    }

    if ( HX711_MODE_MIN > dev->settings.mode || HX711_MODE_MAX < dev->settings.mode )
    {
        return HX711_INVALID_ARG;
    }

    if ( 0 != gpio_get_level(dev->ios.io_sck) )
    {
        if ( ESP_OK != gpio_set_level(dev->ios.io_sck, 0) )
        {
            return HX711_HW_ERR;
        }
    }

    uint64_t deadline_us = esp_timer_get_time() + 1000ULL * (uint64_t)dev->settings.timeout_ms;
    while ( esp_timer_get_time() < deadline_us )
    {

        if ( !atomic_load_explicit(&dev->data_ready, memory_order_acquire) )
        {
            esp_rom_delay_us(100);
            continue;
        }

        gpio_intr_disable((gpio_num_t)dev->ios.io_dout);
        hx711_status_t res = read_raw_(dev, value);
        gpio_intr_enable((gpio_num_t)dev->ios.io_dout);
        return res;
    }

    return HX711_TIMEOUT;
}
