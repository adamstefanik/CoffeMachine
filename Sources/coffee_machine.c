/*
 * @file     coffee_machine.c
 * @brief    Implementation of finite state machine for Coffee Machine
 */

#include "coffee_machine.h"
#include "task_buttons.h"
#include "drv_adc.h"
#include "drv_kava.h"
#include "drv_systick.h"

#define DRAIN_GRACE_PERIOD_MS       (3000U) /* 3 s extra drain time after sensors hit 0 */
#define STORNO_DRAIN_GRACE_MS       (3000U) /* 3 s extra drain time during emergency storno */
#define STORNO_DISPENSE_TIME_MS     (7000U) /* Drain duration for mixing chamber in storno */
#define DONE_SCREEN_TIMEOUT_MS      (3500U) /* Time to show Done screen before IDLE */

/* Program 1: Espresso parameters */
#define STIR_TIME_ESPRESSO_MS       (3000U) /* 3 s stirring in mixer */
#define DISPENSE_TIME_ESPRESSO_MS   (5000U) /* 5 s dispensing into cup */

/* Program 2: Lungo parameters */
#define STIR_TIME_LUNGO_MS          (5000U) /* 5 s stirring for larger volume */
#define DISPENSE_TIME_LUNGO_MS      (7000U) /* 7 s dispensing full cup volume */

/* Program 3: Doppio parameters */
#define STIR_TIME_DOPPIO_MS         (6000U) /* 6 s stirring double batch in mixer */
#define DISPENSE_TIME_DOPPIO_MS     (8000U) /* 8 s dispensing double volume */

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

    /* Deterministic actuator state upon entering new state */
    switch (new_state) {
    case STATE_IDLE:
        s_ctx.doppio_cycle = 1U;
        Kava_VypniVse();
        break;

    case STATE_WIZARD_COFFEE:
    case STATE_WIZARD_SUGAR:
    case STATE_DONE:
        Kava_VypniVse();
        break;

    case STATE_ESPRESSO_FILL_COFFEE:
    case STATE_LUNGO_FILL_COFFEE:
    case STATE_DOPPIO_C1_FILL_COFFEE:
    case STATE_DOPPIO_C2_FILL_COFFEE:
        Kava_NastavVentil(VENTIL_KAVA, true);
        break;

    case STATE_LUNGO_FILL_WATER:
        Kava_NastavVentil(VENTIL_VODA, true);
        break;

    case STATE_ESPRESSO_FILL_SUGAR:
    case STATE_LUNGO_FILL_SUGAR:
    case STATE_DOPPIO_C2_FILL_SUGAR:
        Kava_NastavVentil(VENTIL_CUKR, true);
        break;

    case STATE_ESPRESSO_DRAIN:
    case STATE_LUNGO_DRAIN:
    case STATE_DOPPIO_C1_DRAIN:
    case STATE_DOPPIO_C2_DRAIN:
    case STATE_STORNO_DRAIN_TANKS:
        Kava_NastavVentil(VENTIL_MIX, true);
        break;

    case STATE_ESPRESSO_STIR:
    case STATE_LUNGO_STIR:
    case STATE_DOPPIO_STIR:
        Kava_NastavMichadlo(true);
        break;

    case STATE_ESPRESSO_DISPENSE:
    case STATE_LUNGO_DISPENSE:
    case STATE_DOPPIO_DISPENSE:
    case STATE_STORNO_DRAIN_MIX:
        Kava_NastavVentil(VENTIL_VYPUST, true);
        break;

    default:
        break;
    }
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

    /* Emergency STORNO handling: SW4 press during any active preparation phase */
    if ((s_state >= STATE_ESPRESSO_FILL_COFFEE) && (s_state <= STATE_DOPPIO_DISPENSE)) {
        if (Task_Buttons_IsStorno() || Task_Buttons_WasPressed(BTN_SW4)) {
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

        if (Task_Buttons_WasPressed(BTN_SW1)) {
            s_ctx.drink = DRINK_ESPRESSO;
            s_ctx.coffee_level = ADC_GetCoffeeLevel();
            CoffeeMachine_SetState(STATE_WIZARD_COFFEE);
        } else if (Task_Buttons_WasPressed(BTN_SW2)) {
            s_ctx.drink = DRINK_LUNGO;
            s_ctx.coffee_level = ADC_GetCoffeeLevel();
            CoffeeMachine_SetState(STATE_WIZARD_COFFEE);
        } else if (Task_Buttons_WasPressed(BTN_SW3)) {
            s_ctx.drink = DRINK_DOPPIO;
            s_ctx.coffee_level = ADC_GetCoffeeLevel();
            CoffeeMachine_SetState(STATE_WIZARD_COFFEE);
        }
        break;

    case STATE_WIZARD_COFFEE:
        /* Continuously update coffee level from potentiometer position */
        s_ctx.coffee_level = ADC_GetCoffeeLevel();

        if (Task_Buttons_WasPressed(BTN_SW4)) {
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

        if (Task_Buttons_WasPressed(BTN_SW4)) {
            /* Cancel / Back -> return to drink selection */
            CoffeeMachine_SetState(STATE_IDLE);
        } else if (Buttons_WasAnyConfirmPressed()) {
            /* Confirm sugar level -> dispatch selected beverage program! */
            s_ctx.doppio_cycle = 1U;

            if (s_ctx.drink == DRINK_ESPRESSO) {
                CoffeeMachine_SetState(STATE_ESPRESSO_FILL_COFFEE);
            } else if (s_ctx.drink == DRINK_LUNGO) {
                CoffeeMachine_SetState(STATE_LUNGO_FILL_COFFEE);
            } else {
                CoffeeMachine_SetState(STATE_DOPPIO_C1_FILL_COFFEE);
            }
        }
        break;

    /* -------------------------------------------------------------
     * PROGRAM 1: ESPRESSO CONTROL SEQUENCE (without TANK2)
     * ------------------------------------------------------------- */
    case STATE_ESPRESSO_FILL_COFFEE: {
        /* Target sensor based on selected coffee strength: H3=Mild, H2=Medium, H1=Strong */
        uint8_t target_sensor = HLADINA_H2;
        if (s_ctx.coffee_level == COFFEE_LVL_MILD) {
            target_sensor = HLADINA_H3;
        } else if (s_ctx.coffee_level == COFFEE_LVL_STRONG) {
            target_sensor = HLADINA_H1;
        }

        /* Check if level reached target sensor or upper overflow guard H1 */
        if (Kava_CtiHladinu(target_sensor) || Kava_CtiHladinu(HLADINA_H1)) {
            Kava_NastavVentil(VENTIL_KAVA, false);

            if (s_ctx.sugar_level > SUGAR_LVL_NONE) {
                CoffeeMachine_SetState(STATE_ESPRESSO_FILL_SUGAR);
            } else {
                CoffeeMachine_SetState(STATE_ESPRESSO_DRAIN);
            }
        }
        break;
    }

    case STATE_ESPRESSO_FILL_SUGAR: {
        /* Target sensor based on selected sugar level: H8=Low, H7=Medium, H6=High */
        uint8_t target_sugar_sensor = HLADINA_H8;
        if (s_ctx.sugar_level == SUGAR_LVL_MEDIUM) {
            target_sugar_sensor = HLADINA_H7;
        } else if (s_ctx.sugar_level == SUGAR_LVL_HIGH) {
            target_sugar_sensor = HLADINA_H6;
        }

        if (Kava_CtiHladinu(target_sugar_sensor) || Kava_CtiHladinu(HLADINA_H6)) {
            Kava_NastavVentil(VENTIL_CUKR, false);
            CoffeeMachine_SetState(STATE_ESPRESSO_DRAIN);
        }
        break;
    }

    case STATE_ESPRESSO_DRAIN: {
        /* Wait until TANK1 drops below H3 and TANK3 drops below H8 */
        bool coffee_empty = !Kava_CtiHladinu(HLADINA_H3);
        bool sugar_empty = (s_ctx.sugar_level == SUGAR_LVL_NONE) || !Kava_CtiHladinu(HLADINA_H8);

        if (coffee_empty && sugar_empty) {
            if (!s_ctx.drain_sensors_cleared) {
                s_ctx.drain_sensors_cleared = true;
                s_ctx.drain_cleared_ms = now;
            } else if ((now - s_ctx.drain_cleared_ms) >= DRAIN_GRACE_PERIOD_MS) {
                /* 3 s grace period elapsed: reservoirs and pipes fully empty */
                Kava_NastavVentil(VENTIL_MIX, false);
                s_ctx.timer_duration_ms = STIR_TIME_ESPRESSO_MS;
                CoffeeMachine_SetState(STATE_ESPRESSO_STIR);
            }
        }
        break;
    }

    case STATE_ESPRESSO_STIR:
        /* Stir for 3 s in mixing chamber MIX */
        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavMichadlo(false);
            s_ctx.timer_duration_ms = DISPENSE_TIME_ESPRESSO_MS;
            CoffeeMachine_SetState(STATE_ESPRESSO_DISPENSE);
        }
        break;

    case STATE_ESPRESSO_DISPENSE:
        /* Dispense drink into cup via SV5 for 5 s */
        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavVentil(VENTIL_VYPUST, false);
            s_ctx.timer_duration_ms = DONE_SCREEN_TIMEOUT_MS;
            CoffeeMachine_SetState(STATE_DONE);
        }
        break;

    /* -------------------------------------------------------------
     * PROGRAM 2: LUNGO CONTROL SEQUENCE (with hot water TANK2 up to H4)
     * ------------------------------------------------------------- */
    case STATE_LUNGO_FILL_COFFEE: {
        /* Target sensor based on coffee strength: H3=Mild, H2=Medium, H1=Strong */
        uint8_t target_sensor = HLADINA_H2;
        if (s_ctx.coffee_level == COFFEE_LVL_MILD) {
            target_sensor = HLADINA_H3;
        } else if (s_ctx.coffee_level == COFFEE_LVL_STRONG) {
            target_sensor = HLADINA_H1;
        }

        /* Wait for target sensor or H1 overflow safety */
        if (Kava_CtiHladinu(target_sensor) || Kava_CtiHladinu(HLADINA_H1)) {
            Kava_NastavVentil(VENTIL_KAVA, false);
            CoffeeMachine_SetState(STATE_LUNGO_FILL_WATER);
        }
        break;
    }

    case STATE_LUNGO_FILL_WATER:
        /* Hot water fills TANK2 via SV2 up to high sensor H4 */
        if (Kava_CtiHladinu(HLADINA_H4)) {
            Kava_NastavVentil(VENTIL_VODA, false);

            if (s_ctx.sugar_level > SUGAR_LVL_NONE) {
                CoffeeMachine_SetState(STATE_LUNGO_FILL_SUGAR);
            } else {
                CoffeeMachine_SetState(STATE_LUNGO_DRAIN);
            }
        }
        break;

    case STATE_LUNGO_FILL_SUGAR: {
        uint8_t target_sugar_sensor = HLADINA_H8;
        if (s_ctx.sugar_level == SUGAR_LVL_MEDIUM) {
            target_sugar_sensor = HLADINA_H7;
        } else if (s_ctx.sugar_level == SUGAR_LVL_HIGH) {
            target_sugar_sensor = HLADINA_H6;
        }

        if (Kava_CtiHladinu(target_sugar_sensor) || Kava_CtiHladinu(HLADINA_H6)) {
            Kava_NastavVentil(VENTIL_CUKR, false);
            CoffeeMachine_SetState(STATE_LUNGO_DRAIN);
        }
        break;
    }

    case STATE_LUNGO_DRAIN: {
        /* Drain all 3 reservoirs into mixing chamber via SV4 */
        bool coffee_empty = !Kava_CtiHladinu(HLADINA_H3);
        bool water_empty = !Kava_CtiHladinu(HLADINA_H5);
        bool sugar_empty = (s_ctx.sugar_level == SUGAR_LVL_NONE) || !Kava_CtiHladinu(HLADINA_H8);

        if (coffee_empty && water_empty && sugar_empty) {
            if (!s_ctx.drain_sensors_cleared) {
                s_ctx.drain_sensors_cleared = true;
                s_ctx.drain_cleared_ms = now;
            } else if ((now - s_ctx.drain_cleared_ms) >= DRAIN_GRACE_PERIOD_MS) {
                /* 3 s grace period elapsed: all 3 reservoirs fully emptied */
                Kava_NastavVentil(VENTIL_MIX, false);
                s_ctx.timer_duration_ms = STIR_TIME_LUNGO_MS;
                CoffeeMachine_SetState(STATE_LUNGO_STIR);
            }
        }
        break;
    }

    case STATE_LUNGO_STIR:
        /* Stir for 5 s in mixing chamber MIX */
        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavMichadlo(false);
            s_ctx.timer_duration_ms = DISPENSE_TIME_LUNGO_MS;
            CoffeeMachine_SetState(STATE_LUNGO_DISPENSE);
        }
        break;

    case STATE_LUNGO_DISPENSE:
        /* Dispense extended Lungo volume via SV5 for 7 s */
        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavVentil(VENTIL_VYPUST, false);
            s_ctx.timer_duration_ms = DONE_SCREEN_TIMEOUT_MS;
            CoffeeMachine_SetState(STATE_DONE);
        }
        break;

    /* -------------------------------------------------------------
     * PROGRAM 3: DOPPIO CONTROL SEQUENCE (2x cycle from TANK1)
     * ------------------------------------------------------------- */
    case STATE_DOPPIO_C1_FILL_COFFEE: {
        /* Cycle 1: Fill coffee TANK1 via SV1 */
        uint8_t target_sensor = HLADINA_H2;
        if (s_ctx.coffee_level == COFFEE_LVL_MILD) {
            target_sensor = HLADINA_H3;
        } else if (s_ctx.coffee_level == COFFEE_LVL_STRONG) {
            target_sensor = HLADINA_H1;
        }

        if (Kava_CtiHladinu(target_sensor) || Kava_CtiHladinu(HLADINA_H1)) {
            Kava_NastavVentil(VENTIL_KAVA, false);
            CoffeeMachine_SetState(STATE_DOPPIO_C1_DRAIN);
        }
        break;
    }

    case STATE_DOPPIO_C1_DRAIN:
        /* Cycle 1: Drain TANK1 into mixer via SV4 */
        if (!Kava_CtiHladinu(HLADINA_H3)) {
            if (!s_ctx.drain_sensors_cleared) {
                s_ctx.drain_sensors_cleared = true;
                s_ctx.drain_cleared_ms = now;
            } else if ((now - s_ctx.drain_cleared_ms) >= DRAIN_GRACE_PERIOD_MS) {
                /* TANK1 completely emptied into mixer */
                Kava_NastavVentil(VENTIL_MIX, false);
                s_ctx.doppio_cycle = 2U;
                CoffeeMachine_SetState(STATE_DOPPIO_C2_FILL_COFFEE);
            }
        }
        break;

    case STATE_DOPPIO_C2_FILL_COFFEE: {
        /* Cycle 2: Refill coffee TANK1 via SV1 */
        uint8_t target_sensor = HLADINA_H2;
        if (s_ctx.coffee_level == COFFEE_LVL_MILD) {
            target_sensor = HLADINA_H3;
        } else if (s_ctx.coffee_level == COFFEE_LVL_STRONG) {
            target_sensor = HLADINA_H1;
        }

        if (Kava_CtiHladinu(target_sensor) || Kava_CtiHladinu(HLADINA_H1)) {
            Kava_NastavVentil(VENTIL_KAVA, false);

            if (s_ctx.sugar_level > SUGAR_LVL_NONE) {
                CoffeeMachine_SetState(STATE_DOPPIO_C2_FILL_SUGAR);
            } else {
                CoffeeMachine_SetState(STATE_DOPPIO_C2_DRAIN);
            }
        }
        break;
    }

    case STATE_DOPPIO_C2_FILL_SUGAR: {
        /* Cycle 2: Add optional sugar into TANK3 via SV3 */
        uint8_t target_sugar_sensor = HLADINA_H8;
        if (s_ctx.sugar_level == SUGAR_LVL_MEDIUM) {
            target_sugar_sensor = HLADINA_H7;
        } else if (s_ctx.sugar_level == SUGAR_LVL_HIGH) {
            target_sugar_sensor = HLADINA_H6;
        }

        if (Kava_CtiHladinu(target_sugar_sensor) || Kava_CtiHladinu(HLADINA_H6)) {
            Kava_NastavVentil(VENTIL_CUKR, false);
            CoffeeMachine_SetState(STATE_DOPPIO_C2_DRAIN);
        }
        break;
    }

    case STATE_DOPPIO_C2_DRAIN: {
        /* Cycle 2: Drain coffee and optional sugar into mixer via SV4 */
        bool coffee_empty = !Kava_CtiHladinu(HLADINA_H3);
        bool sugar_empty = (s_ctx.sugar_level == SUGAR_LVL_NONE) || !Kava_CtiHladinu(HLADINA_H8);

        if (coffee_empty && sugar_empty) {
            if (!s_ctx.drain_sensors_cleared) {
                s_ctx.drain_sensors_cleared = true;
                s_ctx.drain_cleared_ms = now;
            } else if ((now - s_ctx.drain_cleared_ms) >= DRAIN_GRACE_PERIOD_MS) {
                /* Both cycles fully drained into mixer */
                Kava_NastavVentil(VENTIL_MIX, false);
                s_ctx.timer_duration_ms = STIR_TIME_DOPPIO_MS;
                CoffeeMachine_SetState(STATE_DOPPIO_STIR);
            }
        }
        break;
    }

    case STATE_DOPPIO_STIR:
        /* Stir double batch in mixing chamber for 6 s */
        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavMichadlo(false);
            s_ctx.timer_duration_ms = DISPENSE_TIME_DOPPIO_MS;
            CoffeeMachine_SetState(STATE_DOPPIO_DISPENSE);
        }
        break;

    case STATE_DOPPIO_DISPENSE:
        /* Dispense double batch via SV5 for 8 s */
        if ((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) {
            Kava_NastavVentil(VENTIL_VYPUST, false);
            s_ctx.timer_duration_ms = DONE_SCREEN_TIMEOUT_MS;
            CoffeeMachine_SetState(STATE_DONE);
        }
        break;

    /* -------------------------------------------------------------
     * ORDER COMPLETION
     * ------------------------------------------------------------- */
    case STATE_DONE:
        Kava_VypniVse();

        if (((now - s_ctx.state_start_ms) >= s_ctx.timer_duration_ms) ||
            Buttons_WasAnyConfirmPressed() || Task_Buttons_WasPressed(BTN_SW4)) {
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
