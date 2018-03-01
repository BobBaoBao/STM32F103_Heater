/*
 * lcd.h
 *
 *  Created on: Mar 1, 2018
 *      Author: anykey
 */

#ifndef LCD_H_
#define LCD_H_

#include "main.h"
#include "stm32f10x.h"

void delay(int a);
void PulseLCD(void); // Строб
void SendByte(char ByteToSend, int IsData);
void SetCursor(char Row, char Col); // Установка курсора
void ClearLCDScreen(); // Очистка диспля
void InitializeLCD(void); // Инициализация дисплея
void PrintStr(char *Text); // Печать строки

#endif /* LCD_H_ */
