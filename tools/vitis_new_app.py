#!/usr/bin/env python3
"""
vitis_new_app.py — create and build a ready-to-run application in ONE command.

    python3 tools/vitis_new_app.py blinky

does everything the GUI walkthrough does:
  1. opens the workspace-example/ Vitis workspace,
  2. creates the platform from release/top_wrapper.xsa if it is missing
     (stdin/stdout preset to uart_USB, the way this project needs it),
  3. creates an application component <name> from the app_template
     main.c, set up to build against workspace-example/mcu (the device
     header cmod_mcu.h and the linker script cmod_mcu.ld),
  4. builds it and prints where the .elf is and how to run it.

The result is a normal Vitis project: you can keep working on it from the
Vitis GUI (Open Workspace -> workspace-example) with full Run/Debug support.

Requires Vitis on PATH (the script relaunches itself under `vitis -s`).
"""
import os
import re
import shutil
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

try:
    import vitis  # only importable inside `vitis -s`
except ImportError:
    os.execvp("vitis", ["vitis", "-s", os.path.abspath(__file__)] + sys.argv[1:])

CPU = "microblaze_riscv_0"
DOMAIN = "standalone_" + CPU


def open_workspace(ws):
    """Vitis refuses a non-empty folder without workspace metadata, which is
    exactly what a fresh clone has (_ide/ is not version-controlled). Seeding
    the one file it checks is enough; Vitis fills in the rest."""
    marker = os.path.join(ws, "_ide", "version.ini")
    if not os.path.exists(marker):
        os.makedirs(os.path.dirname(marker), exist_ok=True)
        with open(marker, "w") as f:
            f.write("com.ide.version=2025.2\n")
    return vitis.create_client(workspace=ws)


# workspace-example/mcu as seen from a component's src/ (= CMAKE_SOURCE_DIR)
MCU_DIR = "${CMAKE_SOURCE_DIR}/../../mcu"


def use_mcu_folder(src):
    """Build against workspace-example/mcu: put it on the include path (for
    cmod_mcu.h) and make its cmod_mcu.ld the linker script. The lscript.ld
    Vitis generates into src/ is deleted, so it cannot be mistaken for the
    script in use. Idempotent."""
    uc = os.path.join(src, "UserConfig.cmake")
    text = open(uc).read()
    key = "set(USER_INCLUDE_DIRECTORIES\n"
    if f'"{MCU_DIR}"' not in text:
        assert key in text, "UserConfig.cmake: no USER_INCLUDE_DIRECTORIES"
        text = text.replace(key, f'{key}"{MCU_DIR}"\n', 1)
    text, n = re.subn(r'set\(USER_LINKER_SCRIPT "[^"]*"\)',
                      f'set(USER_LINKER_SCRIPT "{MCU_DIR}/cmod_mcu.ld")', text)
    assert n == 1, "UserConfig.cmake: no USER_LINKER_SCRIPT"
    with open(uc, "w") as f:
        f.write(text)
    generated = os.path.join(src, "lscript.ld")
    if os.path.exists(generated):
        os.remove(generated)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if len(args) != 1 or not args[0].isidentifier():
        sys.exit("usage: python3 tools/vitis_new_app.py <app_name>\n"
                 "       (letters, digits and _ only, e.g. blinky)")
    name = args[0]
    ws = os.path.join(REPO, "workspace-example")
    xsa = os.path.join(REPO, "release", "top_wrapper.xsa")
    if not os.path.exists(xsa):
        sys.exit(f"missing {xsa} — build the hardware or restore release/")

    client = open_workspace(ws)

    # --- platform: reuse if present, otherwise create from the release XSA
    xpfm = os.path.join(ws, "platform", "export", "platform", "platform.xpfm")
    if not os.path.exists(xpfm):
        print(">> creating platform from release/top_wrapper.xsa "
              "(first time only, takes a few minutes)")
        plat = client.create_platform_component(
            name="platform", hw_design=xsa, os="standalone", cpu=CPU)
        dom = plat.get_domain(name=DOMAIN)
        dom.set_config(option="os", param="standalone_stdin", value="uart_USB")
        dom.set_config(option="os", param="standalone_stdout", value="uart_USB")
        plat.build()
        assert os.path.exists(xpfm), "platform build produced no .xpfm"
    else:
        print(">> reusing existing platform")

    # --- application component from the template
    try:
        app = client.create_app_component(name=name, platform=xpfm,
                                          domain=DOMAIN)
    except Exception:
        print(f">> component {name} already exists — rebuilding it")
        app = client.get_component(name=name)
    src = os.path.join(ws, name, "src")
    tpl = os.path.join(REPO, "workspace-example", "app_template", "src")
    shutil.copy(os.path.join(tpl, "main.c"), os.path.join(src, "main.c"))
    use_mcu_folder(src)
    # the template replaces the generated hello-world sources
    for stale in ("helloworld.c", "platform.c", "platform.h"):
        p = os.path.join(src, stale)
        if os.path.exists(p):
            os.remove(p)
    app.build()

    elf = os.path.join(ws, name, "build", name + ".elf")
    assert os.path.exists(elf), "build produced no ELF"
    rel = os.path.relpath(elf, REPO)
    print("\n" + "=" * 64)
    print(f"BUILD OK: {rel}")
    print("run it (JTAG, volatile):   python3 tools/jtag_run.py " + rel)
    print("ship it (flash, boots at power-on):")
    print("                           Vitis > Program Flash... (datasheet, Standalone Boot)")
    print("edit + debug in the GUI:   vitis -> Open Workspace -> workspace-example")
    print("=" * 64)


main()
