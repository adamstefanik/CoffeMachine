/*
 * @file     drv_kava.c
 * @brief    Implementace ovladace pro model misici jednotky EDU-mod (Automat na kavu)
 */

#include "MKL25Z4.h"
#include "drv_kava.h"

/* ============================================================================
 * Definice cisel pinu na jednotlivych portech MCU FRDM-KL25Z
 * ============================================================================ */

/* Vystupy - Ventily SV1 az SV5 a michadlo MIX */
#define PIN_SV1_KAVA	(5U)	/* PTE5 - Ventil SV1 (TANK1 - kava) */
#define PIN_SV2_VODA	(1U)	/* PTC1 - Ventil SV2 (TANK2 - voda) */
#define PIN_SV3_CUKR	(4U)	/* PTE4 - Ventil SV3 (TANK3 - cukr) */
#define PIN_SV4_MIX		(0U)	/* PTE0 - Ventil SV4 (vypousteni TANK1-3 do MIX) */
#define PIN_SV5_VYPUST	(1U)	/* PTE1 - Ventil SV5 (vypousteni MIX do kelimku) */
#define PIN_MICHADLO	(0U)	/* PTD0 - Michadlo v misici nadrzi MIX */

/* Vstupy - Hladinomery H1 az H8 */
#define PIN_H1			(3U)	/* PTD3  - H1 (TANK1 horni) */
#define PIN_H2			(16U)	/* PTC16 - H2 (TANK1 stredni) */
#define PIN_H3			(2U)	/* PTD2  - H3 (TANK1 dolni) */
#define PIN_H4			(5U)	/* PTC5  - H4 (TANK2 horni) */
#define PIN_H5			(4U)	/* PTD4  - H5 (TANK2 dolni) */
#define PIN_H6			(7U)	/* PTC7  - H6 (TANK3 horni) */
#define PIN_H7			(5U)	/* PTD5  - H7 (TANK3 stredni) */
#define PIN_H8			(6U)	/* PTC6  - H8 (TANK3 dolni) */

/* Pomocna funkce pro zapis log. urovne na vystupni GPIO pin */
static void f_Kava_PinWrite(GPIO_Type *gpio, uint32_t pin, bool stav)
{
    if (stav) {
        gpio->PSOR = (1UL << pin); /* Nastav pin na log. 1 (otevri/zapni) */
    } else {
        gpio->PCOR = (1UL << pin); /* Nastav pin na log. 0 (zavri/vypni) */
    }
}

/* Inicializuje vsechny GPIO piny modelu misici jednotky */
void Kava_Init(void)
{
    /* 1. Povoleni hodinoveho signalu pro porty A, B, C, D, E */
    SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK |
                   SIM_SCGC5_PORTB_MASK |
                   SIM_SCGC5_PORTC_MASK |
                   SIM_SCGC5_PORTD_MASK |
                   SIM_SCGC5_PORTE_MASK);

    /* 2. Nastaveni funkce pinu na GPIO (MUX = 1) pro vystupy */
    PORTE->PCR[PIN_SV1_KAVA]   = PORT_PCR_MUX(1);
    PORTC->PCR[PIN_SV2_VODA]   = PORT_PCR_MUX(1);
    PORTE->PCR[PIN_SV3_CUKR]   = PORT_PCR_MUX(1);
    PORTE->PCR[PIN_SV4_MIX]    = PORT_PCR_MUX(1);
    PORTE->PCR[PIN_SV5_VYPUST] = PORT_PCR_MUX(1);
    PORTD->PCR[PIN_MICHADLO]   = PORT_PCR_MUX(1);

    /* 3. Nastaveni funkce pinu na GPIO (MUX = 1) pro vstupy (hladinomery H1-H8) */
    PORTD->PCR[PIN_H1] = PORT_PCR_MUX(1);
    PORTC->PCR[PIN_H2] = PORT_PCR_MUX(1);
    PORTD->PCR[PIN_H3] = PORT_PCR_MUX(1);
    PORTC->PCR[PIN_H4] = PORT_PCR_MUX(1);
    PORTD->PCR[PIN_H5] = PORT_PCR_MUX(1);
    PORTC->PCR[PIN_H6] = PORT_PCR_MUX(1);
    PORTD->PCR[PIN_H7] = PORT_PCR_MUX(1);
    PORTC->PCR[PIN_H8] = PORT_PCR_MUX(1);

    /* 4. Bezpecne uvedeni vsech vystupu do log. 0 pred nastavenim smeru */
    Kava_VypniVse();

    /* 5. Nastaveni smeru vystupnich pinu (PDDR bit = 1) */
    PTE->PDDR |= (1UL << PIN_SV1_KAVA) |
                 (1UL << PIN_SV3_CUKR) |
                 (1UL << PIN_SV4_MIX)  |
                 (1UL << PIN_SV5_VYPUST);
    PTC->PDDR |= (1UL << PIN_SV2_VODA);
    PTD->PDDR |= (1UL << PIN_MICHADLO);

    /* 6. Nastaveni smeru vstupnich pinu hladinomeru (PDDR bit = 0) */
    PTD->PDDR &= ~((1UL << PIN_H1) |
                   (1UL << PIN_H3) |
                   (1UL << PIN_H5) |
                   (1UL << PIN_H7));
    PTC->PDDR &= ~((1UL << PIN_H2) |
                   (1UL << PIN_H4) |
                   (1UL << PIN_H6) |
                   (1UL << PIN_H8));
}

/* Otevre nebo zavre zvoleny ventil SV1 az SV5 */
void Kava_NastavVentil(uint8_t ventil, bool otevrit)
{
    switch (ventil) {
    case VENTIL_KAVA:   /* SV1 - PTE5 */
        f_Kava_PinWrite(PTE, PIN_SV1_KAVA, otevrit);
        break;

    case VENTIL_VODA:   /* SV2 - PTC1 */
        f_Kava_PinWrite(PTC, PIN_SV2_VODA, otevrit);
        break;

    case VENTIL_CUKR:   /* SV3 - PTE4 */
        f_Kava_PinWrite(PTE, PIN_SV3_CUKR, otevrit);
        break;

    case VENTIL_MIX:    /* SV4 - PTE0 */
        f_Kava_PinWrite(PTE, PIN_SV4_MIX, otevrit);
        break;

    case VENTIL_VYPUST: /* SV5 - PTE1 */
        f_Kava_PinWrite(PTE, PIN_SV5_VYPUST, otevrit);
        break;

    default:
        /* Neplatne cislo ventilu - nic neprovadej */
        break;
    }
}

/* Zapne nebo vypne michadlo v misici nadrzi MIX (PTD0) */
void Kava_NastavMichadlo(bool zapnout)
{
    f_Kava_PinWrite(PTD, PIN_MICHADLO, zapnout);
}

/* Zjisti, zda hladina v nadrzi dosahuje po dany snimac H1 az H8 */
bool Kava_CtiHladinu(uint8_t hladinomer)
{
    uint32_t maska = 0U;
    GPIO_Type *port_gpio = 0;

    switch (hladinomer) {
    case HLADINA_H1: /* PTD3 */
        port_gpio = PTD;
        maska = (1UL << PIN_H1);
        break;

    case HLADINA_H2: /* PTC16 */
        port_gpio = PTC;
        maska = (1UL << PIN_H2);
        break;

    case HLADINA_H3: /* PTD2 */
        port_gpio = PTD;
        maska = (1UL << PIN_H3);
        break;

    case HLADINA_H4: /* PTC5 */
        port_gpio = PTC;
        maska = (1UL << PIN_H4);
        break;

    case HLADINA_H5: /* PTD4 */
        port_gpio = PTD;
        maska = (1UL << PIN_H5);
        break;

    case HLADINA_H6: /* PTC7 */
        port_gpio = PTC;
        maska = (1UL << PIN_H6);
        break;

    case HLADINA_H7: /* PTD5 */
        port_gpio = PTD;
        maska = (1UL << PIN_H7);
        break;

    case HLADINA_H8: /* PTC6 */
        port_gpio = PTC;
        maska = (1UL << PIN_H8);
        break;

    default:
        return false;
    }

    return ((port_gpio->PDIR & maska) != 0U);
}

/* Okamzite uzavre vsechny ventily (SV1-SV5) a vypne michadlo (MIX) */
void Kava_VypniVse(void)
{
    PTE->PCOR = (1UL << PIN_SV1_KAVA) |
                (1UL << PIN_SV3_CUKR) |
                (1UL << PIN_SV4_MIX)  |
                (1UL << PIN_SV5_VYPUST);
    PTC->PCOR = (1UL << PIN_SV2_VODA);
    PTD->PCOR = (1UL << PIN_MICHADLO);
}

/* Overi, zda vsechny spodni hladinomery hornich nadrzi (H3, H5, H8) hlasi log. 0 */
bool Kava_JsouHorniNadrzePodMinimem(void)
{
    return (!Kava_CtiHladinu(HLADINA_H3) &&
            !Kava_CtiHladinu(HLADINA_H5) &&
            !Kava_CtiHladinu(HLADINA_H8));
}
