/**
 * @file hx711_init.c
 * @author Marek Galeczka (marek.galeczka@outlook.com)
 * @brief 
 * @version 0.2
 * @date 2026-05-12
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "hx711.h"
#include "driver/gpio.h"

#define HX711_MODE_MIN HX711_MODE_A128
#define HX711_MODE_MAX HX711_MODE_A64

#define HX711_CHECK_MODE_MIN(mode)      ((mode) >= HX711_MODE_MIN)
#define HX711_CHECK_MODE_MAX(mode)      ((mode) <= HX711_MODE_MAX)
#define HX711_MODE_IS_VALID_MODE(mode)  (HX711_CHECK_MODE_MIN((mode)) && HX711_CHECK_MODE_MAX((mode)))

#define HX711_READY(dev) (!gpio_get_level((dev->ioDout)))

#define ADC_BIT_COUNT 24

static void IRAM_ATTR
hx711_ISR_(void* arg)
{
    hx711_HandleTypeDef dev = (hx711_HandleTypeDef)arg;
    if (dev->callback)
    {
        dev->callback(dev->callbackArg);
    }
}

hx711_StatusTypeDef 
hx711_Open(hx711_HandleTypeDef dev, gpio_num_t ioSck, gpio_num_t ioDout, uint8_t mode, uint32_t timeoutMs, hx711_CallbackTypeDef callback, void* arg)
{
    if (dev == NULL)
    {
        return HX711_ERR_NOVAL;
    }

    if (!GPIO_IS_VALID_GPIO(ioDout) || !GPIO_IS_VALID_OUTPUT_GPIO(ioSck))
    {
        return HX711_ERR_HW;
    }

    if (!HX711_MODE_IS_VALID_MODE(mode))
    {
        return HX711_ERR_INVAL;
    }

    bool enableIsr = (callback != NULL);

    gpio_config_t ioSckConfig = {
        .pin_bit_mask = (1ULL << (gpio_num_t)ioSck),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };

    gpio_config_t ioDoutConfig = {
        .pin_bit_mask = (1ULL << (gpio_num_t)ioDout),
        .mode = GPIO_MODE_INPUT,
        .intr_type = (enableIsr) ? GPIO_INTR_NEGEDGE : GPIO_INTR_DISABLE,
    };

    if (gpio_config(&ioSckConfig) != ESP_OK || gpio_config(&ioDoutConfig)!= ESP_OK)
    {
        return HX711_ERR_HW;
    }

    if (enableIsr)
    {
        if (gpio_isr_handler_add(ioDout, hx711_Isr_, dev) != ESP_OK || gpio_intr_disable(ioDout) != ESP_OK)
        {
            return HX711_ERR_ISR;
        }
    }

    gpio_set_level(ioSck, 1);
    esp_rom_delay_us(100);
    gpio_set_level(ioSck, 0);

    dev->ioSck = ioSck;
    dev->ioDout = ioDout;
    dev->mode = mode;
    dev->timeoutMs = timeoutMs;
    dev->callback = callback;
    dev->callbackArg = arg;
	dev->mux = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    return HX711_ERR_OK;
}

hx711_StatusTypeDef
hx711_Close(hx711_HandleTypeDef dev)
{
    if (dev == NULL)
    {
        return HX711_ERR_INVAL;
    }

    hx711_StatusTypeDef res = HX711_ERR_OK;

    if (dev->callback != NULL)
    {
        if (gpio_intr_disable(dev->ioDout) != ESP_OK || gpio_isr_handler_remove(dev->ioDout) != ESP_OK)
        {
            res = HX711_ERR_ISR;
        }
    }

    if (gpio_reset_pin(dev->ioSck) != ESP_OK || gpio_reset_pin(dev->ioDout) != ESP_OK)
    {
        res = HX711_ERR_HW;
    }

    dev->ioSck = GPIO_NUM_NC;
    dev->ioDout = GPIO_NUM_NC;
    dev->callback = NULL;
    dev->callbackArg = NULL;

    return res;
}

static int32_t
hx711_ReadFromDevice_(hx711_HandleTypeDef dev)
{
	uint32_t rawCode = 0;
	portENTER_CRITICAL(&dev->mux);
	
    for (uint8_t i = 0; i < ADC_BIT_COUNT; ++i)
    {
        gpio_set_level(dev->ioSck, 1);
        esp_rom_delay_us(1);
        rawCode = (rawCode << 1) | gpio_get_level(dev->ioDout);
        gpio_set_level(dev->ioSck, 0);
        esp_rom_delay_us(1);
    }
	
    for (uint8_t i = 0; i < ((uint8_t)dev->mode - ADC_BIT_COUNT); ++i)
    {
        gpio_set_level(dev->ioSck, 1);
        esp_rom_delay_us(1);
        gpio_set_level(dev->ioSck, 0);
        esp_rom_delay_us(1);
    }
	
	portEXIT_CRITICAL(&dev->mux);
    return (rawCode & 0x800000) ? (int32_t)(rawCode | 0xFF000000) : (int32_t)rawCode;
}

hx711_StatusTypeDef
hx711_Read(hx711_HandleTypeDef dev, int32_t* code)
{
    if (dev == NULL)
    {
        return HX711_ERR_NODEV;
    }
    if (code == NULL)
    {
        return HX711_ERR_INVAL;
    }

    if (gpio_get_level(dev->ioSck) != 0)
    {
        if (gpio_set_level(dev->ioSck, 0) != ESP_OK)
        {
            return HX711_ERR;
        }
    }
	
	if (dev->callback != NULL)
	{
		if (!HX711_READY(dev))
		{
			return HX711_ERR_NOT_RDY;
		}
		
		gpio_intr_disable(dev->ioDout);
		*code = hx711_ReadFromDevice_(dev);
		gpio_intr_enable(dev->ioDout);
		return HX711_ERR_OK;
	}
	else
	{
		if ( dev->timeoutMs == 0 )
		{
			if (!HX711_READY(dev))
			{
				return HX711_ERR_NOT_RDY;
			}
			
			*code = hx711_ReadFromDevice_(dev);
			return HX711_ERR_OK;
		}

		uint64_t deadlineUs = esp_timer_get_time() + 1000ULL * (uint64_t)dev->timeoutMs;
		while ( esp_timer_get_time() < deadlineUs )
		{
			if (!HX711_READY(dev))
			{
				vTaskDelay(1);
				continue;
			}
			
			*code = hx711_ReadFromDevice_(dev);
			return HX711_ERR_OK;
		}
	}
	
    return HX711_ERR_TIMEOUT;
}

static hx711_StatusTypeDef
hx711_SetTimeout_(hx711_HandleTypeDef dev, uint32_t* timeoutMs)
{
	if (timeoutMs == NULL)
	{
		return HX711_ERR_INVAL;
	}
	
	dev->timeoutMs = *timeoutMs;
	return HX711_ERR_OK;
}

static hx711_StatusTypeDef
hx711_GetTimeout_(hx711_HandleTypeDef dev, uint32_t* timeoutMs)
{
	if (timeoutMs == NULL)
	{
		return HX711_ERR_INVAL;
	}
	
	*timeoutMs = dev->timeoutMs;
	return HX711_ERR_OK;
}

static hx711_StatusTypeDef
hx711_SetMode_(hx711_HandleTypeDef dev, uint8_t* mode)
{
	if (mode == NULL)
	{
		return HX711_ERR_INVAL;
	}
    if (!HX711_MODE_IS_VALID_MODE(*mode))
    {
        return HX711_ERR_INVAL;
    }
	
	dev->mode = *mode;
	return HX711_ERR_OK;
}

static hx711_StatusTypeDef
hx711_GetMode_(hx711_HandleTypeDef dev, uint8_t* mode)
{
	if (mode == NULL)
	{
		return HX711_ERR_INVAL;
	}
	
	*mode = dev->mode;
	return HX711_ERR_OK;
}

hx711_StatusTypeDef
hx711_Ioctl(hx711_HandleTypeDef dev, hx711_IoctlTypeDef request, void* arg)
{
	if (dev == NULL)
	{
		return HX711_ERR_NODEV;
	}
	
	hx711_StatusTypeDef status;
	
	switch(request)
	{
	case HX711_IOCTL_SET_TIMEOUT:
	{
		status = hx711_SetTimeout_(dev, (uint32_t*)arg);
		break;
	}
	case HX711_IOCTL_GET_TIMEOUT:
	{
		status = hx711_GetTimeout_(dev, (uint32_t*)arg);
		break;
	}
	case HX711_IOCTL_SET_MODE:
	{
		status = hx711_SetMode_(dev, (uint8_t*)arg);
		break;
	}
	case HX711_IOCTL_GET_MODE:
	{
		status = hx711_GetMode_(dev, (uint8_t*)arg);
		break;
	}
	default:
	{
		status = HX711_ERR_INVAL;
	}
	}
	
	return status;
}
