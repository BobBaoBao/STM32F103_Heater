#include "RTC.h"

void RTC_Init(void) {

	// Включить тактирование модулей управления питанием и управлением резервной областью
	RCC->APB1ENR |= RCC_APB1ENR_BKPEN|RCC_APB1ENR_PWREN;
	// Разрешить доступ к области резервных данных
	PWR->CR |= PWR_CR_DBP;
	// Если RTC выключен - инициализировать
	if ((RCC->BDCR & RCC_BDCR_RTCEN) != RCC_BDCR_RTCEN)
	{
		// Сброс данных в резервной области
		RCC->BDCR |=  RCC_BDCR_BDRST;
		RCC->BDCR &= ~RCC_BDCR_BDRST;

		// Установить источник тактирования кварц 32768
		RCC->BDCR |=  RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_LSE;

		RTC->CRL  |=  RTC_CRL_CNF;
		RTC->PRLL  =  0x7FFF;   // Устанавливаем делитель на 32768, чтобы часы считали секунды
		RTC->CRH  |=  RTC_CRH_SECIE;
		RTC->CRL  &=  ~RTC_CRL_CNF;
		// Установить бит разрешения работы и дождаться установки бита готовности
		RCC->BDCR |= RCC_BDCR_LSEON;
		while ((RCC->BDCR & RCC_BDCR_LSEON) != RCC_BDCR_LSEON){}

		RTC->CRL &= (u16)~RTC_CRL_RSF;
		while((RTC->CRL & RTC_CRL_RSF) != RTC_CRL_RSF){}

		RTC->CRL &= ~RTC_CRL_SECF;
	}
	else {
		RTC->CRH  |=  RTC_CRH_SECIE;
	}
	NVIC_EnableIRQ(RTC_IRQn);	// Разрешить прерывания от RTC
}
