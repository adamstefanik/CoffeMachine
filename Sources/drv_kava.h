#ifndef DRV_KAVA_H_
#define DRV_KAVA_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Oznaceni ventilu modelu misici jednotky (kavarny) */
#define VENTIL_KAVA		1	/* SV1 - TANK1 (kavovy koncentrat) */
#define VENTIL_VODA		2	/* SV2 - TANK2 (horka voda) */
#define VENTIL_CUKR		3	/* SV3 - TANK3 (cukr) */
#define VENTIL_MIX		4	/* SV4 - napousteni z nadrzi do mixeru */
#define VENTIL_VYPUST	5	/* SV5 - vypousteni z mixeru do kelimku */

/**
 * @brief Inicializace GPIO pinu pro model misici jednotky (automat na kavu).
 */
void Kava_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_KAVA_H_ */
