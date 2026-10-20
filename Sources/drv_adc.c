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
    /* 1. Povoleni hodin pro ADC0 a porty B a C */
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
    SIM->SCGC5 |= (SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK);

    /* 2. Nastaveni analogove funkce pinu PTB0 a PTB1 (MUX = 0) */
    PORTB->PCR[0] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[1] &= ~PORT_PCR_MUX_MASK;

    /* 3. Konfigurace ADC0:
     *    - 8-bitovy rezim (MODE = 0) pro bleskovy a stabilni prevod
     *    - Hodiny ze sbernice Bus clock (ADICLK = 0)
     *    - Preddelicka 4 (ADIV = 2)
     *    - Dlouhy cas vzorkovani (ADLSMP = 1)
     */
    ADC0->CFG1 = ADC_CFG1_MODE(0) |
                 ADC_CFG1_ADIV(2) |
                 ADC_CFG1_ADLSMP_MASK;

    /* 4. Softwarove spousteni (SC2 = 0) */
    ADC0->SC2 = 0U;

    /* 5. Vypnute hardwarove prumerovani pro okamzitou odezvu bez cekani */
    ADC0->SC3 = 0U;
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

/* Precte surovou hodnotu ze zadaneho kanalu ADC0 (8-bit: 0 az 255) s timeoutem */
uint16_t ADC_ReadChannelRaw(uint8_t channel)
{
    /* Spusteni jednorazoveho prevodu na zadanem kanalu */
    ADC0->SC1[0] = ADC_SC1_ADCH(channel);

    /* Ochrana proti zablokovani (timeout cyklus cca 5 ms) */
    uint32_t timeout = 50000U;
    while (((ADC0->SC1[0] & ADC_SC1_COCO_MASK) == 0U) && (timeout > 0U)) {
        timeout--;
    }

    /* Pokud doslo k timeoutu, vrat 0 */
    if (timeout == 0U) {
        return 0U;
    }

    /* Ctenim R[0] se zaroven vynuluje priznak COCO */
    return (uint16_t)(ADC0->R[0]);
}

/* Precte polohu potenciometru v rozsahu 0 az 255 */
uint8_t ADC_ReadPotentiometer8(void)
{
    return (uint8_t)ADC_ReadChannelRaw(g_adc_pot_channel);
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
