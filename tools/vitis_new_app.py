#!/usr/bin/env python3
"""
vitis_new_app.py — create and build a ready-to-run application in ONE command.

    python3 tools/vitis_new_app.py blinky

does everything the GUI walkthrough does:
  1. opens the workspace-example/ Vitis workspace,
  2. creates the platform from release/top_wrapper.xsa if it is missing
     (stdin/stdout preset to uart_USB, the way this project needs it),
  3. creates an application component <name> from the app_template
     sources (main.c + lscript.ld),
  4. builds it and prints where the .elf is and how to run it.

The result is a normal Vitis project: you can keep working on it from the
Vitis GUI (Open Workspace -> workspace-example) with full Run/Debug support.

Requires Vitis on PATH (the script relaunches itself under `vitis -s`).
"""
import os
import shutil
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

try:
    import vitis  # only importable inside `vitis -s`
except ImportError:
    os.execvp("vitis", ["vitis", "-s", os.path.abspath(__file__)] + sys.argv[1:])

CPU = "microblaze_riscv_0"
DOMAIN = "standalone_" + CPU


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

    client = vitis.create_client(workspace=ws)

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
    for f in ("main.c", "lscript.ld"):
        shutil.copy(os.path.join(tpl, f), os.path.join(src, f))
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
    print("                           python3 tools/flash_app.py " + rel)
    print("edit + debug in the GUI:   vitis -> Open Workspace -> workspace-example")
    print("=" * 64)


main()
