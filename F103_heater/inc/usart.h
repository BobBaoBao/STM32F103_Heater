/*
 * usart.h
 *
 *  Created on: Feb 27, 2018
 *      Author: anykey
 */

#ifndef USART_H_
#define USART_H_

#include "main.h"
#include "stdio.h"

void USART_init(void);
void USART1_Send(char chr);
void USART1_Send_String(char* str);
void USART1_IRQHandler(void);
uint8_t read_number(void);

#endif /* USART_H_ */
