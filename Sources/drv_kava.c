#include "MKL25Z4.h"
#include "drv_kava.h"
#include <stdbool.h>



void Kava_Init(void) {
	SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK);

}
