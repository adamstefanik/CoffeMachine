# Projekt: Automat na kávu (EDU-mod Mísicí jednotka na FRDM-KL25Z) – GitHub Issues

Tento dokument obsahuje zoznam úloh (Issues) pre vývoj embedded softvéru automatu na kávu na výukovom kite **Freescale FRDM-KL25Z** s pripojeným prípravkom **EDU-mod Mísicí jednotka** (predmet *AP5ES – Embedded systémy s mikropočítači*).

---

## Prehľad hardvérového zapojenia (EDU-mod Mísicí jednotka)

### Výstupy (Ovládané prvky – log. 1 = zapnuté/otvorené, log. 0 = vypnuté/zatvorené)
| Prvok modelu | Funkcia v kávovare | Pin MCU |
|---|---|---|
| **Ventil SV1** | Napúšťanie `TANK1` (Kávový koncentrát / Espresso základ) | `PTE5` |
| **Ventil SV2** | Napúšťanie `TANK2` (Horúca voda pre Lungo) | `PTC1` |
| **Ventil SV3** | Napúšťanie `TANK3` (Cukor) | `PTE4` |
| **Ventil SV4** | Vypúšťanie nádrží `TANK1–TANK3` do miešacej nádoby `MIX` | `PTE0` |
| **Ventil SV5** | Vypúšťanie miešacej nádoby `MIX` do pohára (výdaj nápoja) | `PTE1` |
| **Míchadlo MIX** | Miešanie nápoja v nádobe `MIX` | `PTD0` |

### Vstupy (Hladinomery – log. 1 = hladina dosiahla snímač, log. 0 = pod snímačom)
| Nádrž | Snímač | Význam hladiny | Pin MCU |
|---|---|---|---|
| **TANK1 (Káva)** | `H1` | Horný snímač – silná káva (plná nádrž) | `PTD3` |
| **TANK1 (Káva)** | `H2` | Stredný snímač – stredná káva (polovica nádrže) | `PTC16` |
| **TANK1 (Káva)** | `H3` | Dolný snímač – slabá káva (minimum) | `PTD2` |
| **TANK2 (Voda)** | `H4` | Horný snímač – vysoká hladina vody (plná nádrž) | `PTC5` |
| **TANK2 (Voda)** | `H5` | Dolný snímač – nízka hladina vody (minimum) | `PTD4` |
| **TANK3 (Cukor)** | `H6` | Horný snímač – veľa cukru (plná nádrž) | `PTC7` |
| **TANK3 (Cukor)** | `H7` | Stredný snímač – stredne cukru (polovica nádrže) | `PTD5` |
| **TANK3 (Cukor)** | `H8` | Dolný snímač – málo cukru (minimum) | `PTC6` |

### Periférie výukového kitu FRDM-KL25Z
- **Tlačidlá:** `SW1` (`PTA4`), `SW2` (`PTA5`), `SW3` (`PTA16`), `SW4` (`PTA17`)
- **Potenciometer:** Analógový vstup na `ADC0` (`PTB0` / `PTB1` podľa zapojenia kitu)
- **LCD Displej (4×20 znakov):** `PTC8–PTC13`, `PTA12–PTA13` (`drv_lcd.c/.h`)

---

## Issue 1: Repository initialization, KDS project setup, and base drivers
- **Labels:** `setup`, `high-priority`
- **Priority:** High
- **Description:**
  Pripraviť štruktúru projektu pre Kinetis Design Studio (KDS) a nastaviť `.gitignore`, aby bolo možné projekt bez problémov skompilovať, ladiť na kite FRDM-KL25Z a verzovať v Gite.

  **Úlohy:**
  - Obnoviť a skontrolovať projektové súbory pre Kinetis Design Studio (`.project`, `.cproject`, `.settings/`, `Project_Settings/`).
  - Vytvoriť `.gitignore` pre build artefakty (`Debug/`, `Release/`, `.DS_Store`).
  - Overiť funkčnosť poskytnutých bázových ovládačov v `Sources/` (`drv_gpio.c/.h`, `drv_lcd.c/.h`, `drv_systick.c/.h`).
  - V `main.c` doplniť základnú inicializáciu systémových časovačov a periférií (`SYSTICK_initialize()`, `GPIO_Initialize()`, `LCD_initialize()`, `LCD_backlight_on()`).

---

## Issue 2: Implementation of mixing unit driver (drv_kava.c / drv_kava.h)
- **Labels:** `driver`, `gpio`, `high-priority`
- **Priority:** High (Hodnotenie: 2 body)
- **Description:**
  Vytvoriť plne funkčný hardvérový ovládač v súboroch `Sources/drv_kava.h` a `Sources/drv_kava.c` na úrovni registrov (`PORTx->PCR`, `PTx->PDDR`, `PTx->PSOR`, `PTx->PCOR`, `PTx->PDIR`) pre všetky ventily `SV1–SV5`, miešadlo `MIX` a hladinomery `H1–H8`.

  **Úlohy:**
  - V `drv_kava.h` opraviť a dodefinovať konštanty/enumy pre ventily (`VENTIL_KAVA` / `SV1` až `VENTIL_VYPUST` / `SV5`) a hladinomery (`HLADINA_H1` až `HLADINA_H8`) a doplniť ukončovací `#endif`.
  - Implementovať `void Kava_Init(void)`:
    - Povoliť hodinový signál pre porty B, C, D, E v registri `SIM->SCGC5`.
    - Nastaviť `PORT_PCR_MUX(1)` (GPIO režim) pre výstupy (`PTE5`, `PTC1`, `PTE4`, `PTE0`, `PTE1`, `PTD0`) a vstupy (`PTD3`, `PTC16`, `PTD2`, `PTC5`, `PTD4`, `PTC7`, `PTD5`, `PTC6`).
    - Nastaviť smer pinov v `PDDR` (výstupy na `1`, vstupy na `0`) a na začiatku bezpečne vypnúť všetky ventily a miešadlo (`PCOR`).
  - Implementovať `void Kava_NastavVentil(uint8_t ventil, bool otevrit)` pre otváranie (`true`) a zatváranie (`false`) ventilov `SV1–SV5`.
  - Implementovať `void Kava_NastavMichadlo(bool zapnout)` pre zapnutie/vypnutie miešadla na pine `PTD0`.
  - Implementovať `bool Kava_CtiHladinu(uint8_t hladinomer)` pre čítanie stavu snímačov `H1–H8` (vracia `true`, ak je na príslušnom pine log. 1).
  - Pridať pomocnú funkciu `void Kava_VypniVse(void)` na okamžité uzavretie všetkých ventilov a vypnutie miešadla.

---

## Issue 3: Implementation of ADC driver for potentiometer (drv_adc.c / drv_adc.h)
- **Labels:** `driver`, `adc`, `high-priority`
- **Priority:** High (Súčasť hodnotenia UI s potenciometrom: 2 body)
- **Description:**
  Vytvoriť samostatný modul `Sources/drv_adc.h` a `Sources/drv_adc.c` pre čítanie polohy potenciometra na výukovom kite pomocou prevodníka `ADC0` a mapovanie nameranej hodnoty na diskrétne úrovne intenzity kávy a cukru.

  **Úlohy:**
  - Vytvoriť `drv_adc.h` a `drv_adc.c`.
  - Implementovať `void ADC_Init(void)`:
    - Povoliť hodiny pre `ADC0` (`SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK`) a príslušný port potenciometra.
    - Nakonfigurovať `ADC0->CFG1` (napr. 8-bitové alebo 12-bitové rozlíšenie, delička hodín) a `ADC0->SC3` (hardvérové priemerovanie pre stabilitu hodnoty bez kmitania).
  - Implementovať `uint16_t ADC_ReadRaw(void)` pre spustenie konverzie na kanáli potenciometra, čakanie na príznak `COCO` a vrátenie výsledku z `ADC0->R[0]`.
  - Implementovať pomocné funkcie na prepočet hodnoty potenciometra:
    - Úroveň intenzity kávy (1 až 3 $\rightarrow$ zodpovedá snímačom `H3`, `H2`, `H1`).
    - Úroveň intenzity cukru (0 až 3 $\rightarrow$ `0 = bez cukru`, `1 = H8`, `2 = H7`, `3 = H6`) + prepočet na počet dielikov progress baru na LCD.

---

## Issue 4: Non-blocking task for SW1–SW4 button handling (Task Buttons)
- **Labels:** `feature`, `gpio`, `fsm`, `medium-priority`
- **Priority:** High
- **Status:** Dokončené (Implementované v `Sources/task_buttons.h` a `Sources/task_buttons.c`)
- **Description:**
  Vytvoriť neblokujúcu úlohu (task) pre čítanie tlačidiel `SW1`, `SW2`, `SW3` a `SW4` s ošetrením zákmitov (debouncing) a detekciou nábežnej/falling hrany stlačenia, aby program reagoval okamžite a bez blokujúceho čakania.

  **Úlohy:**
  - [x] Nakonfigurovať piny tlačidiel `SW1` (`PTA4`), `SW2` (`PTA5`), `SW3` (`PTA16`) a `SW4` (`PTA17`) cez `pinMode(..., INPUT_PULLUP)` z `drv_gpio.h`.
  - [x] Implementovať periodickú neblokujúcu funkciu `Task_Buttons(void)` volanú pomocou `SYSTICK_millis()` (vzorkovanie každých 10 ms, debounce filter 20 ms).
  - [x] Detegovať jednorazové stlačenie (zmenu stavu z uvoľneného na stlačené tlačidlo cez `Task_Buttons_WasPressed()`), aby držanie tlačidla nespôsobilo viacnásobné preskočenie krokov vo Wizarde.
  - [x] Pri stlačení tlačidla `SW4` nastaviť príznak okamžitého prerušenia / návratu (`STORNO` cez `Task_Buttons_IsStorno()` / `Task_Buttons_GetStorno()`).

---

## Issue 5: LCD user interface and setup wizard (Wizard with progress bars)
- **Labels:** `ui/lcd`, `feature`, `high-priority`
- **Priority:** High (Hodnotenie: 2 body)
- **Description:**
  Implementovať prehľadné používateľské rozhranie na 4×20 LCD displeji vrátane sprievodcu (Wizardu) pred spustením prípravy nápoja, kde si používateľ potenciometrom nastaví intenzitu kávy a intenzitu cukru zobrazenú pomocou grafického progress baru.

  **Úlohy:**
  - Navrhnúť vykresľovanie 4 riadkov LCD displeja bez zbytočného blikania (obnovovať pri zmene stavu alebo periodicky každých ~100–150 ms).
  - Implementovať obrazovky Wizardu:
    1. **Výber nápoja (`STATE_IDLE`):**
       - Zobrazenie ponuky: `SW1: Espresso`, `SW2: Lungo`, `SW3: Doppio`.
    2. **Nastavenie intenzity kávy (`STATE_WIZARD_COFFEE`):**
       - Zobrazenie vybraného nápoja, grafického baru intenzity kávy podľa potenciometra (napr. `[###   ] Stredna (H2)`) a pokynu na potvrdenie (`SW1-3: OK`, `SW4: Zpet`).
    3. **Nastavenie intenzity cukru (`STATE_WIZARD_SUGAR`):**
       - Zobrazenie grafického baru cukru podľa potenciometra (`0 = Bez cukru`, `1 = Malo (H8)`, `2 = Stredne (H7)`, `3 = Vela (H6)`) a potvrdenie štartu prípravy (`SW1-3: START`, `SW4: Storno`).
  - Implementovať stavové obrazovky počas prípravy nápoja (aktuálna fáza: plnenie kávy/vody/cukru, vypúšťanie do mixéra, odpočet času miešania, čapovanie do pohára, prípadne stav `STORNO`).

---

## Issue 6: Finite state machine (FSM) and implementation of 3 drink programs (Espresso, Lungo, Doppio)
- **Labels:** `fsm`, `feature`, `high-priority`
- **Priority:** High (Hodnotenie: 2 b za FSM + 2 b za 3 odlišné programy)
- **Description:**
  Navrhnúť a implementovať hlavný neblokujúci konečný automat (`Task_CoffeeMachine`) s 3 nápojovými programami, ktoré majú odlišnú riadiacu sekvenciu.

  **Úlohy:**
  - Definovať výčtový typ stavov automatu (`CoffeeState_t`) a štruktúru kontextu objednávky (zvolený nápoj, cieľový snímač kávy `H3/H2/H1`, cieľový snímač cukru `0/H8/H7/H6`, počítadlo cyklov pre Doppio, časovače pre `SYSTICK_millis()`).
  - Implementovať **Program 1 – Espresso (bez `TANK2`):**
    1. Napustenie kávy `TANK1` (`SV1`) po zvolenú hladinu (`H3`, `H2` alebo `H1`).
    2. Ak je zvolený cukor ($>0$), napustenie cukru `TANK3` (`SV3`) po zvolenú hladinu (`H8`, `H7` alebo `H6`).
    3. Vypustenie surovín do miešacej nádoby (`SV4`) – čakanie na `H3 == 0 && H8 == 0` + časový presah (napr. 3 s).
    4. Miešanie v nádobe `MIX` po dobu 3 s.
    5. Výdaj nápoja do pohára (`SV5`) po dobu zodpovedajúcu objemu + návrat do `STATE_IDLE`.
  - Implementovať **Program 2 – Lungo (s horúcou vodou `TANK2` na `H4`):**
    1. Napustenie kávy `TANK1` (`SV1`) po zvolenú hladinu (`H3/H2/H1`).
    2. Napustenie horúcej vody `TANK2` (`SV2`) na vysokú hladinu `H4`.
    3. Ak je zvolený cukor ($>0$), napustenie cukru `TANK3` (`SV3`) po zvolenú hladinu (`H8/H7/H6`).
    4. Vypustenie všetkých nádrží do mixéra (`SV4`) – čakanie na `H3 == 0 && H5 == 0 && H8 == 0` + časový presah.
    5. Miešanie v nádobe `MIX` po dobu 5 s.
    6. Výdaj nápoja do pohára (`SV5`) + návrat do `STATE_IDLE`.
  - Implementovať **Program 3 – Doppio (2× cyklus kávy z `TANK1`, bez `TANK2`):**
    1. **1. cyklus kávy:** Napustenie `TANK1` (`SV1`) po zvolenú hladinu (`H3/H2/H1`) a prečerpanie cez `SV4` do mixéra.
    2. **2. cyklus kávy + cukor:** Opätovné napustenie `TANK1` (`SV1`) po zvolenú hladinu + napustenie cukru `TANK3` (`SV3`, ak $>0$) a prečerpanie cez `SV4` do mixéra (čakanie na `H3 == 0 && H8 == 0` + časový presah).
    3. Miešanie dvojitej dávky v nádobe `MIX` po dobu 6 s.
    4. Výdaj nápoja do pohára (`SV5`) + návrat do `STATE_IDLE`.
  - Zabezpečiť, aby v žiadnom stave nebolo použité blokujúce čakanie (`while` / `SYSTICK_delay_ms`).

---

## Issue 7: Immediate program cancellation (STORNO) and safe draining of the unit
- **Labels:** `safety`, `fsm`, `high-priority`
- **Priority:** High (Hodnotenie: 2 body)
- **Description:**
  Zabezpečiť okamžitú odozvu na stlačenie tlačidla `SW4` (STORNO) v ľubovoľnom okamihu behu programu a korektné ukončenie riadenia modelu tak, aby v nádržiach ani v mixéri nezostala kvapalina.

  **Úlohy:**
  - Ak používateľ stlačí `SW4` počas nastavovania vo Wizarde (ešte pred otvorením ventilov), okamžite zrušiť voľbu a vrátiť automat do `STATE_IDLE`.
  - Ak používateľ stlačí `SW4` počas ktorejkoľvek fázy prípravy nápoja:
    1. Okamžite zatvoriť napúšťacie ventily `SV1`, `SV2`, `SV3` a vypnúť miešadlo `MIX`.
    2. Prepnúť automat do stavu `STATE_STORNO_DRAIN_TANKS` a zobraziť na LCD upozornenie `!! STORNO !! Vypoustim nadrze...`.
    3. Otvoriť ventil `SV4`, počkať kým `H3 == 0 && H5 == 0 && H8 == 0` a pridať časový presah (napr. 3 s), potom `SV4` zatvoriť.
    4. Prepnúť do stavu `STATE_STORNO_DRAIN_MIX`, otvoriť výpustný ventil `SV5` na bezpečný čas vyprázdnenia mixéra (napr. 6–8 s) a následne `SV5` zatvoriť.
    5. Vrátiť automat do východiskového stavu `STATE_IDLE`.

---

## Issue 8: Task integration in main.c, code commenting, and testing on EDU-mod fixture
- **Labels:** `testing`, `documentation`, `medium-priority`
- **Priority:** Medium
- **Description:**
  Prepojiť všetky moduly a neblokujúce úlohy v hlavnej slučke `main.c`, doplniť prehľadné komentáre k funkciám a stavom automatu a otestovať hraničné situácie na reálnom modeli.

  **Úlohy:**
  - V `main.c` zostaviť hlavnú nekonečnú slučku `for (;;)` volajúcu jednotlivé neblokujúce úlohy (`Task_Buttons()`, `Task_Potentiometer()`, `Task_CoffeeMachine()`, `Task_Display()`).
  - Otestovať na prípravku EDU-mod časové presahy pri vypúšťaní `SV4` a `SV5`, aby nedochádzalo k rozsvieteniu chybovej LED `ERR` (pretečeniu nádrže alebo mixéra).
  - Skontrolovať čitateľnosť kódu, hlavičkové komentáre a dokumentáciu podľa požiadaviek zadania.
