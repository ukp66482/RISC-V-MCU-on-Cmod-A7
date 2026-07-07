/* adc.h - potentiometer (DIP 15) and die temperature via the ADC */
#ifndef ADC_H
#define ADC_H

#include "xil_types.h"

int adc_init(void);      /* 0 = ok */
u32 adc_pot_mv(void);    /* voltage on DIP 15 in mV (0..3300) */
int adc_die_mc(void);    /* die temperature in milli-degrees C */

#endif
