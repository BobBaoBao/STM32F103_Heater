/*
 * flash.h
 *
 *  Created on: Feb 27, 2018
 *      Author: Denis Denk
 *      Library to use flash
 */

#ifndef FLASH_H_
#define FLASH_H_

#include "stm32f10x.h"

#define FLASH_KEY1	((uint32_t)0x45670123)
#define FLASH_KEY2	((uint32_t)0xCDEF89AB)


uint8_t FLASH_Ready(void);
uint32_t FLASH_Read(uint32_t address);
void FLASH_Erase_Page(uint32_t address);
void FLASH_Write(uint32_t address,uint32_t data);
void FLASH_unlock(void);

#endif /* FLASH_H_ */
