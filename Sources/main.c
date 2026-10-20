/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "MKL25Z4.h"
#include "drv_systick.h"
#include "drv_gpio.h"
#include "drv_lcd.h"
#include "drv_kava.h"

/**
 * @brief Zakladni inicializace vsech pouzivanych periferii a ovladacu.
 */
static void App_Init(void)
{
    /* 1. Inicializace casovace SysTick (1 ms tik pro neblokujici casovani) */
    SYSTICK_initialize();

    /* 2. Inicializace GPIO ovladace pro tlacitka SW1-SW4 a LED na kitu */
    GPIO_Initialize();

    /* 3. Inicializace a zapnuti podsviceni znakoveho LCD displeje (4x20) */
    LCD_initialize();
    LCD_backlight_on();
    LCD_clear();

    /* 4. Inicializace rozhrani modelu misici jednotky (automat na kavu) */
    Kava_Init();

    /* Uvodni obrazovka na LCD displeji */
    LCD_set_cursor(1, 1);
    LCD_puts("Automat na kavu");
    LCD_set_cursor(2, 1);
    LCD_puts("Inicializace OK");
}

int main(void)
{
    /* Inicializace systemu a periferii */
    App_Init();

    /* Hlavni nekonecna smycka programu */
    for (;;) {
        /* Zde budou volany neblokujici ulohy (tasky) automatu */
    }

    /* Never leave main */
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
