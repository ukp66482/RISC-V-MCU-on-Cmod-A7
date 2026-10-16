#!/usr/bin/env python3
"""
vitis_build.py — (re)build any checked-in app from its src/ folder.

    python3 tools/vitis_build.py showcase

Counterpart to vitis_new_app.py: that one creates a NEW app from the
template; this one builds an app whose sources already live in
workspace-example/<name>/src (e.g. the checked-in showcase).
If the folder is bare sources (no Vitis metadata), the component is
created around them first: the sources are stashed, Vitis creates the
component, and the sources are restored — Vitis refuses to create a
component in a non-empty directory. Every app builds against
workspace-example/mcu (cmod_mcu.h on the include path, cmod_mcu.ld as the
linker script), and assembly sources (.S) are listed in the component's
UserConfig.cmake, because the generated CMake collects only C/C++ files.

Requires the platform to exist (workspace-example/platform, built from
release/top_wrapper.xsa — vitis_new_app.py creates it if missing).
Relaunches itself under `vitis -s`.
"""
import os
import re
import shutil
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WS = os.path.join(REPO, "workspace-example")

try:
    import vitis  # only importable inside `vitis -s`
except ImportError:
    os.execvp("vitis", ["vitis", "-s", os.path.abspath(__file__)] + sys.argv[1:])


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


def list_asm_sources(src):
    """Add every .S/.s file in src/ to USER_COMPILE_SOURCES (idempotent)."""
    uc = os.path.join(src, "UserConfig.cmake")
    asm = sorted(f for f in os.listdir(src) if f.endswith((".S", ".s")))
    if not asm or not os.path.exists(uc):
        return
    text = open(uc).read()
    missing = [f for f in asm if f'"{f}"' not in text]
    key = "set(USER_COMPILE_SOURCES\n"
    if missing and key in text:
        text = text.replace(key, key + "".join(f'"{f}"\n' for f in missing), 1)
        with open(uc, "w") as f:
            f.write(text)
        print(">> listed assembly sources in UserConfig.cmake:", ", ".join(missing))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if len(args) != 1:
        sys.exit("usage: python3 tools/vitis_build.py <app_name>")
    name = args[0]
    src = os.path.join(WS, name, "src")
    if not os.path.isdir(src):
        sys.exit(f"no sources at {src}")

    client = open_workspace(WS)
    xpfm = os.path.join(WS, "platform", "export", "platform", "platform.xpfm")
    if not os.path.exists(xpfm):
        sys.exit("platform missing - run tools/vitis_new_app.py once first")

    if not os.path.exists(os.path.join(WS, name, "vitis-comp.json")):
        # stash the WHOLE folder (src + README + companion dirs), let Vitis
        # create the component, then restore everything on top of it —
        # also when Vitis fails, so the checked-in folder is never lost
        stash = tempfile.mkdtemp(prefix=f"vitis_build_{name}_")
        moved = os.path.join(stash, name)
        shutil.move(os.path.join(WS, name), moved)
        try:
            app = client.create_app_component(
                name=name, platform=xpfm, domain="standalone_microblaze_riscv_0")
        finally:
            os.makedirs(os.path.join(WS, name), exist_ok=True)
            for item in os.listdir(moved):
                s = os.path.join(moved, item)
                d = os.path.join(WS, name, item)
                if os.path.isdir(s):
                    shutil.copytree(s, d, dirs_exist_ok=True)
                else:
                    shutil.copy(s, d)
            shutil.rmtree(stash)
        print(f">> created component {name} around existing sources")
    else:
        app = client.get_component(name=name)
        print(f">> reusing component {name}")

    for stale in ("helloworld.c", "platform.c", "platform.h"):
        p = os.path.join(src, stale)
        if os.path.exists(p):
            os.remove(p)
    use_mcu_folder(src)
    list_asm_sources(src)

    app.build()
    elf = os.path.join(WS, name, "build", name + ".elf")
    assert os.path.exists(elf), "build produced no ELF"
    rel = os.path.relpath(elf, REPO)
    print("\nBUILD OK:", rel)
    print("run:   python3 tools/jtag_run.py " + rel)
    print("flash: Vitis > Program Flash... (see the datasheet's Standalone Boot chapter)")


main()
