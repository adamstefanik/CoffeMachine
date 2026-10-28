/*
 * @file     task_buttons.h
 * @brief    Neblokujuca uloha (task) pre osetrenie tlacitiek SW1-SW4 s debouncingom,
 *           detekciou hran (jednoznacne stlacenie) a priznakom STORNO.
 * @note     Projekt: Automat na kavu (EDU-mod na FRDM-KL25Z)
 */

#ifndef TASK_BUTTONS_H_
#define TASK_BUTTONS_H_

#include <stdbool.h>
#include <stdint.h>
#include "drv_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Casove konstanty pre debouncing
 * ============================================================================ */
#define BUTTONS_SAMPLE_INTERVAL_MS   (10U)  /* Perioda vzorkovania tlacitiek v ms */
#define BUTTONS_DEBOUNCE_SAMPLES     (2U)   /* Pocet zhodnych vzoriek pre potvrdenie zmeny (2 * 10 ms = 20 ms) */

/* ============================================================================
 * Logicke identifikatory tlacitiek
 * ============================================================================ */
typedef enum {
    BTN_SW1 = 0,    /* Tlacitko SW1 (PTA4) - Vyber kavy / Potvrdenie (OK) */
    BTN_SW2,        /* Tlacitko SW2 (PTA5) - Horuca voda / Navigacia */
    BTN_SW3,        /* Tlacitko SW3 (PTA16) - Cukor / Start */
    BTN_SW4,        /* Tlacitko SW4 (PTA17) - STORNO / Spat */
    BTN_COUNT
} Button_t;

/* Aliasy pre kompatibilitu s roznymi konvenciami */
#define BUTTON_SW1   BTN_SW1
#define BUTTON_SW2   BTN_SW2
#define BUTTON_SW3   BTN_SW3
#define BUTTON_SW4   BTN_SW4

/* ============================================================================
 * Verejne funkcie ulohy Task_Buttons
 * ============================================================================ */

/**
 * @brief Inicializuje piny tlacitiek SW1 az SW4 ako vstupy s pull-up rezistormi
 *        (INPUT_PULLUP) a nastavi pociatocny stav debouncingu.
 */
void Task_Buttons_Init(void);

/**
 * @brief Periodicka neblokujuca uloha pre citanie a filtrovanie zakmitov (debouncing)
 *        tlacitiek SW1 az SW4. Volana v hlavnej slucke for (;;) kazdych cca 10 ms.
 *        Pouziva SYSTICK_millis() pre presne casovanie bez blokovania.
 */
void Task_Buttons(void);

/**
 * @brief Zisti, ci doslo k jednorazovemu stlaceniu tlacitka (detekcia nabeznej hrany stlacenia).
 *        Pri precitani sa priznak automaticky vynuluje (latch/consume), takze drzanie
 *        tlacitka nesposobi opakovane preskocenie krokov vo Wizarde.
 * @param btn Identifikator tlacitka (BTN_SW1..BTN_SW4 alebo priamo SW1..SW4).
 * @return true ak bolo tlacitko jednorazovo stlacene od posledneho volania; inak false.
 */
bool Task_Buttons_WasPressed(uint8_t btn);

/**
 * @brief Vracia aktualny stabilny (odfiltrovany) fyzicky stav tlacitka.
 * @param btn Identifikator tlacitka (BTN_SW1..BTN_SW4 alebo SW1..SW4).
 * @return true ak je tlacitko aktualne fyzicky stlacene (drzane); inak false.
 */
bool Task_Buttons_IsDown(uint8_t btn);

/**
 * @brief Zisti, ci bolo stlacene tlacidlo SW4 a je aktivny priznak STORNO.
 *        Tento priznak sa NIE JE automaticky mazany tymto volanim.
 * @return true ak bol zaznamenany poziadavok na STORNO; inak false.
 */
bool Task_Buttons_IsStorno(void);

/**
 * @brief Zisti, ci bol aktivovany priznak STORNO, a zaroven ho vynuluje (consume).
 * @return true ak bol poziadavok na STORNO aktivny; inak false.
 */
bool Task_Buttons_GetStorno(void);

/**
 * @brief Rucne vynuluje priznak STORNO (napr. po bezpecnom dokonceni vypustenia).
 */
void Task_Buttons_ClearStorno(void);

/**
 * @brief Umoznuje softverovo vyvolat priznak STORNO (napr. pri bezpecnostnej chybe).
 */
void Task_Buttons_TriggerStorno(void);

/**
 * @brief Vynuluje vsetky nevybavene priznaky stlacenia tlacitiek aj priznak STORNO.
 *        Vhodne volat pri prechode medzi stavmi konecneho automatu (FSM).
 */
void Task_Buttons_ClearAll(void);

/**
 * @brief Zisti, ktore tlacitko bolo stlacene ako prve v poradi (ak vobec nejake).
 *        Priznak stlacenia pre dane tlacitko sa automaticky skonzumuje.
 * @return Index stlaceneho tlacitka (0 az 3) alebo -1 ak ziadne tlacitko nebolo stlacene.
 */
int Task_Buttons_GetPressedButton(void);

/* ============================================================================
 * Pohodlne skratene aliasy
 * ============================================================================ */
#define Buttons_Init             Task_Buttons_Init
#define Buttons_Task             Task_Buttons
#define Buttons_WasPressed       Task_Buttons_WasPressed
#define Buttons_IsDown           Task_Buttons_IsDown
#define Buttons_IsStorno         Task_Buttons_IsStorno
#define Buttons_GetStorno        Task_Buttons_GetStorno
#define Buttons_ClearStorno      Task_Buttons_ClearStorno
#define Buttons_TriggerStorno    Task_Buttons_TriggerStorno
#define Buttons_ClearAll         Task_Buttons_ClearAll
#define Buttons_GetPressedButton Task_Buttons_GetPressedButton

#ifdef __cplusplus
}
#endif

#endif /* TASK_BUTTONS_H_ */
