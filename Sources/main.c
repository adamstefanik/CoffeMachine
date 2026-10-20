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
    pinMode(SW1, INPUT_PULLUP);
    pinMode(SW2, INPUT_PULLUP);
    pinMode(SW3, INPUT_PULLUP);
    pinMode(SW4, INPUT_PULLUP);
    pinMode(LD1, OUTPUT);

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
    LCD_puts("SW1:SV1 SW2:SV2+SV3");
    LCD_set_cursor(3, 1);
    LCD_puts("SW3:SV4+MIX SW4:SV5");
}

/**
 * @brief Testovaci uloha pro overeni komunikace s MCU a modelem misici jednotky
 *        (podle kroku 4 v Projekt1_navod_r2.pdf).
 *        - Drzeni SW1: otevre SV1 (plni TANK1, dokud nedosahne H1)
 *        - Drzeni SW2: otevre SV2 a SV3 (plni TANK2 a TANK3, dokud nedosahnou H4/H6)
 *        - Drzeni SW3: otevre SV4 (vypousti TANK1-3 do MIXu) a zapne michadlo MIX
 *        - Drzeni SW4: otevre SV5 (vypousti MIX do kelimku)
 *        Na 4. radku LCD prubezne zobrazuje stavy hladinomeru H1 az H8.
 */
static void Test_HardwareLoop(void)
{
    static uint32_t last_lcd_ms = 0U;
    uint32_t now = SYSTICK_millis();

    /* Tlacitka na kitu jsou aktivni v log. 0 (stisknuto = LOW) */
    bool sw1_pressed = (pinRead(SW1) == LOW);
    bool sw2_pressed = (pinRead(SW2) == LOW);
    bool sw3_pressed = (pinRead(SW3) == LOW);
    bool sw4_pressed = (pinRead(SW4) == LOW);

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

    /* Obnoveni stavu snimacu H1-H8 na 4. radku LCD kazdych 150 ms */
    if ((now - last_lcd_ms) >= 150U) {
        char radek4[21];
        last_lcd_ms = now;

        /* Format: "H1-3:000 45:00 6-8:000" (20 znaku: "T1:000 T2:00 T3:000 ") */
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
        radek4[19] = '\0';

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
        Test_HardwareLoop();
    }

    /* Never leave main */
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
