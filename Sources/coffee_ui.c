/*
 * @file     coffee_ui.c
 * @brief    Implementation of LCD user interface, Wizard screens, and status screens
 */

#include "coffee_ui.h"
#include "drv_lcd.h"
#include "drv_systick.h"
#include "drv_adc.h"
#include <string.h>

#define UI_REFRESH_INTERVAL_MS   (100U) /* Screen refresh interval in milliseconds */

/* Shadow buffer tracking what is currently visible on the physical LCD */
static char s_shadow[4][21];

/* Framebuffer for the next frame to be rendered */
static char s_frame[4][21];

/* Timing tracking */
static uint32_t s_last_refresh_ms = 0U;
static CoffeeState_t s_last_rendered_state = (CoffeeState_t)0xFF;

/* Helper to pad any null-terminated string to exactly 20 characters */
static void UI_FormatLine20(char *dest, const char *src)
{
    uint8_t i = 0U;
    while ((src[i] != '\0') && (i < 20U)) {
        dest[i] = src[i];
        i++;
    }
    while (i < 20U) {
        dest[i] = ' ';
        i++;
    }
    dest[20] = '\0';
}

/* Helper to format a 1-digit or 2-digit number into buffer */
static void UI_FormatDec2(uint8_t val, char *dest)
{
    if (val >= 10U) {
        dest[0] = (char)('0' + (val / 10U));
        dest[1] = (char)('0' + (val % 10U));
    } else {
        dest[0] = (char)('0' + val);
        dest[1] = ' ';
    }
}

/* Commits the current frame to the physical LCD using differential updates */
static void UI_CommitScreen(void)
{
    for (uint8_t line = 0U; line < 4U; line++) {
        /* Only send data if line content has changed (eliminates LCD flickering) */
        if (memcmp(s_shadow[line], s_frame[line], 20) != 0) {
            LCD_set_cursor(line + 1U, 1U);
            LCD_puts(s_frame[line]);
            memcpy(s_shadow[line], s_frame[line], 21);
        }
    }
}

void UI_Init(void)
{
    LCD_initialize();
    LCD_backlight_on();
    LCD_clear();

    /* Invalidate shadow buffer to ensure initial commit writes all 4 lines */
    for (uint8_t i = 0U; i < 4U; i++) {
        memset(s_shadow[i], 0, 21);
    }
    s_last_refresh_ms = 0U;
    s_last_rendered_state = (CoffeeState_t)0xFF;
}

void UI_ForceRefresh(void)
{
    for (uint8_t i = 0U; i < 4U; i++) {
        memset(s_shadow[i], 0, 21);
    }
}

/* 1. Drink Selection Screen (STATE_IDLE) */
static void UI_RenderIdle(void)
{
    UI_FormatLine20(s_frame[0], "=== SELECT DRINK ===");
    UI_FormatLine20(s_frame[1], "SW1: Espresso");
    UI_FormatLine20(s_frame[2], "SW2: Lungo");
    UI_FormatLine20(s_frame[3], "SW3: Doppio");
}

/* 2. Coffee Strength Adjustment Screen (STATE_WIZARD_COFFEE) */
static void UI_RenderWizardCoffee(DrinkType_t drink, uint8_t coffee_lvl)
{
    char line1[21];
    char line2[21];
    char bar[10];

    /* Line 1: Drink name + parameter */
    const char *name = CoffeeMachine_GetDrinkName(drink);
    uint8_t idx = 0U;
    while ((name[idx] != '\0') && (idx < 10U)) {
        line1[idx] = name[idx];
        idx++;
    }
    const char *suffix = ": Strength";
    uint8_t s = 0U;
    while ((suffix[s] != '\0') && (idx < 20U)) {
        line1[idx++] = suffix[s++];
    }
    line1[idx] = '\0';
    UI_FormatLine20(s_frame[0], line1);

    /* Line 2: Graphical progress bar (width 6) + sensor description (exact 20 chars) */
    ADC_FormatProgressBar(coffee_lvl, 3U, bar, 6U);
    idx = 0U;
    for (uint8_t b = 0U; b < 8U; b++) {
        line2[idx++] = bar[b];
    }

    if (coffee_lvl == 1U) {
        /* "[##    ] Mild   (H3)" */
        const char *desc = " Mild   (H3)";
        for (uint8_t d = 0U; d < 12U; d++) {
            line2[idx++] = desc[d];
        }
    } else if (coffee_lvl == 2U) {
        /* "[####  ] Medium (H2)" */
        const char *desc = " Medium (H2)";
        for (uint8_t d = 0U; d < 12U; d++) {
            line2[idx++] = desc[d];
        }
    } else {
        /* "[######] Strong (H1)" */
        const char *desc = " Strong (H1)";
        for (uint8_t d = 0U; d < 12U; d++) {
            line2[idx++] = desc[d];
        }
    }
    line2[idx] = '\0';
    UI_FormatLine20(s_frame[1], line2);

    /* Line 3: Instruction */
    UI_FormatLine20(s_frame[2], "Turn POT to adjust");

    /* Line 4: Navigation prompts */
    UI_FormatLine20(s_frame[3], "SW1-3: OK   SW4:Back");
}

/* 3. Sugar Level Adjustment Screen (STATE_WIZARD_SUGAR) */
static void UI_RenderWizardSugar(DrinkType_t drink, uint8_t sugar_lvl)
{
    char line1[21];
    char line2[21];
    char bar[10];

    /* Line 1: Drink name + parameter */
    const char *name = CoffeeMachine_GetDrinkName(drink);
    uint8_t idx = 0U;
    while ((name[idx] != '\0') && (idx < 10U)) {
        line1[idx] = name[idx];
        idx++;
    }
    const char *suffix = ": Sugar";
    uint8_t s = 0U;
    while ((suffix[s] != '\0') && (idx < 20U)) {
        line1[idx++] = suffix[s++];
    }
    line1[idx] = '\0';
    UI_FormatLine20(s_frame[0], line1);

    /* Line 2: Graphical progress bar (width 6) + sugar level description */
    ADC_FormatProgressBar(sugar_lvl, 3U, bar, 6U);
    idx = 0U;
    for (uint8_t b = 0U; b < 8U; b++) {
        line2[idx++] = bar[b];
    }

    if (sugar_lvl == 0U) {
        /* "[      ] No sugar   " */
        const char *desc = " No sugar   ";
        for (uint8_t d = 0U; d < 12U; d++) {
            line2[idx++] = desc[d];
        }
    } else if (sugar_lvl == 1U) {
        /* "[##    ] Low    (H8)" */
        const char *desc = " Low    (H8)";
        for (uint8_t d = 0U; d < 12U; d++) {
            line2[idx++] = desc[d];
        }
    } else if (sugar_lvl == 2U) {
        /* "[####  ] Medium (H7)" */
        const char *desc = " Medium (H7)";
        for (uint8_t d = 0U; d < 12U; d++) {
            line2[idx++] = desc[d];
        }
    } else {
        /* "[######] High   (H6)" */
        const char *desc = " High   (H6)";
        for (uint8_t d = 0U; d < 12U; d++) {
            line2[idx++] = desc[d];
        }
    }
    line2[idx] = '\0';
    UI_FormatLine20(s_frame[1], line2);

    /* Line 3: Instruction */
    UI_FormatLine20(s_frame[2], "Turn POT to adjust");

    /* Line 4: Navigation prompts */
    UI_FormatLine20(s_frame[3], "SW1-3:START SW4:Back");
}

/* Active preparation status screens */
static void UI_RenderPreparation(const CoffeeMachineContext_t *ctx, CoffeeState_t state, uint32_t now)
{
    char line1[21];
    char line3[21];

    /* Line 1: Drink header */
    if (ctx->drink == DRINK_DOPPIO) {
        if (ctx->doppio_cycle == 1U) {
            UI_FormatLine20(line1, "Doppio (Cycle 1/2)");
        } else {
            UI_FormatLine20(line1, "Doppio (Cycle 2/2)");
        }
    } else if (ctx->drink == DRINK_LUNGO) {
        UI_FormatLine20(line1, "Lungo Preparation");
    } else {
        UI_FormatLine20(line1, "Espresso Preparation");
    }
    UI_FormatLine20(s_frame[0], line1);

    /* Line 2 & 3: Phase details */
    switch (state) {
    case STATE_FILL_COFFEE:
        UI_FormatLine20(s_frame[1], "Filling Coffee (SV1)");
        if (ctx->coffee_level == 1U) {
            UI_FormatLine20(s_frame[2], "Target: Mild   (H3)");
        } else if (ctx->coffee_level == 2U) {
            UI_FormatLine20(s_frame[2], "Target: Medium (H2)");
        } else {
            UI_FormatLine20(s_frame[2], "Target: Strong (H1)");
        }
        break;

    case STATE_FILL_WATER:
        UI_FormatLine20(s_frame[1], "Filling Water  (SV2)");
        UI_FormatLine20(s_frame[2], "Target: High   (H4)");
        break;

    case STATE_FILL_SUGAR:
        UI_FormatLine20(s_frame[1], "Filling Sugar  (SV3)");
        if (ctx->sugar_level == 1U) {
            UI_FormatLine20(s_frame[2], "Target: Low    (H8)");
        } else if (ctx->sugar_level == 2U) {
            UI_FormatLine20(s_frame[2], "Target: Medium (H7)");
        } else {
            UI_FormatLine20(s_frame[2], "Target: High   (H6)");
        }
        break;

    case STATE_DRAIN_TO_MIX:
        UI_FormatLine20(s_frame[1], "Draining to Mixer");
        UI_FormatLine20(s_frame[2], "Valve SV4 open...");
        break;

    case STATE_STIRRING: {
        UI_FormatLine20(s_frame[1], "Mixing Beverage");
        uint32_t elapsed = now - ctx->state_start_ms;
        uint32_t rem_ms = (elapsed < ctx->timer_duration_ms) ? (ctx->timer_duration_ms - elapsed) : 0U;
        uint8_t rem_s = (uint8_t)((rem_ms + 999U) / 1000U);

        memcpy(line3, "Time left: ", 11);
        UI_FormatDec2(rem_s, &line3[11]);
        line3[13] = 's';
        line3[14] = '\0';
        UI_FormatLine20(s_frame[2], line3);
        break;
    }

    case STATE_DISPENSING: {
        UI_FormatLine20(s_frame[1], "Dispensing to Cup");
        uint32_t elapsed = now - ctx->state_start_ms;
        uint32_t rem_ms = (elapsed < ctx->timer_duration_ms) ? (ctx->timer_duration_ms - elapsed) : 0U;
        uint8_t rem_s = (uint8_t)((rem_ms + 999U) / 1000U);

        memcpy(line3, "Time left: ", 11);
        UI_FormatDec2(rem_s, &line3[11]);
        line3[13] = 's';
        line3[14] = '\0';
        UI_FormatLine20(s_frame[2], line3);
        break;
    }

    case STATE_DONE:
        UI_FormatLine20(s_frame[0], "*** BEVERAGE READY *");
        UI_FormatLine20(s_frame[1], "Enjoy your drink!");
        UI_FormatLine20(s_frame[2], "Take cup from tray");
        UI_FormatLine20(s_frame[3], "Ready for next order");
        return;

    case STATE_STORNO_DRAIN_TANKS:
        UI_FormatLine20(s_frame[0], "!! STORNO ACTIVE !!");
        UI_FormatLine20(s_frame[1], "Draining tanks (SV4)");
        UI_FormatLine20(s_frame[2], "Safe shutdown...");
        UI_FormatLine20(s_frame[3], "Please wait...");
        return;

    case STATE_STORNO_DRAIN_MIX:
        UI_FormatLine20(s_frame[0], "!! STORNO ACTIVE !!");
        UI_FormatLine20(s_frame[1], "Draining mixer (SV5)");
        UI_FormatLine20(s_frame[2], "Safe shutdown...");
        UI_FormatLine20(s_frame[3], "Please wait...");
        return;

    default:
        break;
    }

    /* Line 4 for all active brewing phases: STORNO prompt */
    UI_FormatLine20(s_frame[3], "SW4: Cancel (STORNO)");
}

void Task_Display(void)
{
    uint32_t now = SYSTICK_millis();
    CoffeeState_t state = CoffeeMachine_GetState();
    const CoffeeMachineContext_t *ctx = CoffeeMachine_GetContext();

    /* Refresh immediately if state changed, or periodically every UI_REFRESH_INTERVAL_MS */
    if ((state == s_last_rendered_state) && ((now - s_last_refresh_ms) < UI_REFRESH_INTERVAL_MS)) {
        return;
    }
    s_last_refresh_ms = now;
    s_last_rendered_state = state;

    switch (state) {
    case STATE_IDLE:
        UI_RenderIdle();
        break;

    case STATE_WIZARD_COFFEE:
        UI_RenderWizardCoffee(ctx->drink, ctx->coffee_level);
        break;

    case STATE_WIZARD_SUGAR:
        UI_RenderWizardSugar(ctx->drink, ctx->sugar_level);
        break;

    default:
        UI_RenderPreparation(ctx, state, now);
        break;
    }

    /* Commit changes to physical LCD */
    UI_CommitScreen();
}
