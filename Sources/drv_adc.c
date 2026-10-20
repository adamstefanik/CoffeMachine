/*
 * @file     drv_adc.c
 * @brief    Implementace ovladace ADC0 pro potenciometr na kitu FRDM-KL25Z
 */

#include "MKL25Z4.h"
#include "drv_adc.h"

/* Vychozi kanal potenciometru (PTB0 = ADC0_SE8) */
static uint8_t g_adc_pot_channel = ADC_CH_PTB0;

/* Inicializace prevodniku ADC0 */
void ADC_Init(void)
{
    /* 1. Povoleni hodin pro ADC0 a prislusne porty B a C */
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
    SIM->SCGC5 |= (SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK);

    /* 2. Nastaveni analogove funkce pinu (MUX = 0 vypina digitalni vstupni buffer) */
    PORTB->PCR[0] &= ~PORT_PCR_MUX_MASK; /* PTB0 (ADC0_SE8) */
    PORTB->PCR[1] &= ~PORT_PCR_MUX_MASK; /* PTB1 (ADC0_SE9) */

    /* 3. Konfigurace ADC0:
     *    - 12-bitove rozliseni (MODE = 1)
     *    - Hodinovy signal z Bus clock (ADICLK = 0)
     *    - Preddelicka 2 (ADIV = 1)
     *    - Dlouhy cas vzorkovani (ADLSMP = 1) pro stabilni mereni analogoveho vstupu
     */
    ADC0->CFG1 = ADC_CFG1_MODE(1) |
                 ADC_CFG1_ADIV(1) |
                 ADC_CFG1_ADLSMP_MASK;

    /* 4. Konfigurace softwaroveho spousteni (SC2 = 0) */
    ADC0->SC2 = 0U;

    /* 5. Zapnuti hardwaroveho prumerovani 16 vzorku pro eliminaci ruseni a kmitani hodnoty */
    ADC0->SC3 = ADC_SC3_AVGE_MASK | ADC_SC3_AVGS(2);
}

/* Nastaveni primarniho kanalu potenciometru */
void ADC_SetPotentiometerChannel(uint8_t channel)
{
    g_adc_pot_channel = channel;
}

/* Ziskani aktualniho kanalu potenciometru */
uint8_t ADC_GetPotentiometerChannel(void)
{
    return g_adc_pot_channel;
}

/* Precte surovou hodnotu ze zadaneho kanalu ADC0 (12-bit: 0 az 4095) */
uint16_t ADC_ReadChannelRaw(uint8_t channel)
{
    /* Spusteni jednorazove konverze zapisem kanalu do SC1A */
    ADC0->SC1[0] = ADC_SC1_ADCH(channel);

    /* Cekani na dokonceni konverze (priznak COCO) */
    while ((ADC0->SC1[0] & ADC_SC1_COCO_MASK) == 0U) {
        /* Nekolika-mikrosekundove cekani na hardware ADC */
    }

    /* Precteni a vraceni vysledku (ctenim registru R[0] se zaroven nuluje priznak COCO) */
    return (uint16_t)(ADC0->R[0]);
}

/* Precte polohu potenciometru v rozsahu 0 az 255 (8-bit) */
uint8_t ADC_ReadPotentiometer8(void)
{
    uint16_t raw = ADC_ReadChannelRaw(g_adc_pot_channel);
    /* Prevod 12-bit hodnoty (0 az 4095) na 8-bit (0 az 255) */
    return (uint8_t)(raw >> 4);
}

/* Prepocita polohu potenciometru na uroven intenzity kavy (1 az 3: H3, H2, H1) */
uint8_t ADC_GetCoffeeLevel(void)
{
    uint8_t val = ADC_ReadPotentiometer8();

    if (val < 85U) {
        return 1U; /* Slaba kava (po hladinomer H3) */
    } else if (val < 170U) {
        return 2U; /* Stredni kava (po hladinomer H2) */
    } else {
        return 3U; /* Silna kava (po hladinomer H1) */
    }
}

/* Prepocita polohu potenciometru na uroven cukru (0 az 3: bez cukru, H8, H7, H6) */
uint8_t ADC_GetSugarLevel(void)
{
    uint8_t val = ADC_ReadPotentiometer8();

    if (val < 64U) {
        return 0U; /* 0 = Bez cukru */
    } else if (val < 128U) {
        return 1U; /* 1 = Malo cukru (H8) */
    } else if (val < 192U) {
        return 2U; /* 2 = Stredne cukru (H7) */
    } else {
        return 3U; /* 3 = Hodne cukru (H6) */
    }
}

/* Vygeneruje progress bar ve formatu "[###   ]" do textoveho retezce */
void ADC_FormatProgressBar(uint8_t current_level, uint8_t max_level, char *buffer, uint8_t bar_width)
{
    if (buffer == 0 || bar_width == 0) {
        return;
    }

    if (max_level == 0) {
        max_level = 1;
    }

    if (current_level > max_level) {
        current_level = max_level;
    }

    uint8_t filled = (uint8_t)(((uint32_t)current_level * bar_width) / max_level);
    uint8_t idx = 0;

    buffer[idx++] = '[';
    for (uint8_t i = 0; i < bar_width; i++) {
        if (i < filled) {
            buffer[idx++] = '#';
        } else {
            buffer[idx++] = ' ';
        }
    }
    buffer[idx++] = ']';
    buffer[idx] = '\0';
}
