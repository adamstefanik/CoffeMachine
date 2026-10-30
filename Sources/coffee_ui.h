/*
 * @file     coffee_ui.h
 * @brief    LCD user interface and setup wizard rendering (4x20 character display)
 */

#ifndef COFFEE_UI_H_
#define COFFEE_UI_H_

#include <stdint.h>
#include <stdbool.h>
#include "coffee_machine.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the LCD user interface, clears hardware display,
 *        and initializes the non-flicker differential shadow buffer.
 */
void UI_Init(void);

/**
 * @brief Forces a complete hardware redraw of all 4 lines on the next update.
 */
void UI_ForceRefresh(void);

/**
 * @brief Periodic non-blocking display rendering task (~100-120 ms interval).
 *        Detects state and potentiometer changes and updates the 4x20 LCD without flickering.
 */
void Task_Display(void);

#ifdef __cplusplus
}
#endif

#endif /* COFFEE_UI_H_ */
