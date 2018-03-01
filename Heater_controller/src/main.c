#include "main.h"

#include "flash.h"
#include "usart.h"
#include "RTC.h"
#include "lcd.h"

// Массив для отправки в USART
char buf[100];
// Массивы для хранения параметров точек
u32 setting_time[100] = {0};
u32 setting_temp[100] = {0};
// Переменная хранения количества точек
u8 cycles = 0;
// Переменная хранения времени
u8 timer = 0;

// Счетчик принятых байт по USART
u8 receivedDataCounter = 0;
// Массив для приема по USART
char receivedData[100] = {0};

// Флаг отображающий прием новых данных
bool newParams = false;

int main(void) {

	RCC_init();
	USART_init();
	ADC1_init();
	RTC_Init();
	GPIO_init();

    InitializeLCD();
    ClearLCDScreen();
    SetCursor(0,2);
    PrintStr("test");
    SetCursor(1,4);
    PrintStr("test");

	// Запускаем работу
	StartWork();
}

// Обработчик прерываний от USART1
void USART1_IRQHandler(void) {

	// Выясняем, какое именно событие вызвало прерывание. Если это приём байта в RxD - обрабатываем.
	if (USART1->SR & USART_SR_RXNE) {

		// Сбрасываем флаг прерывания
		USART1->SR &=~USART_SR_RXNE;

		// Принимаем данные и пишем в буфер
		receivedData[receivedDataCounter]  = USART1->DR;

		// Приняли, увеличиваем значение счетчика
		receivedDataCounter ++;
	}

	// Если прилетел символ новой строки - посылка получена - можно читать
	if(USART1->DR == '\n') {

		// Проверяем CRC - если все ок идем дальше

		// Записываем новые параметры точек во флеш
		WriteNewParams(receivedData);
	}
}

// Обработчик прерываний от RTC
void RTC_IRQHandler(void) {

	// Прерывание по прошествии секунды
	if(RTC->CRL & RTC_CRL_SECF) {

		// Сбрасываем флаг
		RTC->CRL &= ~RTC_CRL_SECF;
		// Увеличиваем переменную времени
		timer++;
	}
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

	// Включаем RCC для USART 1, GPIOA GPIOC, альтернативные функции, АЦП и CRC
	RCC->APB2ENR|= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
	RCC->AHBENR |= RCC_AHBENR_CRCEN;


}

void StartWork(void) {

	// Читаем количество точек
	cycles = FLASH_Read(page32);

	// Читаем параметры точек, заполняем массивы параметров
	for(u8 i = 0; i < cycles; i++) {

		setting_temp[i] = FLASH_Read(i*4 + (page31) + 4);
		setting_time[i] = FLASH_Read(i*4 + (page32) + 4);
	}

	// Сбрасываем флаг приема новых данных
	newParams = false;

	// Запускаем работу
	for(u8 i = 0; i < cycles; i++) {

		// Выводим информацию о текущем режиме на дисплей
		ClearLCDScreen();
		SetCursor(0, 2);
		sprintf(buf, "SetTemp: %ld", setting_temp[i]);
		PrintStr(buf);
		SetCursor(1,4);
		sprintf(buf, "SetTime: %ld", setting_time[i]);
		PrintStr("");


		// Запускаем работу первой точки до тех пор, пока не кончится время
		while(timer < (setting_time[i]*60)) {

			// Если прилетели новые данные - выходим из цикла, читаем новые параметры - запускаем цикл
			if(newParams) break;

			// Здесь поддерживаем заданную температуру
			if(PT100_GetTemp() < setting_temp[i]) {

				//GPIOC->ODR |= (1<<HEATER_pin);
				GPIOC->ODR |= (1<<ERROR_pin);
			}
			else {

				//GPIOC->ODR &= ~(1<HEATER_pin);
				GPIOC->ODR &= ~(1<<ERROR_pin);
			}

			// Печатаем "температру" для отладки
			sprintf(buf, "Точка№ %d, Температура: %ld, Уставка: %ld| Текущее время: %d, Уставка: %ld\r\n", i, PT100_GetTemp(),
					setting_temp[i], timer, setting_time[i]*60);

			USART1_Send_String(buf);
			// Тупая задержка
			for(u32 i = 0; i <4000000; i++);
		}

		// Сбрасываем переменную времени и запускаем следующую точку
		timer = 0;

	}

	if (newParams) {

		// Если пришли новые параметры рекурсивно вызываем сами себя и запускаем работу с новыми параметрами
		StartWork();
	}
	else {
		// Если мы попали сюда, значит режим окончил свою работу. Выключаем нагреватель, уходим в бесконечность
		//GPIOC->ODR &= ~(1<<HEATER_pin);
		GPIOC->ODR &= ~(1<<ERROR_pin);
		USART1_Send_String("\r\n Работа окончена!\r\n");
		for(;;);
	}
}

u32 PT100_GetTemp(void) {

	// При 0С = 720, при 220С = 4095.
	return map(ADC1->DR, 720, 4095, 0, 220);
}

float map(float x, float in_min, float in_max, float out_min, float out_max) {

	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void WriteNewParams(const char *str) {

	// Возможно стоит на время записи данных запретить прерывания ?

	/* Парсим принятую посылку:
	 * Читаем нулевой байт. Если "a" - посылка для нас - читаем следующий байт;
	 * Читаем первый байт - параметр. Если "1" - новые значения температур точек, если "2" - новые значения времени точек;
	 * Читаем второй и третий байт - количество точек. От этого числа определяем, сколько байт читать дальше;
	 * Каждый параметр (температура или время) занимает 3 байта)).
	 */

	char ptr[2] = {0};
	char val[3] = {0};

	// Записываем новые параметры во флеш
	FLASH_unlock();

	// Нулевой байт, если "а" - посылка для нас, читаем дальше
	if (receivedData[0] == 'a') {

		// Первый байт, определяем тип принятых данных
		switch(receivedData[1]) {

		// Если "1" - новые значения температур
		case '1':
			// Второй и третий байт - количество точек
			memcpy(ptr, &receivedData[2], 2);
			cycles = atoi(ptr);
			FLASH_Erase_Page(page31);
			FLASH_Write(page31, (u32)cycles);

			for(u8 i = 0; i < cycles; i++) {

				// Читаем байты через 3 (значение температуры занимает 3 байта, лол)
				memcpy(val, &receivedData[4 + i*3], 3);
				setting_temp[i] = atoi(val);

				// Запись параметра температуры во флеш после записи о номере точек (adress page31 + 4byte)
				FLASH_Write(i*4 + (page31 + 4), (u32)setting_temp[i]);
			}

			break;


		// Если "2" - новые значения времени точек
		case '2':
			// Второй и третий байт - количество точек
			memcpy(ptr, &receivedData[2], 2);
			cycles = atoi(ptr);
			FLASH_Erase_Page(page32);
			FLASH_Write(page32, (u32)cycles);

			for(u8 i = 0; i < cycles; i++) {

				memcpy(val, &receivedData[4 + i*3], 3);
				setting_time[i] = atoi(val);

				// Запись параметра времени во флеш
				FLASH_Write(i*4 + (page32 + 4), (u32)setting_time[i]);
			}

			break;

		default:
			Error_Handler();
		}

	} else Error_Handler();;


	// Обнуляем счетчик количества принятых байт и массив принятых байт
	for(u8 i = 0; i <= 100; i++) {
		receivedData[i] = 0;
	}
	receivedDataCounter  = 0;


	// Печатаем параметры точек (для отладки)
	USART1_Send_String("Params in flash:\r\n\r\n");
	sprintf(buf, "Number of dots: %ld\r\n\r\n", (u32)cycles);
	USART1_Send_String(buf);
	for(u8 i = 0; i < cycles; i++) {

		sprintf(buf, "Temp %d: %ld C,\tTime %d: %ld min\r\n", i,
				FLASH_Read(i*4 + (page31) + 4), i,
				FLASH_Read(i*4 + (page32) + 4));
		USART1_Send_String(buf);
	}


	newParams = true;
	/* Возможно стоит добавить какой-то флаг, чтобы работа приостановилась, если запущена */
}

void Error_Handler(void) {

	// Аварийно выключаем нагреватель
	GPIOA->ODR &= ~(1<<HEATER_pin);

	for(;;) {
		// Мигаем, сигнализация об ошибке
		GPIOC->ODR ^= (1<<ERROR_pin);
		for(u32 i = 0; i < 200000; i++);
	}

}

