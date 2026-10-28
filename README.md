# Coffee Machine Controller

An embedded firmware solution for an automated coffee machine running on the Freescale FRDM-KL25Z development board and the EDU-mod mixing unit. Features non-blocking cooperative multitasking, analog potentiometer configuration, an interactive LCD wizard, and fail-safe drainage mechanisms.

## Features

**Beverage Programs**
- Espresso — single-shot coffee extraction with customizable coffee strength
- Lungo — extended brew diluted with hot water from reservoir
- Doppio — double-cycle extraction sequence for rich coffee output

**Hardware Control**
- Direct register-level GPIO control for 5 solenoid valves (SV1–SV5) and stirrer motor
- Continuous level monitoring across 8 discrete sensors (H1–H8)
- Non-blocking cooperative multitasking driven by a 1 ms SysTick timer
- Software debouncing for push-button inputs with edge detection
- ADC potentiometer sampling with hardware averaging for smooth adjustments

**User Interface & Safety**
- 4x20 character LCD interface with dynamic graphical progress bars
- Interactive setup wizard for coffee strength and sugar level adjustment
- Real-time status screens indicating current preparation phases and timers
- Emergency cancel (STORNO) with automated, safe tank and mixer evacuation

## Hardware Architecture

- **MCU Board** — Freescale / NXP FRDM-KL25Z (ARM Cortex-M0+ @ 48 MHz)
- **Mixing Unit** — EDU-mod fixture with 3 supply tanks (Coffee, Water, Sugar) and 1 mixing chamber
- **Actuators** — 5 solenoid valves (SV1–SV5) and 1 DC mixer motor
- **Sensors** — 8 discrete liquid level sensors (H1–H8) distributed across reservoirs
- **Peripherals** — 4 push buttons (SW1–SW4), rotary potentiometer (ADC0), and 4x20 character LCD

## Tech Stack

- **C (C99)** bare-metal firmware
- **ARM Cortex-M0+ (MKL25Z128VLK4)**
- **Direct Register Access** (zero-overhead hardware control without heavy HALs)
- **Kinetis Design Studio (KDS)** & GNU ARM Toolchain
- **No external dependencies**

## Requirements

- Freescale / NXP FRDM-KL25Z development board
- EDU-mod Mísicí jednotka (Mixing Unit) fixture
- Kinetis Design Studio (KDS) v3.x or compatible GCC ARM Embedded toolchain
- OpenSDA debug driver (P&E Micro or CMSIS-DAP)

## How to Run

1. Clone the repository
2. Open Kinetis Design Studio and import the project into your workspace
3. Connect the FRDM-KL25Z board and EDU-mod fixture via USB OpenSDA
4. Build the project using the GCC ARM toolchain
5. Flash and run the firmware onto the target MCU

## Structure

```
CoffeMachine/
├── Includes/                           # CMSIS core and MKL25Z MCU register definitions
│   ├── MKL25Z4.h                       # Device-specific register mappings
│   └── core_cm0plus.h                  # Cortex-M0+ core definitions
├── Project_Settings/                   # Linker scripts, debugger config, startup code
│   ├── Debugger/                       # GDB and OpenSDA launch configurations
│   ├── Linker_Files/                   # Flash and RAM memory layout scripts
│   └── Startup_Code/                   # MCU startup and interrupt vector table
├── Sources/                            # Application and peripheral driver source files
│   ├── drv_adc.c / .h                  # Potentiometer ADC0 driver and value scaling
│   ├── drv_gpio.c / .h                 # GPIO configuration for kit LEDs and buttons
│   ├── drv_kava.c / .h                 # EDU-mod driver (valves SV1–SV5, mixer, sensors H1–H8)
│   ├── drv_lcd.c / .h                  # 4x20 character LCD driver and progress bar rendering
│   ├── drv_systick.c / .h              # 1 ms SysTick timer for non-blocking scheduling
│   ├── task_buttons.c / .h             # Non-blocking debounced button task (SW1–SW4) and STORNO
│   └── main.c                          # System initialization and cooperative run loop
├── zadani/                             # Project assignment documentation and schematics
├── plan.md                             # Issue tracking roadmap and task breakdown
└── README.md
```
