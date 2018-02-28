/*
 * flash.c
 *
 *  Created on: Feb 27, 2018
 *      Author: Denis Denk
 *      Library to use flash
 */

#include "flash.h"

uint8_t FLASH_Ready(void) {

	return !(FLASH->SR & FLASH_SR_BSY);
}

void FLASH_Write(uint32_t address, uint32_t data) {

	// Acess to program flash
	FLASH->CR |= FLASH_CR_PG;
	// Waiting flash to write data
	while(!FLASH_Ready());

	// Write 2 bytes (low)
	*(__IO uint16_t*)address = (uint16_t)data;
	while(!FLASH_Ready());
	address+=2;
	data>>=16;

	// Write 2 bytes (high)
	*(__IO uint16_t*)address = (uint16_t)data;
	while(!FLASH_Ready());
	FLASH->CR &= ~(FLASH_CR_PG);
}

void FLASH_Erase_Page(uint32_t address) {

	// Set the bit to erase one page
	FLASH->CR|= FLASH_CR_PER;
	// Set the page for erasing
	FLASH->AR = address;
	// Start erasing
	FLASH->CR|= FLASH_CR_STRT;
	// Wait while the page is erased
	while(!FLASH_Ready());
	// Reset the bit back
	FLASH->CR&= ~FLASH_CR_PER;
}

uint32_t FLASH_Read(uint32_t address) {

	return (*(__IO uint32_t*)address);
}

void FLASH_unlock(void) {

  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;

}



