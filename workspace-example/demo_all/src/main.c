/******************************************************************************
 * demo_all — full-system breadboard demo for the RISC-V MCU on Cmod A7
 *
 * One program that exercises EVERY peripheral, stage by stage, narrating on
 * the USB serial port (115200 8N1). Runs forever; press the on-board button
 * (BTN0) to skip to the next stage.
 *
 * ── Breadboard wiring (all optional — unwired stages just report "skip") ──
 *
 *   GPIO LEDs   : DIP 1–7 (group A) → LED + 330 Ω → GND   (walking pattern)
 *   PWM LED     : DIP 10 (PWM_0)    → LED + 330 Ω → GND   (breathing)
 *   ADC pot     : DIP 15 (ADC_0)    → potentiometer wiper (ends: 3.3 V / GND)
 *   I2C sensor  : DIP 13 SCL / 14 SDA + 4.7 kΩ pull-ups → any I2C module
 *   SPI loopback: jumper DIP 36 (MOSI) ↔ DIP 37 (MISO)
 *   UART loopback: jumper DIP 11 (TX) ↔ DIP 12 (RX)
 *
 *   Board-only stages (no wiring needed): system info, on-board LED/RGB,
 *   FPGA die temperature, I2C bus scan, memory benchmark, timer interrupt.
 *
 * Every register address comes from xparameters.h; the class-based memory
 * map (0x40[C]x_xxxx: C = 0 GPIO · 1 Timer · 2 PWM · 3 UART · 4 INTC ·
 * 5 QSPI · 6 XADC · 7 I2C · 8 SPI) makes each base address self-describing.
 *****************************************************************************/
#include <stdint.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xuartns550_l.h"
#include "xsysmon.h"
#include "xiic_l.h"
#include "xspi.h"
#include "xintc.h"
#include "xil_exception.h"
#include "sleep.h"

/* ---- peripheral bases (all from xparameters.h) ---- */
#define LED_BASE     XPAR_BOARD_LED_2BITS_BASEADDR   /* 0x4000_0000 */
#define BTN_BASE     XPAR_BOARD_BUTTON_BASEADDR      /* 0x4001_0000 */
#define RGB_BASE     XPAR_BOARD_RGB_BASEADDR         /* 0x4002_0000 */
#define GPIOA_BASE   XPAR_GPIO_A_0_6_BASEADDR        /* 0x4003_0000 */
#define GPIOB_BASE   XPAR_GPIO_B_0_6_BASEADDR
#define GPIOC_BASE   XPAR_GPIO_C_0_6_BASEADDR
#define GPIOD_BASE   XPAR_GPIO_D_0_6_BASEADDR
#define TIMER0_BASE  XPAR_TIMER_0_BASEADDR           /* 0x4010_0000 */
#define TIMER2_BASE  XPAR_TIMER_2_BASEADDR           /* 0x4012_0000 */
#define PWM0_BASE    XPAR_PWM_0_BASEADDR             /* 0x4020_0000 */
#define PWM1_BASE    XPAR_PWM_1_BASEADDR
#define PWM2_BASE    XPAR_PWM_2_BASEADDR
#define UART1_BASE   XPAR_UART_1_BASEADDR            /* 0x4031_0000 (DIP) */
#define I2C_BASE     XPAR_I2C_0_BASEADDR             /* 0x4070_0000 */
#define SPI_BASE     XPAR_SPI_0_BASEADDR             /* 0x4080_0000 */

#define SRAM_BASE    0x60000000U

/* AXI GPIO register offsets */
#define GPIO_DATA    0x0
#define GPIO_TRI     0x4

/* AXI timer register offsets (PG079) */
#define TCSR0        0x00
#define TLR0         0x04
#define TCR0         0x08
#define TCSR_ENT     (1u << 7)   /* enable            */
#define TCSR_ARHT    (1u << 4)   /* auto reload       */
#define TCSR_LOAD    (1u << 5)   /* load TLR into TCR */
#define TCSR_UDT     (1u << 1)   /* down counter      */
#define TCSR_ENIT    (1u << 6)   /* interrupt enable  */
#define TCSR_T0INT   (1u << 8)   /* interrupt flag    */
#define TCSR_PWM     (1u << 9)   /* PWM mode          */
#define TCSR_GENT    (1u << 2)   /* generate out      */

/* ------------------------------------------------------------------ */
/* small helpers                                                      */
/* ------------------------------------------------------------------ */

static void led(u32 v)  { Xil_Out32(LED_BASE, v & 3); }
static int  btn(void)   { return Xil_In32(BTN_BASE) & 1; }

/* wait ms, return 1 early if the button is pressed (= skip request) */
static int wait_btn(u32 ms)
{
    for (u32 i = 0; i < ms; i++) {
        if (btn()) {
            while (btn()) usleep(1000);   /* wait release + debounce */
            usleep(20000);
            return 1;
        }
        usleep(1000);
    }
    return 0;
}

static void stage_banner(int n, const char *name)
{
    xil_printf("\r\n==== Stage %d: %s ====\r\n", n, name);
}

/* ------------------------------------------------------------------ */
/* Stage 0: system info                                               */
/* ------------------------------------------------------------------ */
static void stage_sysinfo(void)
{
    xil_printf("\r\n");
    xil_printf("*************************************************\r\n");
    xil_printf("*  RISC-V MCU on Cmod A7 — full-system demo     *\r\n");
    xil_printf("*************************************************\r\n");
    xil_printf("CPU    : MicroBlaze-V RV32IMB @ %d MHz\r\n",
               XPAR_CPU_CORE_CLOCK_FREQ_HZ / 1000000);
    xil_printf("Cache  : %dK I$ + %dK D$ (write-through, 32B lines)\r\n",
               XPAR_MICROBLAZE_RISCV_ICACHE_BYTE_SIZE / 1024,
               XPAR_MICROBLAZE_RISCV_DCACHE_BYTE_SIZE / 1024);
    xil_printf("Memory : BRAM 128K @0x0 | SRAM 512K @0x60000000 (cached)\r\n");
    xil_printf("Classes: 0x40[C]x_xxxx  C=0 GPIO 1 TMR 2 PWM 3 UART\r\n");
    xil_printf("         4 INTC 5 QSPI 6 XADC 7 I2C 8 SPI\r\n");
    xil_printf("Button : press BTN0 anytime to skip a stage\r\n");
}

/* ------------------------------------------------------------------ */
/* Stage 1: on-board LEDs + RGB                                       */
/* ------------------------------------------------------------------ */
static void stage_board_leds(void)
{
    stage_banner(1, "on-board LEDs + RGB (no wiring)");
    Xil_Out32(LED_BASE + GPIO_TRI, 0);
    Xil_Out32(RGB_BASE + GPIO_TRI, 0);
    for (int round = 0; round < 3; round++) {
        static const u32 rgb[] = {1, 2, 4, 3, 6, 5, 7, 0};
        for (int i = 0; i < 8; i++) {
            led(i & 3);
            Xil_Out32(RGB_BASE, rgb[i]);
            if (wait_btn(150)) goto out;
        }
    }
out:
    Xil_Out32(RGB_BASE, 0);
    led(0);
}

/* ------------------------------------------------------------------ */
/* Stage 2: GPIO groups A–D walking pattern                           */
/* ------------------------------------------------------------------ */
static void stage_gpio(void)
{
    stage_banner(2, "GPIO A-D walking 1 (wire LEDs to DIP 1-7)");
    static const u32 base[] = {GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE};
    for (int g = 0; g < 4; g++) Xil_Out32(base[g] + GPIO_TRI, 0);
    for (int round = 0; round < 3; round++)
        for (int b = 0; b < 7; b++) {
            for (int g = 0; g < 4; g++) Xil_Out32(base[g] + GPIO_DATA, 1u << b);
            if (wait_btn(120)) goto out;
        }
out:
    for (int g = 0; g < 4; g++) Xil_Out32(base[g] + GPIO_DATA, 0);
}

/* ------------------------------------------------------------------ */
/* Stage 3: PWM breathing LED                                         */
/* ------------------------------------------------------------------ */
static void pwm_set(u32 tbase, u32 period, u32 high)
{
    /* PWM mode: TLR0 = period, TLR1 = high time, both timers PWM+GENT */
    Xil_Out32(tbase + TLR0, period);
    Xil_Out32(tbase + TCSR0, TCSR_LOAD);
    Xil_Out32(tbase + 0x10 + TLR0, high);            /* timer 1 regs at +0x10 */
    Xil_Out32(tbase + 0x10 + TCSR0, TCSR_LOAD);
    u32 cfg = TCSR_PWM | TCSR_GENT | TCSR_ARHT | TCSR_UDT | TCSR_ENT;
    Xil_Out32(tbase + TCSR0, cfg);
    Xil_Out32(tbase + 0x10 + TCSR0, cfg);
}

static void stage_pwm(void)
{
    stage_banner(3, "PWM breathing (LED on DIP 10/34/40)");
    const u32 period = 100000;                        /* 1 kHz @ 100 MHz */
    for (int round = 0; round < 2; round++) {
        for (int pct = 2; pct <= 100; pct += 2) {
            u32 high = period / 100 * pct;
            pwm_set(PWM0_BASE, period, high);
            pwm_set(PWM1_BASE, period, period - high);
            pwm_set(PWM2_BASE, period, high);
            if (wait_btn(20)) goto out;
        }
        for (int pct = 100; pct >= 2; pct -= 2) {
            pwm_set(PWM0_BASE, period, period / 100 * pct);
            if (wait_btn(20)) goto out;
        }
    }
out:
    Xil_Out32(PWM0_BASE + TCSR0, 0); Xil_Out32(PWM0_BASE + 0x10 + TCSR0, 0);
    Xil_Out32(PWM1_BASE + TCSR0, 0); Xil_Out32(PWM1_BASE + 0x10 + TCSR0, 0);
    Xil_Out32(PWM2_BASE + TCSR0, 0); Xil_Out32(PWM2_BASE + 0x10 + TCSR0, 0);
}

/* ------------------------------------------------------------------ */
/* Stage 4: XADC — die temperature + 2 external channels              */
/* ------------------------------------------------------------------ */
static XSysMon sysmon;

static void stage_adc(void)
{
    stage_banner(4, "XADC (pot on DIP 15/16; temperature needs nothing)");
    XSysMon_Config *cfg = XSysMon_LookupConfig(XPAR_XADC_WIZ_0_BASEADDR);
    if (!cfg || XSysMon_CfgInitialize(&sysmon, cfg, cfg->BaseAddress) != XST_SUCCESS) {
        xil_printf("  sysmon init failed — skip\r\n");
        return;
    }
    for (int i = 0; i < 8; i++) {
        u16 t  = XSysMon_GetAdcData(&sysmon, XSM_CH_TEMP);
        u16 a4 = XSysMon_GetAdcData(&sysmon, XSM_CH_AUX_MIN + 4);
        u16 a12= XSysMon_GetAdcData(&sysmon, XSM_CH_AUX_MIN + 12);
        /* temp(C) = code*503.975/65536 - 273.15 ; pin V = code/65536 * 3.32V
         * (64-bit intermediate: code*503975 overflows 32 bits — real bug we hit) */
        int tc  = (int)((u64)t * 503975 / 65536) - 273150;   /* milli-degC */
        int mv4 = (int)((u32)a4  * 3320 / 65536);            /* mV at pin  */
        int mv12= (int)((u32)a12 * 3320 / 65536);
        xil_printf("  die %d.%03d C | ADC_0 %d mV | ADC_1 %d mV\r\n",
                   tc / 1000, (tc % 1000 + 1000) % 1000, mv4, mv12);
        if (wait_btn(500)) return;
    }
}

/* ------------------------------------------------------------------ */
/* Stage 5: I2C bus scan                                              */
/* ------------------------------------------------------------------ */
static void stage_i2c(void)
{
    stage_banner(5, "I2C bus scan (sensor on DIP 13/14 + 4.7k pull-ups)");
    int found = 0;
    for (u8 addr = 0x08; addr <= 0x77; addr++) {
        u8 dummy;
        if (XIic_Recv(I2C_BASE, addr, &dummy, 1, XIIC_STOP) == 1) {
            xil_printf("  device ACK at 0x%02X\r\n", addr);
            found++;
        }
    }
    if (!found)
        xil_printf("  no devices found (bus idle — wire a sensor to try)\r\n");
    else
        xil_printf("  %d device(s) on the bus\r\n", found);
}

/* ------------------------------------------------------------------ */
/* Stage 6: SPI loopback                                              */
/* ------------------------------------------------------------------ */
static XSpi spi;

static void stage_spi(void)
{
    stage_banner(6, "SPI loopback (jumper DIP 36 MOSI <-> DIP 37 MISO)");
    static int spi_ready;                            /* init once, reuse */
    if (!spi_ready) {
        XSpi_Config *cfg = XSpi_LookupConfig(SPI_BASE);
        if (!cfg || XSpi_CfgInitialize(&spi, cfg, cfg->BaseAddress) != XST_SUCCESS) {
            xil_printf("  spi init failed — skip\r\n");
            return;
        }
        XSpi_SetOptions(&spi, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
        XSpi_Start(&spi);
        XSpi_IntrGlobalDisable(&spi);
        spi_ready = 1;
    }

    u8 tx[8] = {0xA5, 0x5A, 0x0F, 0xF0, 0x12, 0x34, 0x56, 0x78}, rx[8] = {0};
    XSpi_SetSlaveSelect(&spi, 1);                    /* pulse SS0 */
    XSpi_Transfer(&spi, tx, rx, 8);
    XSpi_SetSlaveSelect(&spi, 0);

    int ok = 1;
    for (int i = 0; i < 8; i++) if (rx[i] != tx[i]) ok = 0;
    if (ok)  xil_printf("  loopback PASS (8/8 bytes, SCK 6.25 MHz)\r\n");
    else     xil_printf("  no loopback — jumper DIP36<->37 to see it PASS\r\n");
}

/* ------------------------------------------------------------------ */
/* Stage 7: external UART loopback                                    */
/* ------------------------------------------------------------------ */
static void stage_uart_ext(void)
{
    stage_banner(7, "external UART loopback (jumper DIP 11 <-> DIP 12)");
    XUartNs550_SetBaud(UART1_BASE, XPAR_XUARTNS550_0_CLOCK_FREQ, 115200);
    XUartNs550_SetLineControlReg(UART1_BASE, XUN_LCR_8_DATA_BITS);
    int ok = 0;
    for (u8 c = 'A'; c <= 'D'; c++) {
        XUartNs550_SendByte(UART1_BASE, c);
        for (int t = 0; t < 1000; t++) {             /* ~10 ms timeout */
            if (XUartNs550_IsReceiveData(UART1_BASE)) {
                if (XUartNs550_RecvByte(UART1_BASE) == c) ok++;
                break;
            }
            usleep(10);
        }
    }
    if (ok == 4) xil_printf("  loopback PASS (4/4 bytes @115200)\r\n");
    else         xil_printf("  no loopback (%d/4) — jumper DIP11<->12 to test\r\n", ok);
}

/* ------------------------------------------------------------------ */
/* Stage 8: memory hierarchy benchmark                                */
/* ------------------------------------------------------------------ */
static u32 bench(volatile u32 *buf, int write_pass)
{
    Xil_Out32(TIMER2_BASE + TLR0, 0);
    Xil_Out32(TIMER2_BASE + TCSR0, TCSR_LOAD);
    Xil_Out32(TIMER2_BASE + TCSR0, TCSR_ENT);        /* free-run up */
    u32 acc = 0;
    if (write_pass) for (int i = 0; i < 4096; i++) buf[i] = i;
    else            for (int i = 0; i < 4096; i++) acc += buf[i];
    u32 cyc = Xil_In32(TIMER2_BASE + TCR0);
    Xil_Out32(TIMER2_BASE + TCSR0, 0);
    (void)acc;
    return cyc;
}

static volatile u32 bram_buf[4096];                  /* lands in BRAM .bss? no — SRAM .bss */

static void stage_memory(void)
{
    stage_banner(8, "memory hierarchy: 16KB passes (cycles @100MHz)");
    volatile u32 *sram = (volatile u32 *)(SRAM_BASE + 0x40000); /* untouched SRAM */
    xil_printf("  SRAM  write        : %u cyc  (write-through: every store pays)\r\n",
               bench(sram, 1));
    xil_printf("  SRAM  read (cold)  : %u cyc  (line fills, 8 words/miss)\r\n",
               bench(sram, 0));
    xil_printf("  SRAM  read (warm)  : %u cyc  (D$ hits)\r\n", bench(sram, 0));
    xil_printf("  .bss  read (cached): %u cyc\r\n", bench(bram_buf, 0));
    xil_printf("  note: stack lives in BRAM (1-cycle, uncached by design)\r\n");
}

/* ------------------------------------------------------------------ */
/* Stage 9: timer interrupt via INTC                                  */
/* ------------------------------------------------------------------ */
static XIntc intc;
static volatile u32 ticks;

static void timer_isr(void *ref)
{
    (void)ref;
    Xil_Out32(TIMER0_BASE + TCSR0,
              Xil_In32(TIMER0_BASE + TCSR0) | TCSR_T0INT);  /* clear flag */
    ticks++;
    led(ticks & 3);
}

static void stage_interrupt(void)
{
    stage_banner(9, "timer_0 interrupt via INTC (4 Hz, 3 seconds)");
    ticks = 0;
    static int intc_ready;                           /* init once, reuse */
    if (!intc_ready) {
        if (XIntc_Initialize(&intc, XPAR_XINTC_0_BASEADDR) != XST_SUCCESS) {
            xil_printf("  intc init failed — skip\r\n");
            return;
        }
        /* timer_0 -> xlconcat In0 -> INTC input 0 (see IP reference §1.4) */
        XIntc_Connect(&intc, 0, (XInterruptHandler)timer_isr, 0);
        XIntc_Start(&intc, XIN_REAL_MODE);
        Xil_ExceptionInit();
        Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                     (Xil_ExceptionHandler)XIntc_InterruptHandler,
                                     &intc);
        intc_ready = 1;
    }
    XIntc_Enable(&intc, 0);
    Xil_ExceptionEnable();

    Xil_Out32(TIMER0_BASE + TLR0, 25000000);         /* 0.25 s @ 100 MHz */
    Xil_Out32(TIMER0_BASE + TCSR0, TCSR_LOAD);
    Xil_Out32(TIMER0_BASE + TCSR0, TCSR_ENIT | TCSR_ARHT | TCSR_UDT | TCSR_ENT);

    usleep(3000000);

    Xil_Out32(TIMER0_BASE + TCSR0, 0);               /* stop timer      */
    Xil_ExceptionDisable();
    XIntc_Disable(&intc, 0);
    xil_printf("  ISR fired %u times (expected ~12) — INTC + mtvec path OK\r\n",
               ticks);
    led(0);
}

/* ------------------------------------------------------------------ */

int main(void)
{
    XUartNs550_SetBaud(XPAR_UART_USB_BASEADDR, XPAR_XUARTNS550_0_CLOCK_FREQ, 115200);
    XUartNs550_SetLineControlReg(XPAR_UART_USB_BASEADDR, XUN_LCR_8_DATA_BITS);
    Xil_Out32(BTN_BASE + GPIO_TRI, 1);               /* button = input  */

    stage_sysinfo();
    while (1) {
        stage_board_leds();
        stage_gpio();
        stage_pwm();
        stage_adc();
        stage_i2c();
        stage_spi();
        stage_uart_ext();
        stage_memory();
        stage_interrupt();
        xil_printf("\r\n---- demo loop complete, restarting ----\r\n");
    }
    return 0;
}
