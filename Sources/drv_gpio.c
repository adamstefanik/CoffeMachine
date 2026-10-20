/*
 * Ukazkovy program pro Programovani mikropocitacu
 * Ovladac pro LED a tlacitka na kitu s FRDM-KL25Z.
 *
 * Detailni dokumentace je v .h souboru.
 *
 */

#include "MKL25Z4.h"
#include "drv_gpio.h"



/*  Internal functions */
static void f_GPIO_set_pin_mode(PORT_Type* port, GPIO_Type* gpio, uint8_t pin, FRDM_kit_pinmode mode );

/* Example of internal constants*/
/*const char UTB_UART_CR = (const char)0x0D;*/


/* Initialize the gpio driver for LEDs and push buttons. */
void GPIO_Initialize(void)
{
	// Enable clock for ports A and B
	SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK );

	// Configure SW1 to GPIO mode; it is by default NMI (non-maskable interrupt)
	// and if we do not provide handler for NMI and do not configure the button,
	// then the program will end in Default ISR when SW1 is pressed.
	PORTA->PCR[4] = PORT_PCR_MUX(1); /* set pin A4 to GPIO mode */
}

/* Configure given pin to behave as input or output. */
void pinMode(FRDM_kit_pin pin, FRDM_kit_pinmode mode )
{
	switch (pin)
	{
		/* All LEDs are on port B */
		case LD1:
		case LD2:
		case LD3:
		case LED_RED:
		case LED_GREEN:
			PORTB->PCR[(uint8_t) pin] = PORT_PCR_MUX(1); /* set pin to GPIO mode */
			f_GPIO_set_pin_mode(PORTB, PTB, (uint8_t) pin, mode); /* pin mode to input/output */
			break;

		case SW1:
		case SW2:
		case SW3:
		case SW4:
			PORTA->PCR[(uint8_t) pin] = PORT_PCR_MUX(1); /* set pin to GPIO mode */
			f_GPIO_set_pin_mode(PORTA, PTA, (uint8_t) pin, mode); /* pin mode to input/output */
			break;

		default:
			while (1) ; 	/* Error: invalid pin! */
	}
}

/* Set value for given pin. The pin must be configured as output with pinMode first! */
void pinWrite(FRDM_kit_pin pin, uint8_t value )
{
	switch(pin)
	{
		/* All LEDs are on port B */
		case LD1:
		case LD2:
		case LD3:
		case LED_RED:
		case LED_GREEN:
			if ( value )
				PTB->PSOR |= (1 << (uint8_t)pin);  /* set pin to HIGH  */
			else
				PTB->PCOR |= (1 << (uint8_t)pin);  /* clear pin (set LOW)  */
			break;

		case SW1:
		case SW2:
		case SW3:
		case SW4:
			if ( value )
				PTA->PSOR |= (1 << (uint8_t)pin);  /* set pin to HIGH  */
			else
				PTA->PCOR |= (1 << (uint8_t)pin);  /* clear pin (set LOW)  */
			break;

		default:
			while(1) ;	/* Error: invalid pin! */
	}
}

/* Read value on given pin. The pin must be configured as input with pinMode first! */
uint8_t pinRead(FRDM_kit_pin pin)
{
	uint8_t retVal = LOW;


	switch(pin)
	{
		/* All LEDs are on port B */
		case LD1:
		case LD2:
		case LD3:
		case LED_RED:
		case LED_GREEN:
			if ((PTB->PDIR & (1 << (uint8_t)pin)) == 0)
				retVal = LOW;
			else
				retVal = HIGH;
			break;

		case SW1:
		case SW2:
		case SW3:
		case SW4:
			if ((PTA->PDIR & (1 << (uint8_t)pin)) == 0)
				retVal = LOW;
			else
				retVal = HIGH;
			break;

		default:
			while(1) ;	/* Error: invalid pin! */
	}

	return retVal;
}



/*internal function
 */
static void f_GPIO_set_pin_mode(PORT_Type* port, GPIO_Type* gpio, uint8_t pin, FRDM_kit_pinmode mode )
{
	switch( mode)
	{
	case INPUT:
		gpio->PDDR &= ~(1 << pin);				/* set direction to input*/
		port->PCR[pin] &= ~PORT_PCR_PE_MASK; 	/* disable pull resistor */
		break;

	case OUTPUT:
		gpio->PDDR |= (1 << pin);		/* set direction to output */
		break;

	case INPUT_PULLUP:
		gpio->PDDR &= ~(1 << pin);		/* set direction to input*/
		/* select pull UP and enable it */
		port->PCR[pin] |= (PORT_PCR_PS_MASK | PORT_PCR_PE_MASK);
		break;

	default:
		while(1);	/* Error: invalid pin mode */
	}
}

