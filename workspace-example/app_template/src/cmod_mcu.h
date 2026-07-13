/*
 * cmod_mcu.h - device header for the RISC-V MCU on Cmod A7-35T
 *
 * Base addresses, register offsets, and bit masks for every peripheral,
 * mirroring chapter 5 of the datasheet (Register Access Conventions and
 * the per-peripheral register maps). All registers are 32-bit words and
 * must be accessed with full-word reads and writes.
 *
 * The same file serves C and assembly:
 *   C:        #include "cmod_mcu.h"      REG32(RGB_BASE) = RGB_GREEN;
 *   assembly: #include "cmod_mcu.h"      li t0, RGB_BASE; li t1, RGB_GREEN;
 *                                        sw t1, 0(t0)
 * (assembly sources are preprocessed, so #include and the #define names
 *  work in .S files; everything C-only is guarded by __ASSEMBLER__)
 */

#ifndef CMOD_MCU_H
#define CMOD_MCU_H

/* ------------------------------------------------------------------ */
/* System                                                              */
/* ------------------------------------------------------------------ */

#define CPU_HZ            100000000   /* system clock: 100 MHz, 10 ns/cycle */

/* Memory map (see datasheet chapter 3) */
#define BOOTROM_BASE      0x00000000  /* 32 KB boot ROM (do not use)   */
#define ITCM_BASE         0x00008000  /* 32 KB fast code (ITCM_FUNC)   */
#define ITCM_SIZE         0x00008000
#define DTCM_BASE         0x00010000  /* 64 KB fast data + stack       */
#define DTCM_SIZE         0x00010000
#define SRAM_BASE         0x60000000  /* 512 KB main memory (cached)   */
#define SRAM_SIZE         0x00080000

/* ------------------------------------------------------------------ */
/* GPIO ports (datasheet 5.2)                                          */
/* ------------------------------------------------------------------ */

#define LED_BASE          0x40000000  /* 2-bit,  on-board LEDs, output      */
#define BTN_BASE          0x40010000  /* 1-bit,  user button BTN1, input    */
#define RGB_BASE          0x40020000  /* 3-bit,  RGB LED, output            */
#define GPIO_A_BASE       0x40030000  /* 7-bit,  DIP pins 1-7,  bidir       */
#define GPIO_B_BASE       0x40040000  /* 7-bit,  DIP pins 17-23, bidir      */
#define GPIO_C_BASE       0x40050000  /* 7-bit,  DIP pins 42-48, bidir      */
#define GPIO_D_BASE       0x40060000  /* 7-bit,  DIP pins 26-32, bidir      */
#define EXTI_BASE         0x40070000  /* 4-bit,  INTR_0..3, input, IRQ 5    */

/* register offsets (all ports share one layout) */
#define GPIO_DATA         0x000       /* RW   pin data                      */
#define GPIO_TRI          0x004       /* RW   direction: 1=input, 0=output  */
#define GPIO_GIER         0x11C       /* RW   global IRQ enable (EXTI only) */
#define GPIO_ISR          0x120       /* R/TOW IRQ status       (EXTI only) */
#define GPIO_IER          0x128       /* RW   IRQ enable        (EXTI only) */

#define GPIO_GIER_ENABLE  0x80000000  /* GIER bit 31                        */
#define GPIO_IRQ_CH1      0x00000001  /* ISR/IER channel bit                */

/* RGB LED bit assignment (bit 0 = Blue, bit 1 = Green, bit 2 = Red) */
#define RGB_BLUE          0x1
#define RGB_GREEN         0x2
#define RGB_RED           0x4

/* ------------------------------------------------------------------ */
/* Timers and PWM (datasheet 5.3) - dual 32-bit counter blocks         */
/* ------------------------------------------------------------------ */

#define TIMER0_BASE       0x40100000  /* general purpose, IRQ 0             */
#define TIMER1_BASE       0x40110000  /* general purpose, IRQ 1             */
#define TIMER2_BASE       0x40120000  /* general purpose, IRQ 2             */
#define PWM0_BASE         0x40200000  /* PWM channel 0, DIP pin 10          */
#define PWM1_BASE         0x40210000  /* PWM channel 1, DIP pin 34          */
#define PWM2_BASE         0x40220000  /* PWM channel 2, DIP pin 40          */

/* register offsets: counter 0 at +0x00, counter 1 at +0x10 */
#define TMR_TCSR0         0x00        /* RW   counter 0 control/status      */
#define TMR_TLR0          0x04        /* RW   counter 0 load value          */
#define TMR_TCR0          0x08        /* RO   counter 0 current count       */
#define TMR_TCSR1         0x10        /* RW   counter 1 control/status      */
#define TMR_TLR1          0x14        /* RW   counter 1 load value          */
#define TMR_TCR1          0x18        /* RO   counter 1 current count       */

/* TCSR bits */
#define TMR_MDT           0x001       /* mode: 0=generate, 1=capture        */
#define TMR_UDT           0x002       /* direction: 0=up, 1=down            */
#define TMR_GENT          0x004       /* enable generate output (PWM)       */
#define TMR_CAPT          0x008       /* capture mode (not wired here)      */
#define TMR_ARHT          0x010       /* auto-reload (1) / stop-hold (0)    */
#define TMR_LOAD          0x020       /* while 1: TCR = TLR (pulse it)      */
#define TMR_ENIT          0x040       /* enable interrupt output            */
#define TMR_ENT           0x080       /* enable (run) the counter           */
#define TMR_TINT          0x100       /* IRQ status; write 1 back to clear  */
#define TMR_PWMA          0x200       /* PWM mode (set in both TCSRs)       */
#define TMR_ENALL         0x400       /* enable both counters at once       */
#define TMR_CASC          0x800       /* cascade into one 64-bit counter    */

/* PWM recipe: TLR0 = period, TLR1 = high time (cycles, +2 overhead),
 * LOAD-pulse both counters, then write TMR_PWM_RUN to both TCSRs.     */
#define TMR_PWM_RUN       (TMR_PWMA | TMR_ENT | TMR_ARHT | TMR_GENT | TMR_UDT)

/* ------------------------------------------------------------------ */
/* UART, industry-standard 16550 (datasheet 5.4)                       */
/* ------------------------------------------------------------------ */

#define UART_USB_BASE     0x40300000  /* USB console, IRQ 4                 */
#define UART1_BASE        0x40310000  /* DIP pins 11/12, IRQ 3              */

/* register offsets (16550 register set, one word apart)               */
#define UART_RBR          0x1000      /* RO/RSE receive buffer (read pops)  */
#define UART_THR          0x1000      /* WO   transmit holding              */
#define UART_DLL          0x1000      /* RW   divisor LSB   (when DLAB=1)   */
#define UART_IER          0x1004      /* RW   interrupt enable              */
#define UART_DLM          0x1004      /* RW   divisor MSB   (when DLAB=1)   */
#define UART_IIR          0x1008      /* RO   interrupt identification      */
#define UART_FCR          0x1008      /* WO   FIFO control                  */
#define UART_LCR          0x100C      /* RW   line control                  */
#define UART_MCR          0x1010      /* RW   modem control                 */
#define UART_LSR          0x1014      /* RO   line status                   */
#define UART_MSR          0x1018      /* RO   modem status                  */
#define UART_SCR          0x101C      /* RW   scratch                       */

#define UART_LCR_8N1      0x03        /* 8 data, no parity, 1 stop          */
#define UART_LCR_DLAB     0x80        /* divisor latch access               */
#define UART_LSR_RX_READY 0x01        /* received byte available            */
#define UART_LSR_THRE     0x20        /* transmit holding register empty    */
#define UART_LSR_TX_EMPTY 0x40        /* transmitter completely idle        */
#define UART_FCR_ENABLE   0x01        /* enable FIFOs                       */
#define UART_FCR_RX_RESET 0x02        /* clear receive FIFO                 */
#define UART_FCR_TX_RESET 0x04        /* clear transmit FIFO                */
#define UART_IER_RX       0x01        /* IRQ on received data               */
#define UART_IER_TX       0x02        /* IRQ on THR empty                   */

/* baud divisor = CPU_HZ / (16 * baud); 115200 baud -> 54 */
#define UART_DIV_115200   54

/* ------------------------------------------------------------------ */
/* Interrupt controller (datasheet 5.1.4 / 2.x)                        */
/* ------------------------------------------------------------------ */

#define INTC_BASE         0x40400000

#define INTC_ISR          0x00        /* RW   interrupt status (raw)        */
#define INTC_IPR          0x04        /* RO   pending = status AND enable   */
#define INTC_IER          0x08        /* RW   interrupt enable mask         */
#define INTC_IAR          0x0C        /* WO   acknowledge (write 1 bits)    */
#define INTC_SIE          0x10        /* WO   set enable bits               */
#define INTC_CIE          0x14        /* WO   clear enable bits             */
#define INTC_IVR          0x18        /* RO   lowest pending input number   */
#define INTC_MER          0x1C        /* RW   master enable                 */

#define INTC_MER_ME       0x1         /* master enable                      */
#define INTC_MER_HIE      0x2         /* hardware IRQ enable (write-once)   */

/* interrupt input numbers (bit n of ISR/IER/IAR = input n) */
#define IRQ_TIMER0        0
#define IRQ_TIMER1        1
#define IRQ_TIMER2        2
#define IRQ_UART1         3
#define IRQ_UART_USB      4
#define IRQ_EXTI          5
#define IRQ_I2C           6
#define IRQ_SPI           7
#define IRQ_BIT(n)        (1u << (n))

/* ------------------------------------------------------------------ */
/* SPI master spi_0 + clock control (datasheet 5.7)                    */
/* NOTE: use the register-level flow from showcase/src/spi0.c; the     */
/* generic driver transfer path is not reliable on this device.        */
/* ------------------------------------------------------------------ */

#define SPI0_BASE         0x40800000  /* external SPI master, IRQ 7         */

#define SPI_DGIER         0x1C        /* RW   global IRQ enable             */
#define SPI_ISR           0x20        /* R/TOW IRQ status                   */
#define SPI_IER           0x28        /* RW   IRQ enable                    */
#define SPI_SRR           0x40        /* WO   software reset (write 0xA)    */
#define SPI_CR            0x60        /* RW   control                       */
#define SPI_SR            0x64        /* RO   status                        */
#define SPI_DTR           0x68        /* WO   transmit data (FIFO)          */
#define SPI_DRR           0x6C        /* RO/RSE receive data (read pops)    */
#define SPI_SSR           0x70        /* RW   slave select (active-low bit) */
#define SPI_TFO           0x74        /* RO   TX FIFO occupancy             */
#define SPI_RFO           0x78        /* RO   RX FIFO occupancy             */

#define SPI_SRR_RESET     0x0000000A  /* reset key                          */

#define SPI_CR_LOOPBACK   0x001
#define SPI_CR_ENABLE     0x002
#define SPI_CR_MASTER     0x004
#define SPI_CR_CPOL       0x008
#define SPI_CR_CPHA       0x010
#define SPI_CR_TXFIFO_RST 0x020
#define SPI_CR_RXFIFO_RST 0x040
#define SPI_CR_MANUAL_SS  0x080
#define SPI_CR_INHIBIT    0x100

#define SPI_SR_RX_EMPTY   0x01        /* drain RX by this level, not counts */
#define SPI_SR_RX_FULL    0x02
#define SPI_SR_TX_EMPTY   0x04
#define SPI_SR_TX_FULL    0x08
#define SPI_SR_MODE_FAULT 0x10

/* board-verified CR values (see spi0.c): setup -> settle -> run */
#define SPI_CR_SETUP      0x1E6       /* enable+master+manual SS+FIFO reset */
#define SPI_CR_RUN        0x086       /* external transfer, SS by SPI_SSR   */
#define SPI_CR_IDLE       0x186       /* inhibited between transfers        */

/* SPI clock control unit: SCK = 200 MHz / N, N = 8..128 (default 32) */
#define SPI0_CLK_BASE     0x40900000
#define SPI0CLK_STATUS    0x004       /* RO   bit 0 = clock locked          */
#define SPI0CLK_CFG_MUL   0x200       /* RW   write 0x0801 (fixed setting)  */
#define SPI0CLK_CFG_DIV   0x208       /* RW   write N                       */
#define SPI0CLK_APPLY     0x25C       /* WO   write 3 to apply              */

/* ------------------------------------------------------------------ */
/* I2C master i2c_0 (datasheet 5.6) - course path is the driver;       */
/* the subset below covers reset and the bus-busy guard.               */
/* ------------------------------------------------------------------ */

#define I2C_BASE          0x40700000  /* DIP pins 13/14, 100 kHz, IRQ 6     */

#define I2C_GIE           0x01C       /* RW   global IRQ enable             */
#define I2C_ISR           0x020       /* R/TOW IRQ status                   */
#define I2C_IER           0x028       /* RW   IRQ enable                    */
#define I2C_SOFTR         0x040       /* WO   soft reset (write 0xA)        */
#define I2C_CR            0x100       /* RW   control                       */
#define I2C_SR            0x104       /* RO   status                        */
#define I2C_TX            0x108       /* WO   TX FIFO                       */
#define I2C_RX            0x10C       /* RO/RSE RX FIFO (read pops)         */
#define I2C_ADR           0x110       /* RW   slave address (as a slave)    */
#define I2C_RX_PIRQ       0x120       /* RW   RX FIFO threshold             */

#define I2C_SOFTR_KEY     0x0000000A

#define I2C_CR_EN         0x01        /* enable the controller              */
#define I2C_CR_TXFIFO_RST 0x02
#define I2C_CR_MSMS       0x04        /* master start/stop                  */
#define I2C_CR_TX         0x08        /* direction: 1 = transmit            */
#define I2C_CR_NACK       0x10
#define I2C_CR_RSTA       0x20        /* repeated start                     */

#define I2C_SR_BUS_BUSY   0x04        /* guard before starting a transfer   */
#define I2C_SR_RX_EMPTY   0x40
#define I2C_SR_TX_EMPTY   0x80

/* ------------------------------------------------------------------ */
/* ADC (datasheet 5.5) - course path is the driver; data registers     */
/* below allow direct reads. Samples are 12-bit, left-justified in     */
/* bits [15:4] of the 16-bit result.                                   */
/* ------------------------------------------------------------------ */

#define XADC_BASE         0x40600000

#define XADC_SRR          0x000       /* WO   software reset (write 0xA)    */
#define XADC_SR           0x004       /* RO   status                        */
#define XADC_TEMP         0x200       /* RO   die temperature               */
#define XADC_VCCINT       0x204       /* RO   core supply                   */
#define XADC_VCCAUX       0x208       /* RO   aux supply                    */
#define XADC_VAUX4        0x250       /* RO   analog input, DIP pin 15      */
#define XADC_VAUX12       0x270       /* RO   analog input, DIP pin 16      */
#define XADC_CFR0         0x300       /* RW   configuration 0               */
#define XADC_CFR1         0x304       /* RW   configuration 1               */
#define XADC_CFR2         0x308       /* RW   configuration 2               */

#define XADC_SRR_KEY      0x0000000A

/* ------------------------------------------------------------------ */
/* QSPI flash controller - managed by the boot firmware; applications  */
/* must not touch it (a stray write can corrupt the stored image).     */
/* ------------------------------------------------------------------ */

#define FLASH_CTRL_BASE   0x40500000

/* ------------------------------------------------------------------ */
/* C helpers                                                           */
/* ------------------------------------------------------------------ */

#ifndef __ASSEMBLER__

#define REG32(addr)       (*(volatile unsigned int *)(addr))

/* examples:
 *   REG32(RGB_BASE + GPIO_TRI) = 0;              -- all pins outputs
 *   REG32(RGB_BASE + GPIO_DATA) = RGB_GREEN;     -- green on
 *   while (!(REG32(UART_USB_BASE + UART_LSR) & UART_LSR_THRE)) ;
 *   REG32(UART_USB_BASE + UART_THR) = 'A';
 */

#endif /* __ASSEMBLER__ */

#endif /* CMOD_MCU_H */
