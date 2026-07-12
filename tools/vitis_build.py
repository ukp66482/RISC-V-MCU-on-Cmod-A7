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
component in a non-empty directory.

Requires the platform to exist (workspace-example/platform, built from
release/top_wrapper.xsa — vitis_new_app.py creates it if missing).
Relaunches itself under `vitis -s`.
"""
import glob
import os
import shutil
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WS = os.path.join(REPO, "workspace-example")

try:
    import vitis  # only importable inside `vitis -s`
except ImportError:
    os.execvp("vitis", ["vitis", "-s", os.path.abspath(__file__)] + sys.argv[1:])


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if len(args) != 1:
        sys.exit("usage: python3 tools/vitis_build.py <app_name>")
    name = args[0]
    src = os.path.join(WS, name, "src")
    if not os.path.isdir(src):
        sys.exit(f"no sources at {src}")

    client = vitis.create_client(workspace=WS)
    xpfm = os.path.join(WS, "platform", "export", "platform", "platform.xpfm")
    if not os.path.exists(xpfm):
        sys.exit("platform missing - run tools/vitis_new_app.py once first")

    if not os.path.exists(os.path.join(WS, name, "vitis-comp.json")):
        # stash the WHOLE folder (src + README + companion dirs), let Vitis
        # create the component, then restore everything on top of it
        stash = os.path.join("/tmp", f"vitis_build_stash_{name}")
        if os.path.isdir(stash):
            shutil.rmtree(stash)
        shutil.move(os.path.join(WS, name), stash)
        app = client.create_app_component(
            name=name, platform=xpfm, domain="standalone_microblaze_riscv_0")
        for item in os.listdir(stash):
            s = os.path.join(stash, item)
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

    app.build()
    elf = os.path.join(WS, name, "build", name + ".elf")
    assert os.path.exists(elf), "build produced no ELF"
    rel = os.path.relpath(elf, REPO)
    print("\nBUILD OK:", rel)
    print("run:   python3 tools/jtag_run.py " + rel)
    print("flash: Vitis > Program Flash... (see the datasheet's Standalone Boot chapter)")


main()
