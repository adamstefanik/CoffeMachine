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
#include "drv_adc.h"
#include "task_buttons.h"

/**
 * @brief Zakladni inicializace vsech pouzivanych periferii a ovladacu.
 */
static void App_Init(void)
{
    /* 1. Inicializace casovace SysTick (1 ms tik pro neblokujici casovani) */
    SYSTICK_initialize();

    /* 2. Inicializace GPIO ovladace pro tlacitka SW1-SW4 a LED na kitu */
    GPIO_Initialize();
    Task_Buttons_Init(); /* Nakonfiguruje SW1-SW4 (INPUT_PULLUP) a pociatocny stav debouncingu */
    pinMode(LD1, OUTPUT);
    pinWrite(LD1, LOW);

    /* 3. Inicializace rozhrani modelu misici jednotky (automat na kavu) */
    Kava_Init();

    /* 4. Inicializace prevodniku ADC0 pro potenciometr */
    ADC_Init();

    /* 5. Inicializace a zapnuti podsviceni znakoveho LCD displeje (4x20) */
    LCD_initialize();
    LCD_backlight_on();
    LCD_clear();

    /* Uvodni obrazovka na LCD displeji (kazdy radek max 20 znaku!) */
    LCD_set_cursor(1, 1);
    LCD_puts("Automat na kavu");
    LCD_set_cursor(2, 1);
    LCD_puts("SW1:Kava SW2:Voda");
}

/**
 * @brief Pomocna funkce pro formatovani 3-ciferneho cisla (0 az 255) do retezce.
 */
static void FormatDec3(uint8_t val, char *out)
{
    out[0] = (char)('0' + (val / 100U));
    out[1] = (char)('0' + ((val / 10U) % 10U));
    out[2] = (char)('0' + (val % 10U));
}

/**
 * @brief Testovaci uloha pro overeni komunikace s MCU, potenciometrem a modelem.
 *        - Radek 1: "Automat na kavu"
 *        - Radek 2: "SW1:Kava SW2:Voda"
 *        - Radek 3: P0:xxx P1:yyy [##  ] (hodnoty kanalu PTB0 a PTB1 a bar podle aktivniho)
 *        - Radek 4: T1:xxx T2:xx T3:xxx (stavy hladinomeru)
 */
static void Test_HardwareLoop(void)
{
    static uint32_t last_lcd_ms = 0U;
    uint32_t now = SYSTICK_millis();

    /* Odfiltrovana stabilni tlacitka na kitu (debouncing z Task_Buttons) */
    bool sw1_pressed = Task_Buttons_IsDown(BTN_SW1);
    bool sw2_pressed = Task_Buttons_IsDown(BTN_SW2);
    bool sw3_pressed = Task_Buttons_IsDown(BTN_SW3);
    bool sw4_pressed = Task_Buttons_IsDown(BTN_SW4);

    /* SV1 (Kava - TANK1): otevreno pri drzeni SW1, ochrana proti preteceni nad H1 */
    Kava_NastavVentil(VENTIL_KAVA, sw1_pressed && !Kava_CtiHladinu(HLADINA_H1));

    /* SV2 (Voda - TANK2) a SV3 (Cukr - TANK3): otevreno pri drzeni SW2, ochrana nad H4/H6 */
    Kava_NastavVentil(VENTIL_VODA, sw2_pressed && !Kava_CtiHladinu(HLADINA_H4));
    Kava_NastavVentil(VENTIL_CUKR, sw2_pressed && !Kava_CtiHladinu(HLADINA_H6));

    /* SV4 (do mixeru) + Michadlo MIX: aktivni pri drzeni SW3 */
    Kava_NastavVentil(VENTIL_MIX, sw3_pressed);
    Kava_NastavMichadlo(sw3_pressed);

    /* SV5 (vypust z mixeru): aktivni pri drzeni SW4 */
    Kava_NastavVentil(VENTIL_VYPUST, sw4_pressed);

    /* Indikace stisku libovolneho tlacitka na LED LD1 */
    pinWrite(LD1, (sw1_pressed || sw2_pressed || sw3_pressed || sw4_pressed) ? HIGH : LOW);

    /* Obnoveni LCD kazdych 100 ms */
    if ((now - last_lcd_ms) >= 100U) {
        char radek3[21];
        char radek4[21];
        last_lcd_ms = now;

        /* Precteme 8-bit hodnoty z PTB0 (kanál 8) i PTB1 (kanál 9) */
        uint8_t val_ptb0 = (uint8_t)ADC_ReadChannelRaw(ADC_CH_PTB0);
        uint8_t val_ptb1 = (uint8_t)ADC_ReadChannelRaw(ADC_CH_PTB1);

        /* Zformatujeme radek 3: "P0:xxx P1:yyy [##  ]" (presne 20 znaku) */
        radek3[0] = 'P';
        radek3[1] = '0';
        radek3[2] = ':';
        FormatDec3(val_ptb0, &radek3[3]);
        radek3[6] = ' ';
        radek3[7] = 'P';
        radek3[8] = '1';
        radek3[9] = ':';
        FormatDec3(val_ptb1, &radek3[10]);
        radek3[13] = ' ';

        /* Progress bar podle zvoleneho kanalu potenciometru (6 znaku: "[####]") */
        char bar[10];
        uint8_t coffee_lvl = ADC_GetCoffeeLevel();
        ADC_FormatProgressBar(coffee_lvl, 3, bar, 4);
        for (uint8_t b = 0; b < 6; b++) {
            radek3[14 + b] = bar[b];
        }
        radek3[20] = '\0';

        LCD_set_cursor(3, 1);
        LCD_puts(radek3);

        /* Format radek 4: "T1:000 T2:00 T3:000 " (presne 20 znaku) */
        radek4[0]  = 'T';
        radek4[1]  = '1';
        radek4[2]  = ':';
        radek4[3]  = Kava_CtiHladinu(HLADINA_H1) ? '1' : '0';
        radek4[4]  = Kava_CtiHladinu(HLADINA_H2) ? '1' : '0';
        radek4[5]  = Kava_CtiHladinu(HLADINA_H3) ? '1' : '0';
        radek4[6]  = ' ';
        radek4[7]  = 'T';
        radek4[8]  = '2';
        radek4[9]  = ':';
        radek4[10] = Kava_CtiHladinu(HLADINA_H4) ? '1' : '0';
        radek4[11] = Kava_CtiHladinu(HLADINA_H5) ? '1' : '0';
        radek4[12] = ' ';
        radek4[13] = 'T';
        radek4[14] = '3';
        radek4[15] = ':';
        radek4[16] = Kava_CtiHladinu(HLADINA_H6) ? '1' : '0';
        radek4[17] = Kava_CtiHladinu(HLADINA_H7) ? '1' : '0';
        radek4[18] = Kava_CtiHladinu(HLADINA_H8) ? '1' : '0';
        radek4[19] = ' ';
        radek4[20] = '\0';

        LCD_set_cursor(4, 1);
        LCD_puts(radek4);
    }
}

int main(void)
{
    /* Inicializace systemu a periferii */
    App_Init();

    /* Hlavni nekonecna smycka programu */
    for (;;) {
        /* 1. Periodicka neblokujici obsluha a debouncing tlacitek (10 ms vzorkovani) */
        Task_Buttons();

        /* 2. Aktualni testovaci smycka hardware */
        Test_HardwareLoop();
    }

    /* Never leave main */
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
