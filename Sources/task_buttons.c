/*
 * @file     task_buttons.c
 * @brief    Implementacia neblokujucej ulohy (Task_Buttons) pre citanie tlacitiek
 *           SW1 az SW4 s osetrenim zakmitov (debouncing), detekciou jednorazoveho
 *           stlacenia a okamzitym nastavenim priznaku STORNO pri stlaceni SW4.
 * @note     Projekt: Automat na kavu (EDU-mod na FRDM-KL25Z)
 */

#include "MKL25Z4.h"
#include "drv_systick.h"
#include "drv_gpio.h"
#include "task_buttons.h"

/* ============================================================================
 * Interne premenne a mapovanie hardveru
 * ============================================================================ */

/* Mapovanie indexov (0..3) na piny ovladaca drv_gpio */
static const FRDM_kit_pin s_btn_pins[BTN_COUNT] = {
    SW1,    /* PTA4  - Index 0 (BTN_SW1) */
    SW2,    /* PTA5  - Index 1 (BTN_SW2) */
    SW3,    /* PTA16 - Index 2 (BTN_SW3) */
    SW4     /* PTA17 - Index 3 (BTN_SW4) */
};

/* Stabilny (odfiltrovany) stav tlacitiek: true = stlacene, false = uvolnene */
static bool s_btn_state[BTN_COUNT] = { false, false, false, false };

/* Pocitadlo zhodnych vzoriek pre filter zakmitov */
static uint8_t s_debounce_count[BTN_COUNT] = { 0U, 0U, 0U, 0U };

/* Priznak jednorazoveho stlacenia (nabezna hrana: z uvolneneho na stlacene) */
static volatile bool s_btn_clicked[BTN_COUNT] = { false, false, false, false };

/* Priznak okamziteho prerusenia / zrusenia (STORNO) vyvolany tlacidlom SW4 */
static volatile bool s_storno_flag = false;

/* Casova znacka posledneho overenia stavu tlacitiek */
static uint32_t s_last_poll_ms = 0U;

/* ============================================================================
 * Interne pomocne funkcie
 * ============================================================================ */

/**
 * @brief Prevedie vstupny parameter (enum Button_t alebo FRDM_kit_pin) na platny index 0 az 3.
 * @param btn Vstupny identifikator (BTN_SW1..BTN_SW4 alebo SW1..SW4).
 * @return Index 0 az 3 alebo -1 pri neplatnom parametri.
 */
static int f_Button_ToIndex(uint8_t btn)
{
    switch (btn) {
    case BTN_SW1:   /* 0 */
    case SW1:       /* 4 */
        return 0;

    case BTN_SW2:   /* 1 */
    case SW2:       /* 5 */
        return 1;

    case BTN_SW3:   /* 2 */
    case SW3:       /* 16 */
        return 2;

    case BTN_SW4:   /* 3 */
    case SW4:       /* 17 */
        return 3;

    default:
        return -1;
    }
}

/* ============================================================================
 * Implementacia verejnych funkcii
 * ============================================================================ */

/* Inicializuje tlacitka SW1 az SW4 ako vstupy s pull-up rezistormi */
void Task_Buttons_Init(void)
{
    /* 1. Zabezpecenie hodinoveho signalu pre Port A */
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    /* 2. Konfiguracia pinov tlacitiek cez pinMode(..., INPUT_PULLUP) z drv_gpio */
    for (uint8_t i = 0U; i < (uint8_t)BTN_COUNT; i++) {
        pinMode(s_btn_pins[i], INPUT_PULLUP);
    }

    /* 3. Nacitanie pociatocneho fyzickeho stavu (aktivna log. 0 = LOW) */
    for (uint8_t i = 0U; i < (uint8_t)BTN_COUNT; i++) {
        s_btn_state[i] = (pinRead(s_btn_pins[i]) == LOW);
        s_debounce_count[i] = 0U;
        s_btn_clicked[i] = false;
    }

    s_storno_flag = false;
    s_last_poll_ms = SYSTICK_millis();
}

/* Periodicka neblokujuca uloha pre citanie a debouncing tlacitiek */
void Task_Buttons(void)
{
    uint32_t now = SYSTICK_millis();

    /* Neblokujuca kontrola casu: vykonavaj len raz za definovanu periodu (10 ms) */
    if ((now - s_last_poll_ms) < BUTTONS_SAMPLE_INTERVAL_MS) {
        return;
    }
    s_last_poll_ms = now;

    /* Vzorkovanie a vyhodnotenie vsetkych 4 tlacitiek */
    for (uint8_t i = 0U; i < (uint8_t)BTN_COUNT; i++) {
        /* Tlacitka na kite maju pull-up; stlacene tlacitko privadza pin k zemi (LOW) */
        bool raw_pressed = (pinRead(s_btn_pins[i]) == LOW);

        if (raw_pressed != s_btn_state[i]) {
            /* Surovy stav sa lisi od stabilneho stavu -> navys pocitadlo zhodnych vzoriek */
            s_debounce_count[i]++;

            if (s_debounce_count[i] >= BUTTONS_DEBOUNCE_SAMPLES) {
                /* Prechod je stabilny po stanoveny cas -> zmen stabilny stav */
                s_btn_state[i] = raw_pressed;
                s_debounce_count[i] = 0U;

                /* Detekcia stlacenia: zmena z false (uvolnene) na true (stlacene) */
                if (s_btn_state[i]) {
                    s_btn_clicked[i] = true;

                    /* Pri stlaceni SW4 sa okamzite nastavi priznak STORNO */
                    if (i == (uint8_t)BTN_SW4) {
                        s_storno_flag = true;
                    }
                }
            }
        } else {
            /* Stav zodpoveda stabilnemu, ziadny zakmit ani zmena nenastala */
            s_debounce_count[i] = 0U;
        }
    }
}

/* Zisti jednorazove stlacenie tlacitka a vynuluje priznak (latch/consume) */
bool Task_Buttons_WasPressed(uint8_t btn)
{
    int idx = f_Button_ToIndex(btn);
    if (idx < 0 || idx >= (int)BTN_COUNT) {
        return false;
    }

    if (s_btn_clicked[idx]) {
        s_btn_clicked[idx] = false; /* Skonzumuj priznak jednorazoveho stlacenia */
        return true;
    }

    return false;
}

/* Vracia aktualny stabilny (odfiltrovany) fyzicky stav tlacitka (true = drzane) */
bool Task_Buttons_IsDown(uint8_t btn)
{
    int idx = f_Button_ToIndex(btn);
    if (idx < 0 || idx >= (int)BTN_COUNT) {
        return false;
    }

    return s_btn_state[idx];
}

/* Overi, ci je aktivny priznak STORNO */
bool Task_Buttons_IsStorno(void)
{
    return s_storno_flag;
}

/* Overi priznak STORNO a zaroven ho vynuluje */
bool Task_Buttons_GetStorno(void)
{
    if (s_storno_flag) {
        s_storno_flag = false;
        return true;
    }
    return false;
}

/* Rucne vynuluje priznak STORNO */
void Task_Buttons_ClearStorno(void)
{
    s_storno_flag = false;
}

/* Umoznuje softverovo aktivovat STORNO */
void Task_Buttons_TriggerStorno(void)
{
    s_storno_flag = true;
}

/* Vynuluje vsetky stlacenia a STORNO */
void Task_Buttons_ClearAll(void)
{
    for (uint8_t i = 0U; i < (uint8_t)BTN_COUNT; i++) {
        s_btn_clicked[i] = false;
        s_debounce_count[i] = 0U;
    }
    s_storno_flag = false;
}

/* Vracia index prveho stlaceneho tlacitka (0 az 3) a skonzumuje ho, alebo -1 */
int Task_Buttons_GetPressedButton(void)
{
    for (uint8_t i = 0U; i < (uint8_t)BTN_COUNT; i++) {
        if (s_btn_clicked[i]) {
            s_btn_clicked[i] = false;
            return (int)i;
        }
    }
    return -1;
}
