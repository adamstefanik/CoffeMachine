/*
 * @file     drv_adc.h
 * @brief    Ovladac pro ADC0 a cteni potenciometru na vyukovem kitu FRDM-KL25Z
 */

#ifndef DRV_ADC_H_
#define DRV_ADC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standardne analogove kanaly na kitu FRDM-KL25Z */
#define ADC_CH_PTB0		(8U)	/* ADC0_SE8  - pin PTB0 */
#define ADC_CH_PTB1		(9U)	/* ADC0_SE9  - pin PTB1 (analog. port) */
#define ADC_CH_PTC1		(15U)	/* ADC0_SE15 - pin PTC1 */

/**
 * @brief Inicializuje prevodnik ADC0 pro 8-bitovy/12-bitovy prevod s hardwarovym filtrovanim.
 */
void ADC_Init(void);

/**
 * @brief Nastavi, ktery kanal ADC ma byt primarne pouzit pro potenciometr (napr. ADC_CH_PTB0 nebo ADC_CH_PTB1).
 * @param channel Cislo kanalu ADC0 (napr. 8 nebo 9).
 */
void ADC_SetPotentiometerChannel(uint8_t channel);

/**
 * @brief Vraci aktualne zvoleny kanal potenciometru.
 */
uint8_t ADC_GetPotentiometerChannel(void);

/**
 * @brief Precte surovou hodnotu z lubovolneho kanalu ADC0.
 * @param channel Cislo kanalu ADC0.
 * @return 12-bitova hodnota (0 az 4095).
 */
uint16_t ADC_ReadChannelRaw(uint8_t channel);

/**
 * @brief Precte polohu potenciometru v rozsahu 0 az 255 (8-bit).
 * @return Hodnota 0 az 255.
 */
uint8_t ADC_ReadPotentiometer8(void);

/**
 * @brief Prepocita hodnotu potenciometru na uroven intenzity kavy (1 az 3).
 *        1 = slaba kava (hladinomer H3)
 *        2 = stredna kava (hladinomer H2)
 *        3 = silna kava (hladinomer H1)
 * @return Uroven 1, 2 alebo 3.
 */
uint8_t ADC_GetCoffeeLevel(void);

/**
 * @brief Prepocita hodnotu potenciometru na uroven intenzity cukru (0 az 3).
 *        0 = bez cukru
 *        1 = malo cukru (hladinomer H8)
 *        2 = stredne cukru (hladinomer H7)
 *        3 = vela cukru (hladinomer H6)
 * @return Uroven 0, 1, 2 alebo 3.
 */
uint8_t ADC_GetSugarLevel(void);

/**
 * @brief Vygeneruje textovy progress bar do zadaneho retazca (napr. "[###   ]").
 * @param current_level Aktualna uroven.
 * @param max_level     Maximalna uroven.
 * @param buffer        Vystupny buffer (velkost minimalne bar_width + 3).
 * @param bar_width     Pocet znakov vnutri zatvoriek (napr. 6).
 */
void ADC_FormatProgressBar(uint8_t current_level, uint8_t max_level, char *buffer, uint8_t bar_width);

#ifdef __cplusplus
}
#endif

#endif /* DRV_ADC_H_ */
