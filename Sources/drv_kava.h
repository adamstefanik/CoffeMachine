/*
 * @file     drv_kava.h
 * @brief    Ovladac pro model misici jednotky EDU-mod (Automat na kavu) na FRDM-KL25Z
 */

#ifndef DRV_KAVA_H_
#define DRV_KAVA_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Identifikatory ventilu (SV1 az SV5)
 * ============================================================================ */
#define VENTIL_KAVA		1	/* SV1 (PTE5) - napousteni TANK1 (kavovy koncentrat) */
#define VENTIL_VODA		2	/* SV2 (PTC1) - napousteni TANK2 (horka voda) */
#define VENTIL_CUKR		3	/* SV3 (PTE4) - napousteni TANK3 (cukr) */
#define VENTIL_MIX		4	/* SV4 (PTE0) - vypousteni TANK1-3 do misici nadoby MIX */
#define VENTIL_VYPUST	5	/* SV5 (PTE1) - vypousteni misici nadoby MIX do kelimku */

/* Aliasy podle oznaceni na panelu EDU-mod */
#define VENTIL_SV1		VENTIL_KAVA
#define VENTIL_SV2		VENTIL_VODA
#define VENTIL_SV3		VENTIL_CUKR
#define VENTIL_SV4		VENTIL_MIX
#define VENTIL_SV5		VENTIL_VYPUST

/* ============================================================================
 * Identifikatory hladinomeru (H1 az H8)
 * ============================================================================ */
/* TANK1 - Kava */
#define HLADINA_H1		1	/* PTD3  - TANK1 horni snimac (silna kava / plna nadrz) */
#define HLADINA_H2		2	/* PTC16 - TANK1 stredni snimac (stredni kava / polovina) */
#define HLADINA_H3		3	/* PTD2  - TANK1 dolni snimac (slaba kava / minimum) */

/* TANK2 - Voda */
#define HLADINA_H4		4	/* PTC5  - TANK2 horni snimac (velka voda / plna nadrz) */
#define HLADINA_H5		5	/* PTD4  - TANK2 dolni snimac (mala voda / minimum) */

/* TANK3 - Cukr */
#define HLADINA_H6		6	/* PTC7  - TANK3 horni snimac (hodne cukru / plna nadrz) */
#define HLADINA_H7		7	/* PTD5  - TANK3 stredni snimac (stredne cukru / polovina) */
#define HLADINA_H8		8	/* PTC6  - TANK3 dolni snimac (malo cukru / minimum) */

/* ============================================================================
 * Verejne funkce ovladace
 * ============================================================================ */

/**
 * @brief Inicializuje vsechny GPIO piny modelu misici jednotky (vystupy SV1-SV5, MIX a vstupy H1-H8).
 *        Vsechny ventily a michadlo jsou po inicializaci bezpecne vypnuty (log. 0).
 */
void Kava_Init(void);

/**
 * @brief Otevre nebo zavre zvoleny ventil SV1 az SV5.
 * @param ventil  Cislo ventilu (VENTIL_KAVA / VENTIL_SV1 az VENTIL_VYPUST / VENTIL_SV5, tj. 1-5).
 * @param otevrit true = otevrit ventil (log. 1), false = zavrit ventil (log. 0).
 */
void Kava_NastavVentil(uint8_t ventil, bool otevrit);

/**
 * @brief Zapne nebo vypne michadlo v misici nadrzi MIX (pin PTD0).
 * @param zapnout true = zapnout michadlo (log. 1), false = vypnout michadlo (log. 0).
 */
void Kava_NastavMichadlo(bool zapnout);

/**
 * @brief Zjisti, zda hladina v nadrzi dosahuje po dany snimac H1 az H8.
 * @param hladinomer Cislo hladinomeru (HLADINA_H1 az HLADINA_H8, tj. 1-8).
 * @return true pokud hladina dosahuje snimace (na pinu je log. 1), false pokud ne (log. 0).
 */
bool Kava_CtiHladinu(uint8_t hladinomer);

/**
 * @brief Okamzite uzavre vsechny ventily (SV1-SV5) a vypne michadlo (MIX).
 */
void Kava_VypniVse(void);

/**
 * @brief Overi, zda vsechny spodni hladinomery hornich nadrzi (H3, H5, H8) hlasi log. 0.
 * @return true pokud jsou H3 == 0, H5 == 0 a H8 == 0; jinak false.
 */
bool Kava_JsouHorniNadrzePodMinimem(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_KAVA_H_ */
