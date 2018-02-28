/*
 * main.h
 *
 *  Created on: 23 февр. 2018 г.
 *      Author: anykey
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f10x.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#define page31 FLASH_BASE + (31 * 1024)
#define page63 FLASH_BASE + (63 * 1024)

#define HEATER_pin 13

// Funtion prototypes
void ADC1_init(void);
void GPIO_init(void);
void RCC_init(void);
void TIM6_init(void);
float map(float x, float in_min, float in_max, float out_min, float out_max);
u8 PT100_GetTemp(void);
void StartWork(void);


// Buffer for USART transmit
char buf[100];
u32 tmp;
u8 uart_data;



#endif /* MAIN_H_ */
