/* ===========================================================================
 * mem_bench — measures the memory-hierarchy latencies on the live board.
 *
 * This is the app behind the numbers in the Memory Specification
 * ("Measured Access Latencies"): TCM read, SRAM cache hit, SRAM cache miss,
 * and write-through store cost, all in cycles per 32-bit word.
 *
 * How it measures:
 *   - timer_2 counts the 100 MHz bus clock -> 1 tick == 1 CPU cycle.
 *   - every benchmark loop is compiled -O2 and noinline (the BENCH macro).
 *     The project default is -O0, whose bloated loops hide the memory
 *     signal — the -O0 replication at the end shows exactly that effect.
 *   - "cold" passes run right after a D-cache invalidate; "warm" passes
 *     re-read the same buffer. A 32 KB buffer exceeds the 16 KB D-cache,
 *     so its warm pass stays cold — capacity in action.
 *
 * Built on the course template: mcu_init() first, template lscript.ld.
 * =========================================================================*/
#include <string.h>
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xuartns550_l.h"
#include "xparameters.h"
#include "sleep.h"

#define ITCM_FUNC __attribute__((section(".itcm.text"), noinline))
#define DTCM_DATA __attribute__((section(".dtcm")))

static void mcu_init(void)
{
    extern char __itcm_lma[], __itcm_start[], __itcm_end[];
    extern char __dtcm_start[], __dtcm_end[];

    memcpy(__itcm_start, __itcm_lma, (size_t)(__itcm_end - __itcm_start));
    memset(__dtcm_start, 0, (size_t)(__dtcm_end - __dtcm_start));
    __asm__ volatile("fence.i");

    XUartNs550_SetBaud(XPAR_UART_USB_BASEADDR, 100000000, 115200);
}

/* ------------------------------------------------------------------ timer */
#define TIMER_BASE  XPAR_TIMER_2_BASEADDR   /* timer_2: free in course apps */
#define TCSR0       0x00
#define TLR0        0x04
#define TCR0        0x08
#define TCSR_ENT    (1u << 7)
#define TCSR_LOAD   (1u << 5)

static inline void t_start(void)
{
    Xil_Out32(TIMER_BASE + TCSR0, 0);
    Xil_Out32(TIMER_BASE + TLR0, 0);
    Xil_Out32(TIMER_BASE + TCSR0, TCSR_LOAD);
    Xil_Out32(TIMER_BASE + TCSR0, TCSR_ENT);
}
static inline u32 t_read(void) { return Xil_In32(TIMER_BASE + TCR0); }

/* -------------------------------------------------------------- the loops */
/* Per-function -O2: measure the memory, not the -O0 loop around it. */
#define BENCH __attribute__((optimize("O2"), noinline))

static volatile u32 sink;

static BENCH u32 rd_pass(volatile u32 *buf, u32 n)
{
    u32 acc = 0;
    t_start();
    for (u32 i = 0; i < n; i++) acc += buf[i];
    u32 c = t_read();
    sink = acc;
    return c;
}

/* identical loop at the project-default -O0, for the comparison demo */
static __attribute__((noinline)) u32 rd_pass_O0(volatile u32 *buf, u32 n)
{
    u32 acc = 0;
    t_start();
    for (u32 i = 0; i < n; i++) acc += buf[i];
    u32 c = t_read();
    sink = acc;
    return c;
}

static BENCH u32 wr_pass(volatile u32 *buf, u32 n)
{
    t_start();
    for (u32 i = 0; i < n; i++) buf[i] = i;
    return t_read();
}

static BENCH u32 hot_pass(volatile u32 *buf, u32 n)
{
    u32 acc = 0;
    t_start();
    for (u32 i = 0; i < n; i++) acc += buf[0];
    u32 c = t_read();
    sink = acc;
    return c;
}

static BENCH u32 alu_pass(u32 n)
{
    u32 a = 1, b = 2;
    t_start();
    for (u32 i = 0; i < n; i++) { a += b; b ^= a; a = (a << 1) | (b & 1); }
    u32 c = t_read();
    sink = a + b;
    return c;
}

/* ---------------------------------------------------------------- buffers */
DTCM_DATA static u32 tcm_buf[256];      /*  1 KB in DTCM (1-cycle)          */
static u32 sram_buf[16384];             /* 64 KB in SRAM .bss (cached range)*/

static void report(const char *name, u32 cyc, u32 n)
{
    u32 whole = cyc / n;
    u32 frac  = (cyc % n) * 100 / n;
    xil_printf("  %-26s %8u cyc   %2u.%u%u cyc/word\r\n",
               name, cyc, whole, frac / 10, frac % 10);
}

int main(void)
{
    mcu_init();                         /* <-- the one rule */

    xil_printf("\r\n==== mem_bench - memory hierarchy latencies ====\r\n");
    xil_printf("I$ %uK  D$ %uK  line %u words   (compare: Memory Spec, "
               "Measured Access Latencies)\r\n",
               XPAR_MICROBLAZE_RISCV_ICACHE_BYTE_SIZE / 1024,
               XPAR_MICROBLAZE_RISCV_DCACHE_BYTE_SIZE / 1024,
               XPAR_MICROBLAZE_RISCV_DCACHE_LINE_LEN);
    xil_printf("TCM buffer @ 0x%08x (DTCM)   SRAM buffer @ 0x%08x\r\n",
               (u32)(UINTPTR)tcm_buf, (u32)(UINTPTR)sram_buf);
    xil_printf("all loops -O2 unless marked; cyc/word includes ~7 cyc of "
               "loop overhead\r\n");

    for (u32 round = 1;; round++) {
        xil_printf("\r\n-- round %u --\r\n", round);

        t_start();
        xil_printf("  timer overhead: %u cyc\r\n", t_read());

        report("ALU only (loop floor)", alu_pass(4096), 4096);

        /* TCM: init once, second pass counted (steady state) */
        for (u32 i = 0; i < 256; i++) tcm_buf[i] = i;
        rd_pass(tcm_buf, 256);
        report("TCM read 1K", rd_pass(tcm_buf, 256), 256);

        /* pure hit discriminator: same word 4096 times */
        Xil_DCacheInvalidate();
        report("SRAM hot word x4096", hot_pass(sram_buf, 4096), 4096);

        /* cold vs warm across buffer sizes; 32K > D$ so warm stays cold */
        static const u32  nw[]  = { 1024, 4096, 8192 };   /* 4/16/32 KB */
        static const char *tag[] = { "4K", "16K", "32K" };
        for (int s = 0; s < 3; s++) {
            Xil_DCacheInvalidate();
            xil_printf("  [%s buffer]\r\n", tag[s]);
            report("    cold (miss)", rd_pass(sram_buf, nw[s]), nw[s]);
            report("    warm (hit)",  rd_pass(sram_buf, nw[s]), nw[s]);
        }

        report("SRAM write 4K (w-thru)", wr_pass(sram_buf, 1024), 1024);

        /* why the spec says "measure with optimization enabled": */
        Xil_DCacheInvalidate();
        xil_printf("  [same 16K read at -O0 - the loop hides the cache]\r\n");
        report("    -O0 cold", rd_pass_O0(sram_buf, 4096), 4096);
        report("    -O0 warm", rd_pass_O0(sram_buf, 4096), 4096);

        xil_printf("-- round %u done (again in 5 s) --\r\n", round);
        sleep(5);
    }
    return 0;
}
