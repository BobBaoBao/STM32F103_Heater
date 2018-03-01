/*
 * main.h
 *
 *  Created on: Feb 28, 2018
 *      Author: anykey
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f10x.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include <stdbool.h>

#define page31 FLASH_BASE + (31 * 1024)
#define page32 FLASH_BASE + (32 * 1024)
#define HEATER_pin 1
#define ERROR_pin 13

// Прототипы функций
void ADC1_init(void);
void GPIO_init(void);
void RCC_init(void);
float map(float x, float in_min, float in_max, float out_min, float out_max);
u32 PT100_GetTemp(void);
void StartWork(void);
void WriteNewParams(const char *str);
void Error_Handler(void);

#endif /* MAIN_H_ */
