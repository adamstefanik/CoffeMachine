# Project: Coffee Machine Controller (EDU-mod Mixing Unit on FRDM-KL25Z) – GitHub Issues

This document contains the breakdown of tasks (Issues) for the embedded firmware development of the coffee machine on the **Freescale FRDM-KL25Z** development board with the attached **EDU-mod Mixing Unit** fixture (*AP5ES – Embedded Systems with Microcontrollers* course).

---

## Hardware Pin Mapping Overview (EDU-mod Mixing Unit)

### Outputs (Actuators – logic 1 = ON/open, logic 0 = OFF/closed)
| Actuator | Function in Coffee Machine | MCU Pin |
|---|---|---|
| **Valve SV1** | Fill `TANK1` (Coffee concentrate / Espresso base) | `PTE5` |
| **Valve SV2** | Fill `TANK2` (Hot water for Lungo) | `PTC1` |
| **Valve SV3** | Fill `TANK3` (Sugar) | `PTE4` |
| **Valve SV4** | Drain reservoirs `TANK1–TANK3` into mixing chamber `MIX` | `PTE0` |
| **Valve SV5** | Dispense mixing chamber `MIX` into cup | `PTE1` |
| **Stirrer MIX** | Stir beverage inside mixing chamber `MIX` | `PTD0` |

### Inputs (Level Sensors – logic 1 = liquid reached sensor, logic 0 = below sensor)
| Tank | Sensor | Level Meaning | MCU Pin |
|---|---|---|---|
| **TANK1 (Coffee)** | `H1` | Upper sensor – strong coffee (full tank) | `PTD3` |
| **TANK1 (Coffee)** | `H2` | Middle sensor – medium coffee (half tank) | `PTC16` |
| **TANK1 (Coffee)** | `H3` | Lower sensor – mild coffee (minimum) | `PTD2` |
| **TANK2 (Water)** | `H4` | Upper sensor – high water level (full tank) | `PTC5` |
| **TANK2 (Water)** | `H5` | Lower sensor – low water level (minimum) | `PTD4` |
| **TANK3 (Sugar)** | `H6` | Upper sensor – high sugar level (full tank) | `PTC7` |
| **TANK3 (Sugar)** | `H7` | Middle sensor – medium sugar level (half tank) | `PTD5` |
| **TANK3 (Sugar)** | `H8` | Lower sensor – low sugar level (minimum) | `PTC6` |

### FRDM-KL25Z On-Board Peripherals
- **Push Buttons:** `SW1` (`PTA4`), `SW2` (`PTA5`), `SW3` (`PTA16`), `SW4` (`PTA17`)
- **Potentiometer:** Analog input on `ADC0` (`PTB0` / `PTB1` depending on board jumper)
- **LCD Display (4×20 characters):** `PTC8–PTC13`, `PTA12–PTA13` (`drv_lcd.c/.h`)

---

## Issue 1: Repository initialization, KDS project setup, and base drivers
- **Labels:** `setup`, `high-priority`
- **Priority:** High
- **Description:**
  Prepare the project structure for Kinetis Design Studio (KDS) and configure `.gitignore` so the project can be seamlessly compiled, debugged on the **FRDM-KL25Z** board, and versioned in Git.

  **Tasks:**
  - [ ] Restore and verify project configuration files for Kinetis Design Studio (`.project`, `.cproject`, `.settings/`, `Project_Settings/`).
  - [ ] Create `.gitignore` for build artifacts (`Debug/`, `Release/`, `.DS_Store`).
  - [ ] Verify the functionality of provided base drivers in `Sources/` (`drv_gpio.c/.h`, `drv_lcd.c/.h`, `drv_systick.c/.h`).
  - [ ] In `main.c`, add basic initialization of system timers and peripherals (`SYSTICK_initialize()`, `GPIO_Initialize()`, `LCD_initialize()`, `LCD_backlight_on()`).

---

## Issue 2: Implementation of mixing unit driver (drv_kava.c / drv_kava.h)
- **Labels:** `driver`, `gpio`, `high-priority`
- **Priority:** High (Assignment grading: 2 points)
- **Description:**
  Create a fully functional hardware driver in `Sources/drv_kava.h` and `Sources/drv_kava.c` at the register level (`PORTx->PCR`, `PTx->PDDR`, `PTx->PSOR`, `PTx->PCOR`, `PTx->PDIR`) for all valves `SV1–SV5`, mixer `MIX`, and level sensors `H1–H8`.

  **Tasks:**
  - [ ] In `drv_kava.h`, fix and define constants/enums for valves (`VENTIL_KAVA` / `SV1` to `VENTIL_VYPUST` / `SV5`) and level sensors (`HLADINA_H1` to `HLADINA_H8`) and add missing terminating `#endif`.
  - [ ] Implement `void Kava_Init(void)`:
    - Enable clock gating for ports B, C, D, E in register `SIM->SCGC5`.
    - Configure `PORT_PCR_MUX(1)` (GPIO mode) for all model outputs and inputs.
    - Set pin directions in `PDDR` (outputs to `1`, inputs to `0`) and safely close all valves and stop mixer initially (`PCOR`).
  - [ ] Implement `void Kava_NastavVentil(uint8_t ventil, bool otevrit)` to open (`true`) and close (`false`) valves `SV1–SV5`.
  - [ ] Implement `void Kava_NastavMichadlo(bool zapnout)` to enable/disable the mixer motor on pin `PTD0`.
  - [ ] Implement `bool Kava_CtiHladinu(uint8_t hladinomer)` to read the state of sensors `H1–H8`.
  - [ ] Add helper function `void Kava_VypniVse(void)` to immediately close all valves and stop the mixer.

---

## Issue 3: Implementation of ADC driver for potentiometer (drv_adc.c / drv_adc.h)
- **Labels:** `driver`, `adc`, `high-priority`
- **Priority:** High (Part of UI grading with potentiometer: 2 points)
- **Description:**
  Create a dedicated module `Sources/drv_adc.h` and `Sources/drv_adc.c` for reading the potentiometer position on the development board using converter `ADC0` and mapping the sampled value to discrete levels of coffee strength and sugar intensity.

  **Tasks:**
  - [ ] Create `drv_adc.h` and `drv_adc.c`.
  - [ ] Implement `void ADC_Init(void)`:
    - Enable clock gating for `ADC0` (`SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK`) and the corresponding potentiometer port.
    - Configure `ADC0->CFG1` (e.g. 8-bit or 12-bit resolution, clock divider) and `ADC0->SC3` (hardware averaging for stable values without jitter).
  - [ ] Implement `uint16_t ADC_ReadRaw(void)` to trigger conversion on potentiometer channel, poll for `COCO` flag, and return result from `ADC0->R[0]`.
  - [ ] Implement helper functions for potentiometer value scaling:
    - Coffee strength level (1 to 3 -> corresponds to sensors `H3`, `H2`, `H1`).
    - Sugar intensity level (0 to 3 -> `0 = No sugar`, `1 = H8`, `2 = H7`, `3 = H6`) + conversion to progress bar segments on LCD.

---

## Issue 4: Non-blocking task for SW1–SW4 button handling (Task Buttons)
- **Labels:** `feature`, `gpio`, `fsm`, `medium-priority`
- **Priority:** High
- **Description:**
  Create a non-blocking task for reading push buttons `SW1`, `SW2`, `SW3`, and `SW4` with software debouncing and edge detection (rising/falling edge), allowing the application to respond immediately without blocking delays.

  **Tasks:**
  - [x] Configure button pins `SW1` (`PTA4`), `SW2` (`PTA5`), `SW3` (`PTA16`), and `SW4` (`PTA17`) via `pinMode(..., INPUT_PULLUP)` from `drv_gpio.h`.
  - [x] Implement periodic non-blocking function `Task_Buttons(void)` invoked via `SYSTICK_millis()` (e.g. every 10–20 ms).
  - [x] Detect single-press events (transition from released to pressed) so that holding a button does not cause unwanted step skips in the Wizard.
  - [x] On `SW4` press, trigger immediate cancellation / return flag (`STORNO`).

---

## Issue 5: LCD user interface and setup wizard (Wizard with progress bars)
- **Labels:** `ui/lcd`, `feature`, `high-priority`
- **Priority:** High (Assignment grading: 2 points)
- **Description:**
  Implement a clear user interface on the 4x20 character LCD, including an interactive setup wizard prior to beverage preparation, allowing the user to configure coffee strength and sugar level with the potentiometer and visualized via graphical progress bars.

  **Tasks:**
  - [x] Design non-flickering 4-line LCD rendering (refresh on state changes or periodically every ~100–150 ms).
  - [x] Implement Wizard screens:
    1. **Drink Selection (`STATE_IDLE`):**
       - Display options: `SW1: Espresso`, `SW2: Lungo`, `SW3: Doppio`.
    2. **Coffee Strength Adjustment (`STATE_WIZARD_COFFEE`):**
       - Display selected beverage, graphical coffee strength bar based on potentiometer (e.g. `[###   ] Medium (H2)`), and confirmation prompts (`SW1-3: OK`, `SW4: Back`).
    3. **Sugar Level Adjustment (`STATE_WIZARD_SUGAR`):**
       - Display graphical sugar bar based on potentiometer (`0 = No sugar`, `1 = Low (H8)`, `2 = Medium (H7)`, `3 = High (H6)`), and start confirmation (`SW1-3: START`, `SW4: Cancel`).
  - [x] Implement active preparation status screens (current phase: filling coffee/water/sugar, draining to mixer, mixing countdown, dispensing to cup, and `STORNO` status).

---

## Issue 6: Finite state machine (FSM) and implementation of 3 drink programs (Espresso, Lungo, Doppio)
- **Labels:** `fsm`, `feature`, `high-priority`
- **Priority:** High (Assignment grading: 2 pts for FSM + 2 pts for 3 distinct beverage programs)
- **Description:**
  Design and implement the main non-blocking finite state machine (`Task_CoffeeMachine`) with 3 distinct beverage programs, each following a specific control sequence.

  **Tasks:**
  - [x] Define state enum (`CoffeeState_t`) and order context structure (selected drink, target coffee sensor `H3/H2/H1`, target sugar sensor `0/H8/H7/H6`, cycle counter for Doppio, timestamps for `SYSTICK_millis()`).
  - [x] Implement **Program 1 – Espresso (without `TANK2`):**
    1. Fill coffee `TANK1` (`SV1`) up to target sensor (`H3`, `H2`, or `H1`).
    2. If sugar selected (`> 0`), fill sugar `TANK3` (`SV3`) up to target sensor (`H8`, `H7`, or `H6`).
    3. Drain ingredients into mixing chamber (`SV4`) – wait for `H3 == 0 && H8 == 0` + drain grace period (3 s).
    4. Stir in mixing chamber `MIX` for 3 s.
    5. Dispense drink into cup (`SV5`) for volume-equivalent duration + transition back to `STATE_IDLE`.
  - [x] Implement **Program 2 – Lungo (with hot water `TANK2` up to `H4`):**
    1. Fill coffee `TANK1` (`SV1`) up to target sensor (`H3/H2/H1`).
    2. Fill hot water `TANK2` (`SV2`) up to high level `H4`.
    3. If sugar selected (`> 0`), fill sugar `TANK3` (`SV3`) up to target sensor (`H8/H7/H6`).
    4. Drain all reservoirs into mixing chamber (`SV4`) – wait for `H3 == 0 && H5 == 0 && H8 == 0` + grace period.
    5. Stir in mixing chamber `MIX` for 5 s.
    6. Dispense drink into cup (`SV5`) + transition back to `STATE_IDLE`.
  - [x] Implement **Program 3 – Doppio (2x coffee cycle from `TANK1`, without `TANK2`):**
    1. **Cycle 1 (Coffee):** Fill `TANK1` (`SV1`) to target sensor (`H3/H2/H1`) and transfer via `SV4` to mixer.
    2. **Cycle 2 (Coffee + Sugar):** Fill `TANK1` (`SV1`) to target sensor again + fill sugar `TANK3` (`SV3`, if `> 0`), and transfer via `SV4` to mixer (wait for `H3 == 0 && H8 == 0` + grace period).
    3. Stir double batch in mixing chamber `MIX` for 6 s.
    4. Dispense drink into cup (`SV5`) + transition back to `STATE_IDLE`.
  - [x] Ensure no blocking calls (`while` loops / `SYSTICK_delay_ms`) are used in any state.

---

## Issue 7: Immediate program cancellation (STORNO) and safe draining of the unit
- **Labels:** `safety`, `fsm`, `high-priority`
- **Priority:** High (Assignment grading: 2 points)
- **Description:**
  Ensure immediate system response to `SW4` (`STORNO`) button press at any moment during execution and implement a clean, fail-safe shutdown routine leaving zero liquid inside reservoirs or mixing chamber.

  **Tasks:**
  - [ ] If user presses `SW4` during setup in Wizard (prior to opening any valves), immediately cancel selection and return to `STATE_IDLE`.
  - [ ] If user presses `SW4` during any phase of beverage preparation:
    1. Immediately close supply valves `SV1`, `SV2`, `SV3` and stop mixer `MIX`.
    2. Transition FSM to `STATE_STORNO_DRAIN_TANKS` and display alert on LCD: `!! STORNO !! Draining tanks...`.
    3. Open valve `SV4`, wait until `H3 == 0 && H5 == 0 && H8 == 0`, append grace period (e.g. 3 s), then close `SV4`.
    4. Transition to `STATE_STORNO_DRAIN_MIX`, open dispense valve `SV5` for safe emptying duration (e.g. 6–8 s), then close `SV5`.
    5. Return FSM to default state `STATE_IDLE`.

---

## Issue 8: Task integration in main.c, code commenting, and testing on EDU-mod fixture
- **Labels:** `testing`, `documentation`, `medium-priority`
- **Priority:** Medium
- **Description:**
  Integrate all peripheral modules and non-blocking tasks into the main loop of `main.c`, document functions and FSM states with clean comments, and test edge cases on the physical hardware fixture.

  **Tasks:**
  - [ ] In `main.c`, construct the main infinite loop `for (;;)` dispatching non-blocking tasks (`Task_Buttons()`, `Task_Potentiometer()`, `Task_CoffeeMachine()`, `Task_Display()`).
  - [ ] Test drain timing grace periods for `SV4` and `SV5` on the EDU-mod fixture to prevent `ERR` fault LED activation (overflow in reservoir or mixer).
  - [ ] Review code readability, function header comments, and documentation according to assignment requirements.
