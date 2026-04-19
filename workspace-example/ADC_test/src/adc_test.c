/******************************************************************************
 * XADC 5-Channel Read Test
 *
 * Hardware:
 *   - XADC Wizard @ 0x44A30000, Continuous sequencer mode, 500 KSPS aggregate
 *
 * Enabled Channels:
 *   - VAUX4       : DIP Pin 15 (external 0–3.3 V, on-board divider to 0–1 V)
 *   - VAUX12      : DIP Pin 16 (external 0–3.3 V, on-board divider to 0–1 V)
 *   - Temperature : FPGA die temperature
 *   - VCCINT      : Core voltage (1.0 V)
 *   - VCCAUX      : Auxiliary voltage (1.8 V)
 *
 * Output:
 *   Prints all 5 channels once per loop iteration. External VAUX inputs are
 *   scaled back up by the divider ratio (≈ 1/0.301) to report pin voltage.
 *****************************************************************************/

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xsysmon.h"
#include "sleep.h"

#define SYSMON_BASEADDR     XPAR_XSYSMON_0_BASEADDR

/* Aux channel numbers: VAUXn = XSM_CH_AUX_MIN + n */
#define XSM_CH_VAUX4        (XSM_CH_AUX_MIN + 4)    /* 20 */
#define XSM_CH_VAUX12       (XSM_CH_AUX_MIN + 12)   /* 28 */

/* On-board divider on VAUX4/VAUX12: 2.32 kΩ / 1 kΩ, ratio ≈ 0.301 */
#define VAUX_DIVIDER_INV    3.322f

/* Read interval in microseconds (1,000,000 = 1 s) */
#define READ_INTERVAL_US    500000U   /* 500 ms */

static XSysMon sysmon;

static int sysmon_init(void)
{
    XSysMon_Config *cfg = XSysMon_LookupConfig(SYSMON_BASEADDR);
    if (cfg == NULL) {
        xil_printf("ERROR: XSysMon_LookupConfig failed\r\n");
        return XST_FAILURE;
    }

    int status = XSysMon_CfgInitialize(&sysmon, cfg, cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: XSysMon_CfgInitialize failed\r\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

/* Print an integer voltage in millivolts (avoids float printf in xil_printf) */
static void print_mv(const char *label, u32 mv)
{
    xil_printf("  %s = %u.%03u V\r\n", label, mv / 1000, mv % 1000);
}

int main(void)
{
    init_platform();
    xil_printf("\r\n=== XADC 5-Channel Read Test ===\r\n");

    if (sysmon_init() != XST_SUCCESS) {
        return -1;
    }

    xil_printf("XADC initialized. Reading every %u ms...\r\n\r\n",
               READ_INTERVAL_US / 1000);

    while (1) {
        u16 raw_temp   = XSysMon_GetAdcData(&sysmon, XSM_CH_TEMP);
        u16 raw_vccint = XSysMon_GetAdcData(&sysmon, XSM_CH_VCCINT);
        u16 raw_vccaux = XSysMon_GetAdcData(&sysmon, XSM_CH_VCCAUX);
        u16 raw_vaux4  = XSysMon_GetAdcData(&sysmon, XSM_CH_VAUX4);
        u16 raw_vaux12 = XSysMon_GetAdcData(&sysmon, XSM_CH_VAUX12);

        /* Temperature: T(°C) = raw * 503.975 / 65536 − 273.15
         * Scale to milli-°C using integer math: mC = raw*503975/65536 − 273150
         * u64 needed: 65535 * 503975 ≈ 3.3e10 overflows u32. */
        int temp_mc = (int)(((u64)raw_temp * 503975U) >> 16) - 273150;

        /* Supply voltage: V = raw / 65536 * 3.0  →  mV = raw * 3000 / 65536 */
        u32 vccint_mv = ((u32)raw_vccint * 3000U) >> 16;
        u32 vccaux_mv = ((u32)raw_vccaux * 3000U) >> 16;

        /* VAUX pin voltage: ADC sees 0–1 V, scaled back to pin (0–3.3 V)
         * pin_mV = raw / 65536 * 1000 mV * 3.322 = raw * 3322 / 65536 */
        u32 vaux4_mv  = ((u32)raw_vaux4  * 3322U) >> 16;
        u32 vaux12_mv = ((u32)raw_vaux12 * 3322U) >> 16;

        xil_printf("--- XADC sample ---\r\n");
        xil_printf("  Temp   = %d.%03d C (raw=0x%04X)\r\n",
                   temp_mc / 1000, (temp_mc < 0 ? -temp_mc : temp_mc) % 1000,
                   raw_temp);
        print_mv("VCCINT", vccint_mv);
        print_mv("VCCAUX", vccaux_mv);
        print_mv("VAUX4 (DIP15)", vaux4_mv);
        print_mv("VAUX12(DIP16)", vaux12_mv);
        xil_printf("\r\n");

        usleep(READ_INTERVAL_US);
    }

    cleanup_platform();
    return 0;
}
