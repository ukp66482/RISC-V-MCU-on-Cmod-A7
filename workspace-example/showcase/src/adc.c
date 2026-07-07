/* adc.c - 12-bit ADC (continuous sequencer), read as 16-bit left-aligned.
 *
 * DIP 15 (ADC_0) goes through the on-board 2.32k/1k divider (ratio 0.301),
 * so pin millivolts = code * 3322 / 65536.  Never put more than 3.3 V on
 * the pin.  The wiper of a 10k pot between 3.3 V and GND is the intended
 * source here.
 */
#include "adc.h"
#include "xparameters.h"
#include "xsysmon.h"

static XSysMon sysmon;
static int ready;

int adc_init(void)
{
    XSysMon_Config *cfg = XSysMon_LookupConfig(XPAR_XADC_WIZ_0_BASEADDR);
    if (!cfg || XSysMon_CfgInitialize(&sysmon, cfg, cfg->BaseAddress) != XST_SUCCESS)
        return -1;
    ready = 1;
    return 0;
}

u32 adc_pot_mv(void)
{
    if (!ready) return 0;
    u16 raw = XSysMon_GetAdcData(&sysmon, XSM_CH_AUX_MIN + 4);   /* DIP 15 */
    return ((u32)raw * 3322U) >> 16;
}

int adc_die_mc(void)
{
    if (!ready) return 0;
    u16 raw = XSysMon_GetAdcData(&sysmon, XSM_CH_TEMP);
    return (int)(((u64)raw * 503975U) >> 16) - 273150;
}
