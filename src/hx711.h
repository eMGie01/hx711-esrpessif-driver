/**
 * @file hx711.h
 * @author Marek Galeczka (marek.galeczka@outlook.com)
 * @brief 
 * @version 0.2
 * @date 2026-05-12
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef HX711_H
#define HX711_H

#include <stdint.h>
#include <stdbool.h>
// #include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * What do i want from this driver?
 * 1. I want it to be written in modern embedded style (open, close, read, write, ioctl) with handles and typedefs,
 * 2. I want it to have capabilities of notifying task by event group beats in IST use
 * 3. I want all of the functions, structures and enums to be written in: header_PascalCase
 *      - ex. hx711_StatusTypeDef
 */

#define HX711_MODE_A128 25
#define HX711_MODE_B32  26
#define HX711_MODE_A64  27

typedef void (*hx711_CallbackTypeDef)(void* arg);

typedef struct hx711_StatusTypeDef
{
    HX711_ERR_OK = 0,
    HX711_ERR_NODEV = -1,
    HX711_ERR_INVAL = -2,
    HX711_ERR_HW = -3,
    HX711_ERR_ISR = -4,

} hx711_StatusTypeDef;

typedef struct hx711_TypeDef 
{
    gpio_num_t ioSck;
    gpio_num_t ioDout;
    uint8_t mode;
    uint32_t timeoutMs;             // used for polling
    hx711_CallbackTypeDef callback; // NULL == polling mode
    void* callbackArg;

} hx711_TypeDef;

typedef hx711_TypeDef* hx711_HandleTypeDef;

hx711_StatusTypeDef hx711_Open(hx711_HandleTypeDef dev, uint8_t ioSck, uint8_t ioDout, uint8_t mode, uint32_t timeoutMs, hx711_CallbackTypeDef callback, void* arg);
hx711_StatusTypeDef hx711_Close(hx711_HandleTypeDef dev);

hx711_StatusTypeDef hx711_Read();
hx711_StatusTypeDef hx711_Write();
hx711_StatusTypeDef hx711_Ioctl();

#ifdef __cplusplus
}
#endif

#endif // HX711_H
