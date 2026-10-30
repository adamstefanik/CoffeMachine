/*
 * @file     coffee_machine.h
 * @brief    Finite state machine, drink configurations, and context for Coffee Machine
 */

#ifndef COFFEE_MACHINE_H_
#define COFFEE_MACHINE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DRINK_NONE = 0,
    DRINK_ESPRESSO = 1,
    DRINK_LUNGO = 2,
    DRINK_DOPPIO = 3
} DrinkType_t;

typedef enum {
    COFFEE_LVL_MILD = 1,   /* Level 1: sensor H3 */
    COFFEE_LVL_MEDIUM = 2, /* Level 2: sensor H2 */
    COFFEE_LVL_STRONG = 3  /* Level 3: sensor H1 */
} CoffeeLevel_t;

typedef enum {
    SUGAR_LVL_NONE = 0,    /* Level 0: 0 s / no sugar */
    SUGAR_LVL_LOW = 1,     /* Level 1: sensor H8 */
    SUGAR_LVL_MEDIUM = 2,  /* Level 2: sensor H7 */
    SUGAR_LVL_HIGH = 3     /* Level 3: sensor H6 */
} SugarLevel_t;

typedef enum {
    /* Wizard screens */
    STATE_IDLE = 0,
    STATE_WIZARD_COFFEE,
    STATE_WIZARD_SUGAR,

    /* Active beverage preparation phases */
    STATE_FILL_COFFEE,
    STATE_FILL_WATER,
    STATE_FILL_SUGAR,
    STATE_DRAIN_TO_MIX,
    STATE_STIRRING,
    STATE_DISPENSING,
    STATE_DONE,

    /* Emergency cancellation and safe drain phases */
    STATE_STORNO_DRAIN_TANKS,
    STATE_STORNO_DRAIN_MIX
} CoffeeState_t;

typedef struct {
    DrinkType_t   drink;
    uint8_t       coffee_level;         /* 1..3 */
    uint8_t       sugar_level;          /* 0..3 */
    uint8_t       doppio_cycle;         /* 1 or 2 for Doppio */
    uint32_t      state_start_ms;       /* Millis when current state was entered */
    uint32_t      timer_duration_ms;    /* Target duration for timed operations */
    bool          drain_sensors_cleared;/* Indicates upper tanks just emptied below low sensor */
    uint32_t      drain_cleared_ms;     /* Millis timestamp when sensors emptied */
} CoffeeMachineContext_t;

/**
 * @brief Initializes coffee machine state machine and order context.
 */
void CoffeeMachine_Init(void);

/**
 * @brief Main non-blocking task executing coffee machine state machine.
 */
void Task_CoffeeMachine(void);

/**
 * @brief Returns the current state of the coffee machine.
 */
CoffeeState_t CoffeeMachine_GetState(void);

/**
 * @brief Returns read-only pointer to the current machine context.
 */
const CoffeeMachineContext_t* CoffeeMachine_GetContext(void);

/**
 * @brief Returns human-readable name of the specified drink.
 */
const char* CoffeeMachine_GetDrinkName(DrinkType_t drink);

/**
 * @brief Requests immediate program cancellation (STORNO).
 */
void CoffeeMachine_RequestStorno(void);

#ifdef __cplusplus
}
#endif

#endif /* COFFEE_MACHINE_H_ */
