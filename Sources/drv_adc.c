/*
 * @file     drv_adc.c
 * @brief    Implementace ovladace ADC0 pro potenciometr na kitu FRDM-KL25Z
 */

#include "MKL25Z4.h"
#include "drv_adc.h"
#include "drv_systick.h"

#define POT_SAMPLE_INTERVAL_MS   (20U) /* Perioda vzorkovani potenciometru v ms */

/* Vychozi kanal potenciometru (PTB0 = ADC0_SE8) */
static uint8_t g_adc_pot_channel = ADC_CH_PTB0;

/* Filtrovana hodnota a aktualni urovne s hysterezi */
static uint8_t s_cached_pot_raw = 128U;
static uint8_t s_cached_coffee_level = 2U;
static uint8_t s_cached_sugar_level = 0U;
static uint32_t s_last_sample_ms = 0U;

/* Pomocna funkce pro aktualizaci stavu urovne kavy s hysterezi (±5 LSB) */
static uint8_t ADC_ComputeCoffeeLevelWithHysteresis(uint8_t current_level, uint8_t raw_val)
{
    switch (current_level) {
    case 1U: /* Slaba kava (H3) */
        if (raw_val >= 90U) { /* Prah 85 + 5 */
            return (raw_val >= 175U) ? 3U : 2U;
        }
        return 1U;

    case 2U: /* Stredni kava (H2) */
        if (raw_val <= 80U) { /* Prah 85 - 5 */
            return 1U;
        } else if (raw_val >= 175U) { /* Prah 170 + 5 */
            return 3U;
        }
        return 2U;

    case 3U: /* Silna kava (H1) */
        if (raw_val <= 165U) { /* Prah 170 - 5 */
            return (raw_val <= 80U) ? 1U : 2U;
        }
        return 3U;

    default:
        return 2U;
    }
}

/* Pomocna funkce pro aktualizaci stavu urovne cukru s hysterezi (±4 LSB) */
static uint8_t ADC_ComputeSugarLevelWithHysteresis(uint8_t current_level, uint8_t raw_val)
{
    switch (current_level) {
    case 0U: /* Bez cukru */
        if (raw_val >= 68U) { /* Prah 64 + 4 */
            return (raw_val >= 196U) ? 3U : ((raw_val >= 132U) ? 2U : 1U);
        }
        return 0U;

    case 1U: /* Malo cukru (H8) */
        if (raw_val <= 60U) { /* Prah 64 - 4 */
            return 0U;
        } else if (raw_val >= 132U) { /* Prah 128 + 4 */
            return (raw_val >= 196U) ? 3U : 2U;
        }
        return 1U;

    case 2U: /* Stredne cukru (H7) */
        if (raw_val <= 124U) { /* Prah 128 - 4 */
            return (raw_val <= 60U) ? 0U : 1U;
        } else if (raw_val >= 196U) { /* Prah 192 + 4 */
            return 3U;
        }
        return 2U;

    case 3U: /* Hodne cukru (H6) */
        if (raw_val <= 188U) { /* Prah 192 - 4 */
            return (raw_val <= 60U) ? 0U : ((raw_val <= 124U) ? 1U : 2U);
        }
        return 3U;

    default:
        return 0U;
    }
}

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

    /* 6. Pocatecni nacteni polohy potenciometru pro okamzitou dostupnost platnych dat */
    uint8_t init_raw = (uint8_t)ADC_ReadChannelRaw(g_adc_pot_channel);
    s_cached_pot_raw = init_raw;
    s_cached_coffee_level = (init_raw < 85U) ? 1U : ((init_raw < 170U) ? 2U : 3U);
    s_cached_sugar_level = (init_raw < 64U) ? 0U : ((init_raw < 128U) ? 1U : ((init_raw < 192U) ? 2U : 3U));
    s_last_sample_ms = 0U;
}

/* Neblokujici periodicka uloha pro vzorkovani a filtraci potenciometru (~20 ms) */
void Task_Potentiometer(void)
{
    uint32_t now = SYSTICK_millis();

    /* Vykonavej pouze pokud uplynula perioda vzorkovani */
    if ((now - s_last_sample_ms) < POT_SAMPLE_INTERVAL_MS) {
        return;
    }
    s_last_sample_ms = now;

    /* Precti surovou 8-bitovou hodnotu z ADC */
    uint8_t raw = (uint8_t)ADC_ReadChannelRaw(g_adc_pot_channel);

    /* Exponencialni vyhlazovaci filtr (IIR filtr 1. radu): 75% historie + 25% novy vzorek */
    s_cached_pot_raw = (uint8_t)(((uint16_t)s_cached_pot_raw * 3U + raw) / 4U);

    /* Aktualizace urovni s hysterezi pro zamezeni kmitani na hranicich segmentu */
    s_cached_coffee_level = ADC_ComputeCoffeeLevelWithHysteresis(s_cached_coffee_level, s_cached_pot_raw);
    s_cached_sugar_level  = ADC_ComputeSugarLevelWithHysteresis(s_cached_sugar_level, s_cached_pot_raw);
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

/* Precte polohu potenciometru v rozsahu 0 az 255 (vraci filtrovanou hodnotu) */
uint8_t ADC_ReadPotentiometer8(void)
{
    return s_cached_pot_raw;
}

/* Prepocita polohu potenciometru na uroven intenzity kavy (1 az 3: H3, H2, H1) */
uint8_t ADC_GetCoffeeLevel(void)
{
    return s_cached_coffee_level;
}

/* Prepocita polohu potenciometru na uroven cukru (0 az 3: bez cukru, H8, H7, H6) */
uint8_t ADC_GetSugarLevel(void)
{
    return s_cached_sugar_level;
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
