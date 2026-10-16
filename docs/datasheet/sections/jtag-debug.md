# JTAG Debug Mode — Vitis Unified IDE

This guide describes how to load and debug a MicroBlaze RISC-V application
over JTAG using the AMD Vitis Unified IDE. JTAG loads the program directly
into RAM and provides full debug control. Nothing is written to flash.

## Prerequisites

- The hardware design as an `.xsa` file: use the prebuilt
  [`release/top_wrapper.xsa`](../../../release/top_wrapper.xsa)
- Vitis Unified IDE 2025.2 installed
- Cmod A7-35T board connected via its micro-USB cable (the cable carries
  power, UART, and JTAG)
- A serial terminal program for the board's console output

---

## 1. Open the Workspace

Launch the Vitis Unified IDE and click **Set Workspace** on the Welcome page.

![Set Workspace](images/jtag0.png)

Select the `workspace-example/` folder in the repository. The template paths
in this chapter assume this location; any empty folder can also be used if the
paths are adjusted accordingly.

![Open Folder](images/jtag1.png)

The first time a Vitis installation opens this workspace it may show an
**Update Workspace** dialog. Click **Update**. This action refreshes only the
workspace metadata and does not modify any component.

![Update Workspace](images/jtag2.png)

---

## 2. Create a Platform

The platform wraps the hardware design and provides the driver layer (BSP).
It is created once per workspace and reused by every application. If the
workspace already contains a `platform` component, skip to section 3.

### 2.1 New Platform Component

From the menu bar, select **File > New Component > Platform**.

![File > New Component > Platform](images/jtag3.png)

### 2.2 Platform Name and Location

Enter `platform` as the component name and keep the workspace directory as
the location. Click **Next**.

![Platform Name and Location](images/jtag4.png)

### 2.3 Select the Hardware Design (XSA)

On the **Flow** page, select **Hardware Design** and click **Browse** next to
the *Hardware Design (XSA) For Implementation* field.

![Select Platform Creation Flow](images/jtag5.png)

Select the prebuilt `release/top_wrapper.xsa` from the repository.

![Select top_wrapper.xsa](images/jtag6.png)

### 2.4 Select Operating System and Processor

- **Operating system**: `standalone`
- **Processor**: `microblaze_riscv_0`

Click **Next**.

![OS and Processor](images/jtag7.png)

### 2.5 Platform Summary

Review the settings and click **Finish**.

![Platform Summary](images/jtag8.png)

Vitis generates the platform component. When it finishes, the Explorer shows
the `platform` component with its `Sources` and `Output` trees, and the
notification *"Created platform: platform"* appears.

![Platform Created](images/jtag9.png)

---

## 3. Create an Application

### 3.1 Create from Template

Open the **Examples** view from the left activity bar. Under *Embedded
Software Examples*, select **Hello World** and click the **+** (Create
Application Component from Template) button.

![Hello World Template](images/jtag10.png)

### 3.2 Application Name and Location

Enter your application name (this chapter uses `test`). Click **Next**.

![Application Name](images/jtag11.png)

### 3.3 Select the Platform

Select the `platform` component created in section 2 (Board: `cmod_a7-35t`).
Click **Next**.

![Select Platform](images/jtag12.png)

### 3.4 Select the Domain

Select `standalone_microblaze_riscv_0`. Click **Next**.

![Select Domain](images/jtag13.png)

### 3.5 Application Summary

Review the settings and click **Finish**.

![Application Summary](images/jtag14.png)

The new component appears in the Explorer. Its `src/` folder holds the
template's generated files: `helloworld.c`, `lscript.ld`, `platform.c`, and
`platform.h`.

![Application Created](images/jtag15.png)

---

## 4. Replace the Sources with the Course Template

The generated Hello World sources are replaced by the course template's
`main.c`, and the component is pointed at the workspace's `mcu/` folder,
which holds the two files every application builds against:

| File | Content |
|---|---|
| `mcu/cmod_mcu.h` | Device header: base address, register offsets, and bit masks of every peripheral (C and assembly), plus `REG32()`, the `ITCM_FUNC`/`DTCM_DATA` placement attributes, and `mcu_init()` for C |
| `mcu/cmod_mcu.ld` | Linker script: places code and data in ITCM, DTCM, and SRAM |

### 4.1 Delete the Generated Files

In the Explorer, select all four files under `src/` (`helloworld.c`,
`lscript.ld`, `platform.c`, `platform.h`), right-click, and choose
**Delete**.

![Delete Generated Sources](images/jtag16.png)

### 4.2 Import the Template File

Right-click the `src/` folder and choose **Import > Files...**

![Import Files](images/jtag17.png)

Navigate to `workspace-example/app_template/src/` and select `main.c`.
Click **Open**.

![Select Template Files](images/jtag18.png)

Afterwards `src/` contains only `main.c`. C files inside `src/` are compiled
automatically. Assembly files (`.S`) are not collected automatically: list
them under **Compiler Settings > Sources > Compile sources**
(`USER_COMPILE_SOURCES`) in the component's settings.

### 4.3 Point the Component at the MCU Folder

Open the component's **Settings > UserConfig.cmake** and set two values:

| Setting | Value |
|---|---|
| **Compiler Settings > Includes > Include paths (-I)** | `${CMAKE_SOURCE_DIR}/../../mcu` |
| **Linker Settings > Linker Script > Linker script file** | `${CMAKE_SOURCE_DIR}/../../mcu/cmod_mcu.ld` |

`${CMAKE_SOURCE_DIR}` is the component's `src/` folder, so both values refer
to `workspace-example/mcu/`. In the file `src/UserConfig.cmake` the two
settings read:

```cmake
set(USER_INCLUDE_DIRECTORIES
"${CMAKE_SOURCE_DIR}/../../mcu"
)
set(USER_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/../../mcu/cmod_mcu.ld")
```

> **Note:** Keep `mcu_init();` as the first line of `main()`. It switches
> interrupts off until the application enables them (a program loaded over
> a running one must not inherit its interrupts), and it sets the USB UART
> to 115200; the bootloader leaves it at 9600.

The demo loop cycles the on-board RGB LED and prints a status line about
once a second.

### 4.4 About the Linker Script

`cmod_mcu.ld` defines three memory regions: `ITCM` (32 KB), `DTCM` (64 KB),
and `SRAM` (512 KB). Everything goes to SRAM except `ITCM_FUNC` code and the
interrupt entry path (ITCM), and `DTCM_DATA` variables, the interrupt tables,
and the stack (DTCM). The loader writes ITCM and DTCM directly, so
`ITCM_FUNC` code and `DTCM_DATA` variables, initial values included, are in
place before `main()` runs.

The stack receives all DTCM not used by `DTCM_DATA`, growing down from
`0x0001_FFF0`; the heap receives all SRAM after the program. `_STACK_SIZE`
and `_HEAP_SIZE` at the top of `cmod_mcu.ld` are minimums: the link fails if
less remains. A different minimum for one application goes under **Linker
Settings > Miscellaneous > Linker flags**, for example
`-Wl,--defsym=_STACK_SIZE=0x4000`. The linker-script generator on the
settings page writes a new `lscript.ld`, which this project does not use.

---

## 5. Set the Console UART (BSP)

The BSP determines which UART carries `xil_printf` output. The default is
`uart_1` (the DIP-header pins); with the default setting, no output appears
on the USB console.

Open the platform's **Settings > vitis-comp.json**, navigate to
**standalone** (Board Support Package > microblaze_riscv_0 > standalone).
The configuration table shows `standalone_stdin` and `standalone_stdout` set
to `uart_1`:

![BSP Defaults to uart_1](images/jtag19.png)

Change both to `uart_USB`. The output log reports the domain being
reconfigured.

![stdin/stdout Set to uart_USB](images/jtag20.png)

> **Note:** If the platform BSP is ever regenerated (for example after
> changing another BSP option), re-check this page; regeneration can reset
> stdin/stdout to `uart_1`. After changing it, rebuild both the platform and
> the application; the setting is compiled into the application's BSP
> library.

---

## 6. Build

### 6.1 Build the Platform

In the **FLOW** panel, select the `platform` component and click **Build**.
Wait for *"Platform Build Finished successfully"*.

![Build Platform](images/jtag23.png)

### 6.2 Build the Application

Switch the FLOW component to your application and click **Build**.

![Build Application](images/jtag24.png)

The first application build asks how platform builds should be handled.
Select **Always build platform with application** and click **Save in
Workspace Preference**. BSP changes (such as section 5) then propagate
automatically on every build.

![Platform Build Dependency](images/jtag25.png)

The build produces `<workspace>/test/build/test.elf`.

> During the build, Vitis automatically links in the BSP's `boot.S` and the
> toolchain C library's startup object `crt0.o`: `boot.S` runs first after
> reset (initializes registers, stack pointer, and trap vector), and `crt0.o`
> clears `.bss` before calling `main()`.

---

## 7. Debug over JTAG

### 7.1 Start a Debug Session

In the **FLOW** panel, click **Debug**. Vitis programs the FPGA over JTAG,
loads the ELF into memory, and pauses execution at `main()`.

![Debug Launched](images/jtag26.png)

The Debug view shows the target (*RISC-V at USER2*, *Hart #0 — PAUSED ON
BREAKPOINT*), the call stack, local variables, and breakpoints. Note the call
stack address: `main()` is located at `0x600009EC`, in SRAM, exactly where
the linker script placed it.

![Paused at main()](images/jtag27.png)

### 7.2 Open a Serial Terminal, Then Continue

Open a serial terminal at 115200 before resuming execution.

![GTKTerm on the Board UART](images/jtag32.png)

Click **Continue (F5)**. The application starts: the memory tour prints once,
followed by a status line about once a second, and the on-board RGB LED
cycles through its colors.

![Memory Tour and Status Output](images/jtag33.png)

### 7.3 Breakpoints and Stepping

Click in the gutter next to a line number to set a breakpoint. The toolbar
provides **Continue**, **Step Over**, **Step Into**, and **Step Out**. The
left panel shows the call stack, variables, and watch expressions while the
target is paused.

To run the application without the debugger, use **Run** in the FLOW panel
instead. The loading sequence is the same, but execution does not pause at
`main()`.

---

## 8. Inspection Tools

While a debug session is active, the **View** menu provides three inspectors:

![View Menu](images/jtag28.png)

### 8.1 Memory Inspector

Enter an address to inspect memory directly, for example `0x60000000` (the
application image in SRAM), `0x8000` (ITCM), or `0x10000` (DTCM).

![Memory Inspector](images/jtag29.png)

### 8.2 Register Inspector

The Register Inspector shows all processor registers with live values. With
the target paused at `main()`, `sp` reads `0x0001FFF0`, just below the top of
the DTCM, matching the linker script's stack placement (the top 16 bytes hold
the startup code's argument slot).

![Register Inspector](images/jtag30.png)

### 8.3 Disassembly View

The Disassembly view shows the executing machine code with source
interleaving. The instruction addresses around `main()` all fall in
`0x6000_xxxx`; the program runs from SRAM.

![Disassembly](images/jtag31.png)

---

## 9. Troubleshooting

**Platform build fails with
`error: passing argument 2 of 'strcmp' ... [-Wint-conversion]` in
`xclk_wiz.c`** (reported as *"Error in generating platform"*). Two conditions
combine to cause this error:

- The clock-management driver source that ships with Vitis 2025.2
  (`clk_wiz` v1_10) contains a misplaced parenthesis in `xclk_wiz.c`
  line 839.
- If another RISC-V toolchain with GCC 14 or newer is on your PATH (for
  example one installed at `/opt/riscv` for assembly coursework), the BSP
  build may select it instead of the Vitis-bundled compiler. GCC 14+ treats
  this construct as an error; the bundled compiler treats it as a warning.

The offending line as displayed in the IDE:

![clk_wiz Driver Bug](images/jtag21.png)

Fix the source (this fix works with either compiler): change line 839 from

```c
strcmp(InstancePtr->Config.Name, "xlnx,clkx5-wiz-1.0" >= 0)
```

to

```c
strcmp(InstancePtr->Config.Name, "xlnx,clkx5-wiz-1.0") >= 0
```

![After the Fix](images/jtag22.png)

Then rebuild the platform. Apply the same one-line fix to the Vitis
installation copy at
`<install>/2025.2/data/embeddedsw/XilinxProcessorIPLib/drivers/clk_wiz_v1_10/src/xclk_wiz.c`,
where `<install>` is the Xilinx installation directory (`/opt/Xilinx` by
default on Linux). The platform re-imports driver sources whenever the BSP
is regenerated, so an unpatched installation reintroduces the error.

To check which compiler configured the BSP, read
`platform/.../bsp/libsrc/build_configs/gen_bsp/compile_commands.json`.
To force reselection, delete `platform/.../bsp/libsrc/build_configs` and
rebuild from a session whose PATH does not contain the other toolchain.

**The application runs (RGB LED cycles) but the USB console is silent.**
`xil_printf` output is directed to the DIP-header UART. The BSP stdout has
reverted to `uart_1`: redo section 5, then rebuild both the platform and
the application. The compiled setting is recorded in
`platform/export/platform/sw/standalone_microblaze_riscv_0/include/bspconfig.h`:
`STDOUT_BASEADDRESS` must read `0x40300000` (uart_USB), not `0x40310000`
(uart_1).
