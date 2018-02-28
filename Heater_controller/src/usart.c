/*
 * usart.c
 *
 *  Created on: Feb 27, 2018
 *      Author: anykey
 */
#include "usart.h"
#include "stm32f10x.h"


void USART_init(void) {

	RCC->APB2ENR|= RCC_APB2ENR_USART1EN;

	// Set up UART pins
	GPIOA->CRH &= ~GPIO_CRH_CNF9;	// Clear CNF bit 9
	GPIOA->CRH |= GPIO_CRH_CNF9_1;	// Set CNF bit 9 to 10 - AFIO Push-Pull
	GPIOA->CRH |= GPIO_CRH_MODE9_0;
	GPIOA->CRH &= ~GPIO_CRH_CNF10;	// Clear CNF bit 9
	GPIOA->CRH |= GPIO_CRH_CNF10_0;	// Set CNF bit 9 to 01 = HiZ
	GPIOA->CRH &= ~GPIO_CRH_MODE10;	// Set MODE bit 9 to Mode 01 = 10MHz

	// Set up RCC for UART 9600b (72 MHz) page 800 in RM0008
	USART1->BRR = 0x1D4C;

	// Set up TxD, RxD, UART, start UART and enable interrupts of RxD & TxD
	USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE;

	// Назначаем обработчик для всех прерываний от USART1
	NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_Send(char chr) {

	while(!(USART1->SR & USART_SR_TC));
	USART1->DR = chr;
}

void USART1_Send_String(char* str) {

	int i=0;
	while(str[i])
		USART1_Send(str[i++]);
}
