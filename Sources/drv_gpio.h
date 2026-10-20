/*
 * @file     drv_gpio.h
 * @brief    Simple GPIO driver for LED and push buttons Freescale KL25Z
 * @version  V1.00
 * @date     17. July 2015
 *
 * @note
 *
 * Ukazkovy program pro Programovani mikropocitacu
 * Ovladac pro LED a tlacitka. Umoznuje ovladat tlacitka na vyvojovem kitu
 * s FRDM-KL25Z ve stylu jako Arduino funkce digitalRead, digitalWrite and pinMode.
 *
 * Pozor: toto neni priklad univerzalniho ovladace pro mapovani nejakych cisel/nazvu pinu
 * na fyzicke piny mikrokontroleru. Piny jsou mapovany primitivne, natvrdo pomoc switch.
 * Nazvy funkci neodpovidaji doporucene konvenci {nazev_ovladace}_{nazev_funkce}, aby byla
 * zajistena jejich podobnost funkcim Arduina, ktere jsou obecne zname.
 *
 * Vyjimky z doporucenych konvenci kvuli strucnejsimu zapisu programu:
 * - funkce pinWrite, pinMode, pinRead nezacinaji predponou modulu (GPIO)
 * - nazvy polozek vyctovych typu (enum) neobsahuji nazev modulu (GPIO).
 *
 *
 */
#ifndef	UTBFRDM_DRV_GPIO_H
#define UTBFRDM_DRV_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif


/** \ingroup  UTB_FRDM_Drivers
    \defgroup UTB_FRDM_NewDriver New Driver for FRDM-KL25Z
  @{
 */

/** Available pins
 * Hodnoty prvku jsou cisla pinu. Port neni resen, ten musi byt "hard-coded" ve funkcich.
 * */
typedef enum _utb_frdm_gpio_pins
{
	INVALID_PIN = 0xFF,
	LD1 = 8,
	LD2 = 9,
	LD3 = 10,
	LED_RED = 18,
	LED_GREEN = 19,
	SW1 = 4,
	SW2 = 5,
	SW3 = 16,
	SW4 = 17,
	LAST_PIN,
} FRDM_kit_pin;

/** Mozne hodnoty zapisovane na pin */
#define	HIGH	(1)	/* log. 1 */
#define LOW		(0) /* log. 0 */

/* Mozne rezimy pinu */
typedef enum _utb_frdm_pin_modes
{
	INPUT,
	OUTPUT,
	INPUT_PULLUP,
} FRDM_kit_pinmode;



 /**
  * @brief Initialize the gpio driver for LEDs and push buttons.
  * @param param Must be one of the values defined in UART0_baudrate enum!
  * @return none
  * @note It will enable the clock for the ports as required.
  */
void GPIO_Initialize(void);

/**
  * @brief Configure given pin to behave as input or output.
  * @param pin Pin to configure. Must be one of the values defined in FRDM_kit_pin enum!
  * @param mode Mode to use for the pin. Can be INPUT, OUTPUT or INPUT_PULLUP.
  * @return none
  * @note
  */
void pinMode(FRDM_kit_pin pin, FRDM_kit_pinmode mode );

/**
  * @brief Set value for given pin. The pin must be configured as output with pinMode first!
  * @param pin Pin to set. Must be one of the values defined in FRDM_kit_pin enum!
  * @param value Value to write to the pin. The value can be HIGH or LOW.
  * @return none
  * @note
  */
void pinWrite(FRDM_kit_pin pin, uint8_t value );

/**
  * @brief  Read value on given pin. The pin must be configured as input with pinMode first!
  * @param pin Pin to read from. Must be one of the values defined in FRDM_kit_pin enum!
  * @return HIGH or LOW value depending on the voltage level on the pin.
  * @note
  */
uint8_t pinRead(FRDM_kit_pin pin);




/*@} end of UTB_FRDM_NewDriver */


#ifdef __cplusplus
}
#endif

#endif	/* UTBFRDM_DRV_ name _H */

