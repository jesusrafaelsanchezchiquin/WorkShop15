/*
 * ds3231.h
 *
 *  Created on: 1 nov. 2020
 *      Author: jeras_7wkqxva
 */

#ifndef MAIN_INCLUDE_DS3231_H_
#define MAIN_INCLUDE_DS3231_H_

#include <stdio.h>// para poder usar uint8_t
void tareaDS3231(void* P);
char* leer_ds3231(uint8_t* seg, uint8_t* min, uint8_t* hor,
		uint8_t* fec , uint8_t* mes, uint8_t* ani, uint8_t* ampm);


#endif /* MAIN_INCLUDE_DS3231_H_ */
