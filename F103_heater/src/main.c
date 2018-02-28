#include "main.h"

#include "flash.h"
#include "usart.h"
#include "RTC.h"

// Create two array for parameters, "temp" for temperature dot parameter, "time" for time dot parameter
u8 temp[20] = {0};
u8 time[20] = {0};
u8 timer = 0;

/*
void RTC_IRQHandler(void) {

	// Interrupt from seconds
	if(RTC->CRL & RTC_CRL_SECF) {

		// Reset interrupt flag
		RTC->CRL &= ~RTC_CRL_SECF;
		GPIOC->ODR ^= (1<<HEATER_pin);
	}

	// timer++;
	//if (timer == 55) {

	//USART1_Send_String("\r\n1");
	//timer = 0;
	//}
}
*/


int main(void) {

	// SystemInit();
	RCC_init();
	USART_init();
	ADC1_init();
	// RTC_Init();
	// TIM6_init();
	GPIO_init();

	// Before using flash need to unlock it
	FLASH_unlock();
	// Clear page
	FLASH_Erase_Page(page31);


	/* Start the program. Ask user about workmode:
	 * 1) New mode or 2) Start with previous parameters
	 */

	USART1_Send_String("======= Enter workmode: =======");
	USART1_Send_String("\r\n 1. New parameters     \
					    \r\n 2. Previous parameters\r\n");
	USART1_Send_String("===============================");

	// Wait for answer
	while (!(USART1->SR & USART_SR_RXNE));
	uart_data = read_number();

	switch (uart_data) {

	case 1:
		/* Enter new parameters from user, safe in flash and start work */
		USART1_Send_String("\r\n\r\n> Mode 1 is selected \r\n\r\n");
		USART1_Send_String("Enter number of dots: \r\n");

		// Wait for a new number of dots
		while (!(USART1->SR & USART_SR_RXNE));
		u8 numDots = read_number();
		sprintf(buf, "> %d dots selected\r\n\r\n", numDots);
		USART1_Send_String(buf);

		// Safe number of selected dots in flash ()
		FLASH_Write(page31, (u32)numDots);

		// Fill the arrays with the selected parameters and write to flash
		for(u8 i = 0; i < numDots; i++) {

			sprintf(buf, "Enter temperature for the %d dot: \r\n", i);
			USART1_Send_String(buf);
			while (!(USART1->SR & USART_SR_RXNE));
			uart_data = read_number();
			temp[i] = uart_data;

			// Write *temperature* param to flash after *numDots* (adress page31 + 4byte)
			FLASH_Write(i*4 + (page31 + 4), (u32)temp[i]);

			sprintf(buf, "Enter time for the %d dot: \r\n", i);
			USART1_Send_String(buf);
			while (!(USART1->SR & USART_SR_RXNE));
			uart_data = read_number();
			time[i] = uart_data;

			// Write *time* param to flash after last (numDots * 4) *temperature* param
			FLASH_Write(i*4 + (page31 + 4) + (numDots * 4), (u32)time[i]);
		}

		// Print selected params (for debug)
		USART1_Send_String("\r\n=======Selected params:========\r\n");
		for(u8 i = 0; i < numDots; i++) {

			sprintf(buf, "Temp for %d dot: %d C\r\n", i, temp[i]);
			USART1_Send_String(buf);

			sprintf(buf, "Time for %d dot: %d min\r\n", i, time[i]);
			USART1_Send_String(buf);
			USART1_Send_String("\r\n");
		}

		// Print data from flash (for debug)
		USART1_Send_String("\r\n\r\n=======Params in flash:========\r\n");
		for(u8 i = 0; i < numDots; i++) {

			sprintf(buf, "Temp %d: %ld C, Time %d: %ld min\r\n", i, FLASH_Read(i*4 + (page31) + 4), i,
					FLASH_Read(i*4 + (page31) + 4 + numDots * 4));
			USART1_Send_String(buf);
			// Stupid delay for correct work flash
			for(s32 i = 0; i < 400000; i++);
		}

		// Start work with new parameters
		StartWork();

		break;

	case 2:
		/* Read old parameters from flash and start working */
		USART1_Send_String("\r\n\r\n> Mode 2 is selected \r\n\r\n");

		StartWork();

		break;

	default:
		USART1_Send_String("\r\n\r\nInvalid command!\r\n\r\n");
		for(;;);
		break;
	}
}

void StartWork(void) {

	// Read number of dots
	u8 cycles = FLASH_Read(page31);

	/* ! PRINT DATA ON DISPLAY 1602
	 *
	 *
	 */

	for(u8 i = 0; i < cycles; i++) {

		// Читаем параметры точки и уходим в while(время первой точки)
		u32 setting_time = FLASH_Read(i*4 + (page31) + 4);
		u32 setting_temp = FLASH_Read(i*4 + (page31) + 4 + cycles * 4);

		// Запускаем работу первой точки до тех пор, пока не кончится время
		while(timer * 60 < setting_time) {

			// Здесь поддерживаем заданную температуру
			if(PT100_GetTemp() < setting_temp) {

				GPIOC->ODR |= (1<<HEATER_pin);
			} else {

				GPIOC->ODR &= ~(1<HEATER_pin);
			}
			// Сбрасываем счетчик времени в таймере
		}

		/* После окончания времени запускаем этот же цикл со значением времени второй точки
		 * Повторяем нужное количетство раз (равное количеству точек)
		 */
	}

	// После окончания всех режимов отключаем нагреватель и уходим в бесконечный цикл
	GPIOC->ODR &= ~(1 << HEATER_pin);
	for(;;);
}

void ADC1_init(void) {

	// Set up RCC for ADC1
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	// Set up ADC on PA0 Pin:  CNF0, CNF1, MODE0, MODE1 == 0;
	GPIOA->CRL &= ~(GPIO_CRL_MODE0_0 | GPIO_CRL_MODE0_1 | GPIO_CRL_CNF0_0 | GPIO_CRL_CNF0_1);

	ADC1->SMPR2 |= ADC_SMPR2_SMP0;
	// Set up Continous mode ADC
	ADC1->CR2 |= ADC_CR2_CONT | ADC_CR2_ADON | ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL | ADC_CR2_JEXTSEL;
	ADC1->CR2 |= ADC_CR2_SWSTART;
}

void GPIO_init(void) {

	// Clear PC13 bit (ONBOARD RED LED)
	GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
	// Configure PC13 as Push Pull output at max 10Mhz
	GPIOC->CRH |= GPIO_CRH_MODE13_0;
}

void RCC_init(void) {

	uint32_t StartUpCounter = 0, HSEStatus = 0;

	/* Конфигурация  SYSCLK, HCLK, PCLK2 и PCLK1 */
	/* Включаем HSE */
	RCC->CR |= ((uint32_t)RCC_CR_HSEON);

	/* Ждем пока HSE не выставит бит готовности либо не выйдет таймаут*/
	do {

		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;
	}
	while((HSEStatus == 0) && (StartUpCounter != HSEStartUp_TimeOut));

	if ((RCC->CR & RCC_CR_HSERDY) != RESET) {

		HSEStatus = (uint32_t)0x01;
	} else {

		HSEStatus = (uint32_t)0x00;
	}

	/* Если HSE запустился нормально */
	if (HSEStatus == (uint32_t)0x01)
	{
		/* Включаем буфер предвыборки FLASH */
		FLASH->ACR |= FLASH_ACR_PRFTBE;

		/* Конфигурируем Flash на 2 цикла ожидания 												*/
		/* Это нужно потому, что Flash не может работать на высокой частоте 					*/
		/* если это не сделать, то будет странный глюк. Проц может запуститься, но через пару 	*/
		/* секунд повисает без "видимых причин". Вот такие вот неочевидные вилы. 				*/
		FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;

		/* HCLK = SYSCLK 															*/
		/* AHB Prescaler = 1 														*/
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;

		/* PCLK2 = HCLK 															*/
		/* APB2 Prescaler = 1 														*/
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;

		/* PCLK1 = HCLK 															*/
		/* APB1 Prescaler = 2 														*/
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;

		/* Конфигурируем множитель PLL configuration: PLLCLK = HSE * 9 = 72 MHz 	*/
		/* При условии, что кварц на 8МГц! 										*/
		/* RCC_CFGR_PLLMULL9 - множитель на 9. Если нужна другая частота, не 72МГц 	*/
		/* то выбираем другой множитель. 											*/

		/* Сбрасываем в нули прежнее значение*/
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));

		/* А теперь накатываем новое 												*/
		/* RCC_CFGR_PLLSRC_HSE -- выбираем HSE на вход 								*/
		/* RCC_CFGR_PLLMULL9 -- множитель 9											*/
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);

		/* Все настроили? Включаем PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Ожидаем, пока PLL выставит бит готовности */
		while((RCC->CR & RCC_CR_PLLRDY) == 0) {
			// Ждем
		}

		/* Работает? Можно переключать! Выбираем PLL как источник системной частоты */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

		/* Ожидаем, пока PLL выберется как источник системной частоты */
		while((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) {
			// Ждем
		}
	} else {

		/* Все плохо... HSE не завелся... Чего-то с кварцем или еще что...
	 	Надо бы както обработать эту ошибку... Если мы здесь, то мы работаем
	 	от HSI! */
	}



	// Set up RCC on USART1 & GPIOA & GPIOC, set up alt func., set up RCC on ADC1
	RCC->APB2ENR|= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
	// Set up RCC on TIM2
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
}

void TIM6_init(void) {

	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	// Prescaler 1000
	TIM3->PSC = SystemCoreClock / 1000 - 1;
	// Max value to count
	TIM3->ARR = 10000 - 1;
	// Let the timer generate events
	TIM3->DIER |= TIM_DIER_UIE;
	// Enable counter
	TIM3->CR1 |= TIM_CR1_CEN;
	// Enable interrupts
	NVIC_EnableIRQ(TIM3_IRQn);
}

u8 PT100_GetTemp(void) {

	// При 0С = 720, при 220С = 4095.
	return map(ADC1->DR, 720, 4095, 0, 220);
}

float map(float x, float in_min, float in_max, float out_min, float out_max) {

	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

