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
#include "task_buttons.h"
#include "drv_kava.h"
#include "drv_adc.h"
#include "coffee_machine.h"
#include "coffee_ui.h"

/**
 * @brief Zakladni inicializace vsech pouzivanych periferii, ovladacu a uloh.
 */
static void App_Init(void)
{
    /* 1. Inicializace casovace SysTick (1 ms tik pro neblokujici casovani) */
    SYSTICK_initialize();

    /* 2. Inicializace GPIO a ovladace tlacitek SW1-SW4 s debouncingem */
    GPIO_Initialize();
    Task_Buttons_Init();

    /* 3. Inicializace uzivatelske LED LD1 na desce kitu pro indikaci */
    pinMode(LD1, OUTPUT);
    pinWrite(LD1, LOW);

    /* 4. Inicializace rozhrani modelu misici jednotky (automat na kavu) */
    Kava_Init();

    /* 5. Inicializace prevodniku ADC0 pro potenciometr */
    ADC_Init();

    /* 6. Inicializace stavoveho automatu kávovaru */
    CoffeeMachine_Init();

    /* 7. Inicializace a uvodni vykresleni uzivatelskeho rozhrani na LCD (4x20) */
    UI_Init();
}

/**
 * @brief Pomocna neblokujici uloha pro indikaci stisku tlacitek na LED LD1.
 */
static void Task_StatusLed(void)
{
    bool any_button = Task_Buttons_IsDown(BTN_SW1) || Task_Buttons_IsDown(BTN_SW2) ||
                      Task_Buttons_IsDown(BTN_SW3) || Task_Buttons_IsDown(BTN_SW4);
    pinWrite(LD1, any_button ? HIGH : LOW);
}

int main(void)
{
    /* Inicializace systemu a periferii */
    App_Init();

    /* Hlavni nekonecna smycka programu - vykonavani neblokujicich uloh */
    for (;;) {
        Task_Buttons();         /* Snimani a debouncing tlacitek SW1 az SW4 */
        Task_CoffeeMachine();   /* Stavovy automat rizeni pripravy napoju */
        Task_Display();         /* Nekmitave vykreslovani 4-radkoveho LCD a Wizardu */
        Task_StatusLed();       /* Vizuální indikace aktivity na LED kitu */
    }

    /* Never leave main */
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
