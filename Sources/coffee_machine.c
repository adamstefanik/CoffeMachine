/*
 * @file     coffee_machine.c
 * @brief    Implementation of finite state machine for Coffee Machine
 */

#include "coffee_machine.h"
#include "task_buttons.h"
#include "drv_adc.h"
#include "drv_kava.h"
#include "drv_systick.h"

#define DRAIN_GRACE_PERIOD_MS       (2500U) /* Extra drain time after lower sensors hit 0 */
#define STORNO_DRAIN_GRACE_MS       (3000U) /* Extra drain time during emergency storno */
#define STORNO_DISPENSE_TIME_MS     (7000U) /* Drain duration for mixing chamber in storno */
#define DONE_SCREEN_TIMEOUT_MS      (3500U) /* Time to show Done screen before IDLE */

/* Drink-specific parameters */
#define STIR_TIME_ESPRESSO_MS       (3000U)
#define STIR_TIME_LUNGO_MS          (5000U)
#define STIR_TIME_DOPPIO_MS         (6000U)

#define DISPENSE_TIME_ESPRESSO_MS   (5000U)
#define DISPENSE_TIME_LUNGO_MS      (7000U)
#define DISPENSE_TIME_DOPPIO_MS     (8000U)

static CoffeeState_t s_state = STATE_IDLE;
static CoffeeMachineContext_t s_ctx;

static bool Buttons_WasAnyConfirmPressed(void)
{
    return Task_Buttons_WasPressed(BTN_SW1) ||
           Task_Buttons_WasPressed(BTN_SW2) ||
           Task_Buttons_WasPressed(BTN_SW3);
}

static void CoffeeMachine_SetState(CoffeeState_t new_state)
{
    s_state = new_state;
    s_ctx.state_start_ms = SYSTICK_millis();
    s_ctx.drain_sensors_cleared = false;
    s_ctx.drain_cleared_ms = 0U;
    Task_Buttons_ClearAll();
}

void CoffeeMachine_Init(void)
{
    s_ctx.drink = DRINK_NONE;
    s_ctx.coffee_level = 2U; /* Default Medium */
    s_ctx.sugar_level = 0U;  /* Default None */
    s_ctx.doppio_cycle = 1U;
    s_ctx.state_start_ms = 0U;
    s_ctx.timer_duration_ms = 0U;
    s_ctx.drain_sensors_cleared = false;
    s_ctx.drain_cleared_ms = 0U;

    Kava_VypniVse();
    CoffeeMachine_SetState(STATE_IDLE);
}

CoffeeState_t CoffeeMachine_GetState(void)
{
    return s_state;
}

const CoffeeMachineContext_t* CoffeeMachine_GetContext(void)
{
    return &s_ctx;
}

const char* CoffeeMachine_GetDrinkName(DrinkType_t drink)
{
    switch (drink) {
    case DRINK_ESPRESSO:
        return "Espresso";
    case DRINK_LUNGO:
        return "Lungo";
    case DRINK_DOPPIO:
        return "Doppio";
    default:
        return "Coffee";
    }
}

void CoffeeMachine_RequestStorno(void)
{
    /* Immediately close all supply valves and stop stirrer */
    Kava_NastavVentil(VENTIL_KAVA, false);
    Kava_NastavVentil(VENTIL_VODA, false);
    Kava_NastavVentil(VENTIL_CUKR, false);
    Kava_NastavMichadlo(false);

    /* Open valve SV4 to empty upper tanks into mixer */
    Kava_NastavVentil(VENTIL_MIX, true);
    CoffeeMachine_SetState(STATE_STORNO_DRAIN_TANKS);
}

void Task_CoffeeMachine(void)
{
    uint32_t now = SYSTICK_millis();

    /* Emergency STORNO handling: SW4 press during active preparation */
    if ((s_state >= STATE_FILL_COFFEE) && (s_state <= STATE_DISPENSING)) {
        if (Buttons_WasPressed(SW4)) {
            CoffeeMachine_RequestStorno();
            return;
        }
    }

    switch (s_state) {
    /* -------------------------------------------------------------
     * WIZARD SCREENS
     * ------------------------------------------------------------- */
    case STATE_IDLE:
        Kava_VypniVse();

        if (Buttons_WasPressed(SW1)) {
            s_ctx.drink = DRINK_ESPRESSO;
            s_ctx.coffee_level = ADC_GetCoffeeLevel();
            CoffeeMachine_SetState(STATE_WIZARD_COFFEE);
        } else if (Buttons_WasPressed(SW2)) {
            s_ctx.drink = DRINK_LUNGO;
            s_ctx.coffee_level = ADC_GetCoffeeLevel();
            CoffeeMachine_SetState(STATE_WIZARD_COFFEE);
        } else if (Buttons_WasPressed(SW3)) {
            s_ctx.drink = DRINK_DOPPIO;
            s_ctx.coffee_level = ADC_GetCoffeeLevel();
            CoffeeMachine_SetState(STATE_WIZARD_COFFEE);
        }
        break;

    case STATE_WIZARD_COFFEE:
        /* Continuously update coffee level from potentiometer position */
        s_ctx.coffee_level = ADC_GetCoffeeLevel();

        if (Buttons_WasPressed(SW4)) {
            /* Cancel / Back -> return to drink selection */
            CoffeeMachine_SetState(STATE_IDLE);
        } else if (Buttons_WasAnyConfirmPressed()) {
            /* Confirm coffee level -> move to sugar wizard */
            s_ctx.sugar_level = ADC_GetSugarLevel();
            CoffeeMachine_SetState(STATE_WIZARD_SUGAR);
        }
        break;

    case STATE_WIZARD_SUGAR:
        /* Continuously update sugar level from potentiometer position */
        s_ctx.sugar_level = ADC_GetSugarLevel();

        if (Buttons_WasPressed(SW4)) {
            /* Cancel / Back -> return to drink selection */
            CoffeeMachine_SetState(STATE_IDLE);
        } else if (Buttons_WasAnyConfirmPressed()) {
            /* Confirm sugar level -> start beverage preparation! */
            s_ctx.doppio_cycle = 1U;
            Kava_NastavVentil(VENTIL_KAVA, true);
            CoffeeMachine_SetState(STATE_FILL_COFFEE);
        }
        break;

    /* -------------------------------------------------------------
     * ACTIVE PREPARATION PHASES
     * ------------------------------------------------------------- */
    case STATE_FILL_COFFEE: {
        Kava_NastavVentil(VENTIL_KAVA, true);

        /* Determine target sensor */
        uint8_t target_sensor = HLADINA_H2;
        if (s_ctx.coffee_level == 1U) {
            target_sensor = HLADINA_H3;
        } else if (s_ctx.coffee_level == 3U) {
            target_sensor = HLADINA_H1;
        }

        /* Check target sensor or overflow safety */
        if (Kava_CtiHladinu(target_sensor) || Kava_CtiHladinu(HLADINA_H1)) {
            Kava_NastavVentil(VENTIL_KAVA, false);

            if ((s_ctx.drink == DRINK_DOPPIO) && (s_ctx.doppio_cycle == 1U)) {
                /* Doppio cycle 1: transfer coffee directly to mixer */
                Kava_NastavVentil(VENTIL_MIX, true);
                CoffeeMachine_SetState(STATE_DRAIN_TO_MIX);
            } else if (s_ctx.drink == DRINK_LUNGO) {
                /* Lungo: next fill hot water tank (SV2) */
                Kava_NastavVentil(VENTIL_VODA, true);
                CoffeeMachine_SetState(STATE_FILL_WATER);
            } else {
                /* Espresso or Doppio cycle 2: fill sugar if selected, else drain */
                if (s_ctx.sugar_level > 0U) {
                    Kava_NastavVentil(VENTIL_CUKR, true);
                    CoffeeMachine_SetState(STATE_FILL_SUGAR);
                } else {
                    Kava_NastavVentil(VENTIL_MIX, true);
                    CoffeeMachine_SetState(STATE_DRAIN_TO_MIX);
                }
            }
        }
        break;
    }

    case STATE_FILL_WATER:
        Kava_NastavVentil(VENTIL_VODA, true);

        /* Lungo hot water fills up to sensor H4 */
        if (Kava_CtiHladinu(HLADINA_H4)) {
            Kava_NastavVentil(VENTIL_VODA, false);

            if (s_ctx.sugar_level > 0U) {
                Kava_NastavVentil(VENTIL_CUKR, true);
                CoffeeMachine_SetState(STATE_FILL_SUGAR);
            } else {
                Kava_NastavVentil(VENTIL_MIX, true);
                CoffeeMachine_SetState(STATE_DRAIN_TO_MIX);
            }
        }
        break;

    case STATE_FILL_SUGAR: {
        Kava_NastavVentil(VENTIL_CUKR, true);

        uint8_t target_sugar_sensor = HLADINA_H8;
        if (s_ctx.sugar_level == 2U) {
            target_sugar_sensor = HLADINA_H7;
        } else if (s_ctx.sugar_level == 3U) {
            target_sugar_sensor = HLADINA_H6;
        }

        if (Kava_CtiHladinu(target_sugar_sensor) || Kava_CtiHladinu(HLADINA_H6)) {
            Kava_NastavVentil(VENTIL_CUKR, false);
            Kava_NastavVentil(VENTIL_MIX, true);
            CoffeeMachine_SetState(STATE_DRAIN_TO_MIX);
        }
        break;
    }

    case STATE_DRAIN_TO_MIX: {
        Kava_NastavVentil(VENTIL_MIX, true);

        /* Check whether active reservoirs have fallen below lower sensors */
        bool coffee_empty = !Kava_CtiHladinu(HLADINA_H3);
        bool water_empty = (s_ctx.drink != DRINK_LUNGO) || !Kava_CtiHladinu(HLADINA_H5);
        bool sugar_empty = (s_ctx.sugar_level == 0U) ||
                           ((s_ctx.drink == DRINK_DOPPIO) && (s_ctx.doppio_cycle == 1U)) ||
                           !Kava_CtiHladinu(HLADINA_H8);

        if (coffee_empty && water_empty && sugar_empty) {
            if (!s_ctx.drain_sensors_cleared) {
                s_ctx.drain_sensors_cleared = true;
                s_ctx.drain_cleared_ms = now;
            } else if ((now - s_ctx.drain_cleared_ms) >= DRAIN_GRACE_PERIOD_MS) {
                /* Grace period elapsed: all upper tanks are fully empty */
                Kava_NastavVentil(VENTIL_MIX, false);

                if ((s_ctx.drink == DRINK_DOPPIO) && (s_ctx.doppio_cycle == 1U)) {
                    /* Start cycle 2 for Doppio */
                    s_ctx.doppio_cycle = 2U;
                    Kava_NastavVentil(VENTIL_KAVA, true);
                    CoffeeMachine_SetState(STATE_FILL_COFFEE);
                } else {
                    /* Proceed to stirring */
                    Kava_NastavMichadlo(true);
                    if (s_ctx.drink == DRINK_ESPRESSO) {
                        s_ctx.timer_duration_ms = STIR_TIME_ESPRESSO_MS;
                    } else if (s_ctx.drink == DRINK_LUNGO) {
                        s_ctx.timer_duration_ms = STIR_TIME_LUNGO_MS;
                    } else {
                        s_ctx.timer_duration_ms = STIR_TIME_DOPPIO_MS;
                    }
                    CoffeeMachine_SetState(STATE_STIRRING);
                }
            }
        }
        break;
    }

    case STATE_STIRRING:
        Kava_NastavMichadlo(true);

        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavMichadlo(false);
            Kava_NastavVentil(VENTIL_VYPUST, true);

            if (s_ctx.drink == DRINK_ESPRESSO) {
                s_ctx.timer_duration_ms = DISPENSE_TIME_ESPRESSO_MS;
            } else if (s_ctx.drink == DRINK_LUNGO) {
                s_ctx.timer_duration_ms = DISPENSE_TIME_LUNGO_MS;
            } else {
                s_ctx.timer_duration_ms = DISPENSE_TIME_DOPPIO_MS;
            }
            CoffeeMachine_SetState(STATE_DISPENSING);
        }
        break;

    case STATE_DISPENSING:
        Kava_NastavVentil(VENTIL_VYPUST, true);

        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavVentil(VENTIL_VYPUST, false);
            s_ctx.timer_duration_ms = DONE_SCREEN_TIMEOUT_MS;
            CoffeeMachine_SetState(STATE_DONE);
        }
        break;

    case STATE_DONE:
        Kava_VypniVse();

        if (((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) ||
            Buttons_WasAnyConfirmPressed() || Buttons_WasPressed(SW4)) {
            CoffeeMachine_SetState(STATE_IDLE);
        }
        break;

    /* -------------------------------------------------------------
     * STORNO / EMERGENCY DRAIN
     * ------------------------------------------------------------- */
    case STATE_STORNO_DRAIN_TANKS:
        Kava_NastavVentil(VENTIL_MIX, true);

        /* Wait until all lower level sensors are 0 */
        if (!Kava_CtiHladinu(HLADINA_H3) && !Kava_CtiHladinu(HLADINA_H5) && !Kava_CtiHladinu(HLADINA_H8)) {
            if (!s_ctx.drain_sensors_cleared) {
                s_ctx.drain_sensors_cleared = true;
                s_ctx.drain_cleared_ms = now;
            } else if ((now - s_ctx.drain_cleared_ms) >= STORNO_DRAIN_GRACE_MS) {
                Kava_NastavVentil(VENTIL_MIX, false);
                Kava_NastavVentil(VENTIL_VYPUST, true);
                s_ctx.timer_duration_ms = STORNO_DISPENSE_TIME_MS;
                CoffeeMachine_SetState(STATE_STORNO_DRAIN_MIX);
            }
        }
        break;

    case STATE_STORNO_DRAIN_MIX:
        Kava_NastavVentil(VENTIL_VYPUST, true);

        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavVentil(VENTIL_VYPUST, false);
            CoffeeMachine_SetState(STATE_IDLE);
        }
        break;

    default:
        CoffeeMachine_SetState(STATE_IDLE);
        break;
    }
}
