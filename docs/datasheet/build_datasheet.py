#!/usr/bin/env python3
"""Build the unified Cmod A7 MCU datasheet (integrated edition).

The markdown files under docs/ stay the single source of truth. Instead of
concatenating whole files, this script parses each file into sections and
reassembles them into a real datasheet outline (RECIPE below): overview,
CPU/memory, pinout, peripherals, electrical/power, boot, development.
Sections that duplicate other sections are dropped explicitly (DROPPED, with
reasons); a coverage check fails the build if any source section is neither
used nor accounted for.

Requires the `typst` CLI on PATH (https://typst.app - single static binary),
or set the TYPST environment variable to the binary path.
"""
import os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.dirname(HERE)
OUT_PDF = os.path.join(HERE, "Cmod_A7_MCU_Datasheet.pdf")

SOURCES = {
    "ov":   os.path.join(HERE, "sections/overview.md"),
    "ip":   os.path.join(HERE, "sections/peripherals.md"),
    "pin":  os.path.join(HERE, "sections/pins.md"),
    "pwr":  os.path.join(HERE, "sections/power.md"),
    "mem":  os.path.join(HERE, "sections/memory.md"),
    "jtag": os.path.join(HERE, "sections/jtag-debug.md"),
    "boot": os.path.join(HERE, "sections/standalone-boot.md"),
}

# Datasheet outline. Entry types:
#   ("h1", title) / ("h2", title)          new structural heading
#   ("sec", alias, title, level, retitle)  source section incl. subsections,
#                                          re-rooted at the given level
#   ("body", alias, title)                 section body only (no heading)
#   ("intro", alias)                       file preamble (text before first ##)
#   ("text", markdown)                     glue text, converted like markdown
#   ("typst", code)                        raw typst (for glue with @refs)
RECIPE = [
    ("h1", "Device Overview"),
    ("sec", "ov", "Introduction", 2, None),
    ("sec", "ov", "Features", 2, None),
    ("sec", "ov", "System Block Diagram", 2, None),
    ("sec", "ov", "Reference Documents", 2, None),

    ("h1", "System Architecture"),
    ("sec", "ip", "MicroBlaze RISC-V (`microblaze_riscv_0`)", 2, "MicroBlaze RISC-V Core"),
    ("sec", "ip", "AXI SmartConnect (`microblaze_riscv_0_axi_periph`)", 2, "Bus Architecture"),
    ("sec", "ip", "AXI Interrupt Controller (`microblaze_riscv_0_axi_intc`)", 2, "Interrupt Controller"),
    ("sec", "ip", "Clocking Wizard (`clk_wiz_1`)", 2, "Clocking"),
    ("sec", "ip", "Processor System Reset (`rst_clk_wiz_1_100M`)", 2, "Reset"),
    ("sec", "ip", "Debug Module (`mdm_1`)", 2, "Debug Module"),
    ("sec", "ip", "Complete Address Map", 2, None),

    ("h1", "Memory Hierarchy"),
    ("sec", "mem", "Memory Hierarchy Overview", 2, None),
    ("sec", "mem", "Memory Map", 2, None),
    ("sec", "ip", "Local Memory", 2, "Local Memory (ITCM / DTCM / Bootloader)"),
    ("sec", "ip", "SRAM / Cellular RAM (`axi_emc_0`)", 2, "External SRAM (Cellular RAM)"),
    ("sec", "ip", "QSPI Flash (`axi_quad_spi_0`)", 2, "QSPI Flash"),
    ("sec", "mem", "Caches", 2, None),
    ("sec", "mem", "Measured Access Latencies", 2, None),
    ("sec", "mem", "Memory Placement Guidance", 2, None),

    ("h1", "Pinout"),
    ("sec", "pin", "DIP Connector Overview", 2, None),
    ("sec", "pin", "Pin Map — Left Side (Pin 1–24)", 2, None),
    ("sec", "pin", "Pin Map — Right Side (Pin 25–48)", 2, None),
    ("sec", "pin", "On-Board I/O (No DIP Pin Exposure)", 2, None),

    ("h1", "Peripherals"),
    ("typst", "All peripheral registers are memory-mapped and reached "
              "through the AXI data port; base addresses follow the "
              "class-based scheme summarised in @sec-map. Pin locations "
              "for every signal are listed in the pin maps of @ch-pinout. "
              "The QSPI flash controller is described with the memory "
              "system in @sec-flash."),
    ("sec", "ip", "GPIO (General Purpose I/O)", 2, "GPIO"),
    ("sec", "ip", "Timers & PWM", 2, "Timers and PWM"),
    ("sec", "ip", "UART Communication", 2, "UART"),
    ("sec", "ip", "I2C Controller (`i2c_0`)", 2, "I2C Controller"),
    ("sec", "ip", "External SPI Master (`spi_0`)", 2, "SPI Master"),
    ("sec", "ip", "XADC Wizard (`xadc_wiz_0`)", 2, "Analog-to-Digital Converter (XADC)"),
    ("sec", "pin", "Analog Input Circuit", 3, None),

    ("h1", "Electrical Characteristics and Power"),
    ("h2", "Electrical Characteristics"),
    ("body", "pin", "Electrical Characteristics"),
    ("body", "pwr", "Warnings"),
    ("sec", "pwr", "Overview", 2, "Power Architecture"),
    ("sec", "pwr", "Power Input Options", 2, None),
    ("sec", "pwr", "Output Power Rails", 2, None),
    ("sec", "pwr", "VU Pin Behavior", 2, None),
    ("sec", "pwr", "Dual Power Source (USB + External)", 2, None),
    ("sec", "pwr", "Quick Reference for External Power Design", 2, None),

    ("h1", "JTAG Debug Mode"),
    ("intro", "jtag"),
    ("sec", "jtag", "Prerequisites", 2, None),
    ("sec", "jtag", "Open the Workspace", 2, None),
    ("sec", "jtag", "Create a Platform", 2, None),
    ("sec", "jtag", "Create an Application", 2, None),
    ("sec", "jtag", "Replace the Sources with the Course Template", 2, None),
    ("sec", "jtag", "Set the Console UART (BSP)", 2, None),
    ("sec", "jtag", "Build", 2, None),
    ("sec", "jtag", "Debug over JTAG", 2, None),
    ("sec", "jtag", "Inspection Tools", 2, None),
    ("sec", "jtag", "Troubleshooting", 2, None),

    ("h1", "Standalone Boot"),
    ("intro", "boot"),
    ("sec", "boot", "How Standalone Boot Works", 2, None),
    ("sec", "boot", "Prerequisites", 2, None),
    ("sec", "boot", "Create the Bootloader Component", 2, None),
    ("sec", "boot", "Configure the Bootloader", 2, None),
    ("sec", "boot", "Build the Bootloader", 2, None),
    ("sec", "boot", "Merge the Bootloader into the Hardware Image", 2, None),
    ("sec", "boot", "Prepare the Application Image", 2, None),
    ("sec", "boot", "Program the Flash", 2, None),
    ("sec", "boot", "Boot and Verify", 2, None),
    ("sec", "boot", "Updating the Application", 2, None),
    ("sec", "boot", "Working with JTAG Debug Mode", 2, None),
    ("sec", "boot", "Troubleshooting", 2, None),
]

# Sections deliberately left out, with the reason (coverage check enforces this)
DROPPED = {
    ("pin", "GPIO Groups"): "duplicates the GPIO group table in Peripherals/GPIO",
    ("pin", "Interrupt Inputs"): "DIP pins already in the EXTI table; FPGA pins in the pin maps",
    ("pin", "PWM Outputs"): "duplicates the PWM table in Peripherals/Timers",
    ("pin", "UART Interfaces"): "duplicates the UART sections in Peripherals",
    ("pin", "Analog Inputs (XADC)"): "channel table duplicates the XADC section (circuit subsection kept)",
    ("pin", "Serial Expansion Pins (I2C / SPI)"): "duplicates the I2C/SPI sections in Peripherals",
}

HEADING = re.compile(r"^(#{2,5})\s+([\d.]+)?\.?\s*(.*?)\s*$")


# ---------------------------------------------------------------- parsing
def strip_comments(lines):
    out, inside = [], False
    for ln in lines:
        s = ln.strip()
        if not inside and s.startswith("<!--"):
            inside = not s.endswith("-->")
            continue
        if inside:
            inside = not s.endswith("-->")
            continue
        out.append(ln)
    return out


class Node:
    def __init__(self, level, title, idx):
        self.level, self.title, self.idx = level, title, idx
        self.end = None          # exclusive end of deep body
        self.children = []


def parse_md(path):
    lines = strip_comments(open(path).read().splitlines())
    nodes, order = {}, []
    stack = []
    for i, ln in enumerate(lines):
        m = HEADING.match(ln)
        if not m or ln.startswith("####"):
            continue
        level = len(m.group(1))
        node = Node(level, m.group(3), i)
        while stack and stack[-1].level >= level:
            stack.pop().end = i
        if stack:
            stack[-1].children.append(node)
        stack.append(node)
        order.append(node)
        if node.title in nodes:
            sys.exit("duplicate section title in %s: %s" % (path, node.title))
        nodes[node.title] = node
    for n in stack:
        n.end = len(lines)
    intro_end = order[0].idx if order else len(lines)
    intro = [l for l in lines[:intro_end] if not l.startswith("# ")]
    return {"lines": lines, "nodes": nodes, "order": order,
            "intro": intro, "dir": os.path.dirname(path)}


def own_body(doc, node):
    end = node.children[0].idx if node.children else node.end
    return doc["lines"][node.idx + 1:end]


def deep_body(doc, node):
    return doc["lines"][node.idx + 1:node.end]


def is_blank(lines):
    return all(not l.strip() or l.strip() == "---"
               or re.match(r"\*\*(.+?):\*\*\s", l) for l in lines)


# ------------------------------------------------------------- conversion
def esc(t):
    t = re.sub(r"\\([\\`*_{}\[\]()#+.!~-])", r"\1", t)  # undo markdown escapes
    for ch in "\\#$@[]*_<~":
        t = t.replace(ch, "\\" + ch)
    return t


def inline(t, code):
    out, pos = [], 0
    for m in re.finditer(r"`([^`]+)`", t):
        out.append(esc(t[pos:m.start()]))
        out.append("__CODE%d__" % len(code))
        code.append(m.group(1))
        pos = m.end()
    out.append(esc(t[pos:]))
    t = "".join(out)
    t = re.sub(r"\\\*\\\*(.+?)\\\*\\\*", r"*\1*", t)               # **bold**
    t = re.sub(r"\\\*(.+?)\\\*", r"_\1_", t)                       # *italic*
    t = re.sub(r"\\\[(.+?)\\\]\((https?://.+?)\)",
               r"#link(\"\2\")[\1]", t)                            # web link
    t = re.sub(r"\\\[(.+?)\\\]\([^)]*\)", r"\1", t)                # file link -> text
    t = re.sub(r"__CODE(\d+)__", lambda m: "`%s`" % code[int(m.group(1))], t)
    return t


class Converter:
    def __init__(self, build_dir):
        self.build_dir = build_dir
        self.code = []
        self.out = []
        self.img_seq = 0

    def line(self, s=""):
        self.out.append(s)

    def inline(self, t):
        return inline(t, self.code)

    def flush_para(self, para):
        if para:
            self.line(self.inline(" ".join(para)))
            self.line()
            para.clear()

    def flush_table(self, rows):
        if not rows:
            return
        ncol = len(rows[0])
        body = [r for r in rows[1:]
                if not all(re.fullmatch(r":?-+:?", c) for c in r)]
        if ncol == 2:
            widest = 1
        else:
            avg = [sum(len(r[i]) for r in body if i < len(r)) /
                   max(len(body), 1) for i in range(ncol)]
            widest = avg.index(max(avg))
        cols = ", ".join("1fr" if i == widest else "auto"
                         for i in range(ncol))
        self.line("#table(")
        self.line("  columns: (%s)," % cols)
        self.line("  table.header(%s)," %
                  ", ".join("[*%s*]" % self.inline(c) if c.strip() else "[]"
                            for c in rows[0]))
        for r in body:
            r = (r + [""] * ncol)[:ncol]
            self.line("  " + ", ".join("[%s]" % self.inline(c)
                                       for c in r) + ",")
        self.line(")")
        self.line()
        rows.clear()

    def flush_quote(self, quote):
        if not quote:
            return
        title = "Note"
        m = re.match(r"\*\*(.+?):\*\*\s*(.*)$", quote[0])
        if m:
            title, quote[0] = m.group(1), m.group(2)
        lines = []
        for q in quote:
            if q.startswith("- "):
                lines.append("- " + self.inline(q[2:]))
            else:
                lines.append(self.inline(q))
        self.line('#note(title: "%s")[' % title)
        self.line("\n".join(lines))
        self.line("]")
        self.line()
        quote.clear()

    def flush_figure(self, fig, caption=None):
        if fig is None:
            return None
        path, alt = fig
        width = 95 if path.endswith(".svg") else 82  # diagrams need the room
        self.line('#figure(image("%s", width: %d%%), caption: [%s])'
                  % (path, width, caption if caption is not None
                     else self.inline(alt)))
        self.line()
        return None

    def convert(self, lines, src_dir):
        """Convert a chunk of markdown lines; headings use their # depth."""
        table, quote, para, fig = [], [], [], None
        li = None  # pending list item: (marker, [text parts])
        in_fence = False

        def flush_li():
            nonlocal li
            if li:
                self.line(li[0] + " " + self.inline(" ".join(li[1])))
                li = None

        for ln in lines:
            if in_fence:
                self.line(ln)
                if ln.startswith("```"):
                    in_fence = False
                    self.line()
                continue
            if ln.startswith("```"):
                flush_li(); self.flush_para(para); fig = self.flush_figure(fig)
                self.flush_table(table); self.flush_quote(quote)
                self.line(ln)
                in_fence = True
                continue
            if ln.startswith("|"):
                flush_li(); self.flush_para(para); fig = self.flush_figure(fig)
                table.append([c.strip()
                              for c in ln.strip().strip("|").split("|")])
                continue
            self.flush_table(table)
            if ln.startswith(">"):
                flush_li(); self.flush_para(para); fig = self.flush_figure(fig)
                quote.append(ln.lstrip("> ").rstrip())
                continue
            self.flush_quote(quote)
            if ln.strip() == "---":
                flush_li(); self.flush_para(para); fig = self.flush_figure(fig)
                continue
            m = HEADING.match(ln)
            if m:
                flush_li(); self.flush_para(para); fig = self.flush_figure(fig)
                self.line("=" * (len(m.group(1)) - 1) + " " +
                          self.inline(m.group(3)))
                self.line()
                continue
            m = re.match(r"!\[(.*?)\]\((.*?)\)", ln.strip())
            if m:
                flush_li(); self.flush_para(para); fig = self.flush_figure(fig)
                img = os.path.normpath(os.path.join(src_dir, m.group(2)))
                base = "img%d_%s" % (self.img_seq, os.path.basename(img))
                self.img_seq += 1
                shutil.copy(img, os.path.join(self.build_dir, base))
                fig = (base, m.group(1))
                continue
            m = re.match(r"^\*Figure\s+\d+\.?\s*(.*)\*\s*$", ln.strip())
            if m and fig is not None:
                fig = self.flush_figure(fig, caption=self.inline(m.group(1)))
                continue
            if not ln.strip():
                flush_li()
                self.flush_para(para)
                continue
            fig = self.flush_figure(fig)
            m = re.match(r"^-\s+(.*)$", ln)
            if m:
                flush_li(); self.flush_para(para)
                li = ("-", [m.group(1)])
                continue
            m = re.match(r"^\d+\.\s+(.*)$", ln)
            if m:
                flush_li(); self.flush_para(para)
                li = ("+", [m.group(1)])
                continue
            if li:
                li[1].append(ln.strip())  # continuation of a wrapped item
                continue
            para.append(ln.strip())
        flush_li()
        self.flush_para(para)
        self.flush_table(table)
        self.flush_quote(quote)
        self.flush_figure(fig)


# ---------------------------------------------------------------- assembly
def shift_headings(lines, delta):
    out = []
    for ln in lines:
        m = HEADING.match(ln)
        if m:
            ln = "#" * (len(m.group(1)) + delta) + " " + m.group(3)
        out.append(ln)
    return out


def main():
    typst = os.environ.get("TYPST") or shutil.which("typst")
    if not typst:
        sys.exit("typst CLI not found - install from https://typst.app "
                 "or set TYPST=/path/to/typst")
    docs = {alias: parse_md(path) for alias, path in SOURCES.items()}

    # title-page metadata comes from the IP reference preamble
    meta = {}
    for ln in docs["ip"]["intro"]:
        m = re.match(r"\*\*(.+?):\*\*\s*(.+?)\s*$", ln)
        if m:
            meta[m.group(1).split()[0].lower()] = m.group(2)

    build = tempfile.mkdtemp(prefix="cmod_datasheet_")
    ok = False
    # ip/mem intros = standalone-doc metadata blocks, not datasheet content
    used_nodes, used_intros = set(), {"ip", "ov", "mem"}
    try:
        conv = Converter(build)
        for entry in RECIPE:
            kind = entry[0]
            if kind in ("h1", "h2"):
                conv.line("=" * (1 if kind == "h1" else 2) + " " +
                          conv.inline(entry[1]))
                conv.line()
            elif kind == "text":
                conv.convert(entry[1].splitlines() + [""], HERE)
            elif kind == "typst":
                conv.line(entry[1])
                conv.line()
            elif kind == "intro":
                alias = entry[1]
                used_intros.add(alias)
                conv.convert(docs[alias]["intro"] + [""], docs[alias]["dir"])
            elif kind == "body":
                alias, title = entry[1], entry[2]
                node = docs[alias]["nodes"][title]
                used_nodes.add((alias, title))
                conv.convert(own_body(docs[alias], node) + [""],
                             docs[alias]["dir"])
            elif kind == "sec":
                alias, title, level, retitle = entry[1:]
                doc = docs[alias]
                node = doc["nodes"][title]
                mark = [node]
                while mark:
                    n = mark.pop()
                    used_nodes.add((alias, n.title))
                    mark.extend(n.children)
                chunk = ["#" * (level + 1) + " " + (retitle or node.title)]
                chunk += shift_headings(deep_body(doc, node),
                                        level + 1 - node.level)
                conv.convert(chunk + [""], doc["dir"])

        # coverage check: every source section is used or explicitly dropped
        problems = []
        for alias, doc in docs.items():
            if alias not in used_intros and not is_blank(doc["intro"]):
                problems.append("%s: file intro unused" % alias)
            for node in doc["order"]:
                key = (alias, node.title)
                if key in used_nodes:
                    continue
                if key in DROPPED:
                    continue
                if is_blank(own_body(doc, node)) and all(
                        (alias, c.title) in used_nodes or
                        (alias, c.title) in DROPPED
                        for c in node.children):
                    continue
                problems.append("%s: section %r not accounted for"
                                % (alias, node.title))
        for key in DROPPED:
            if key[1] not in docs[key[0]]["nodes"]:
                problems.append("DROPPED entry %r does not exist" % (key,))
        if problems:
            sys.exit("COVERAGE FAILURE:\n  " + "\n  ".join(problems))
        print("coverage OK: %d sections used, %d dropped (documented)"
              % (len(used_nodes), len(DROPPED)))

        body = "\n".join(conv.out)
        # attach labels to reassembled section headings
        LABELS = [
            (r"^(== Interrupt Controller[^\n]*)", "sec-intc"),
            (r"^(== Complete Address Map[^\n]*)", "sec-map"),
            (r"^(= Pinout[^\n]*)", "ch-pinout"),
            (r"^(= Peripherals[^\n]*)", "ch-periph"),
            (r"^(== QSPI Flash[^\n]*)", "sec-flash"),
            (r"^(= Standalone Boot[^\n]*)", "ch-standalone"),
            (r"^(= JTAG Debug Mode[^\n]*)", "ch-jtag"),
            (r"^(= Electrical Characteristics and Power[^\n]*)", "ch-electrical"),
            (r"^(== Prepare the Application Image[^\n]*)", "sec-appimage"),
            (r"^(== Create a Platform[^\n]*)", "sec-platform"),
            (r"^(== Create an Application[^\n]*)", "sec-createapp"),
            (r"^(== Set the Console UART[^\n]*)", "sec-uart"),
            (r"^(== Troubleshooting[^\n]*)", "sec-jtag-ts"),
            (r"^(=== Analog Input Circuit[^\n]*)", "sec-analog"),
        ]
        for pat, label in LABELS:
            body, n = re.subn(pat, r"\1 <%s>" % label, body,
                              count=1, flags=re.M)
            if n == 0:
                print("WARNING: label target not found: %s" % label)
        # repair references that were correct in the standalone files but
        # dangle after integration (stale intra-file § numbers, old doc names)
        REFS = [
            ("(see the Standalone Boot Mode guide)", "(see @ch-standalone)"),
            ("as described in the JTAG Debug Mode chapter",
             "as described in @ch-jtag"),
            ("underlying the Electrical Characteristics chapter",
             "underlying @ch-electrical"),
            ("built in the JTAG Debug Mode chapter", "built in @ch-jtag"),
            ("skip directly to the application image step.",
             "skip directly to @sec-appimage."),
            ("skip to section 3.", "skip to @sec-createapp."),
            ("created in section 2", "created in @sec-platform"),
            ("(such as section 5)", "(such as @sec-uart)"),
            ("redo section 5,", "redo @sec-uart,"),
            ("see the Troubleshooting section.", "see @sec-jtag-ts."),
            ("Peripheral registers and base addresses are documented in "
             "the IP peripheral reference.",
             "Peripheral registers and base addresses are listed in "
             "@sec-map and detailed in @ch-periph."),
            ("Devices are addressed by their 7-bit I2C address.",
             "Devices are addressed by their 7-bit I2C address. "
             "Interrupt routing is listed in @sec-intc."),
            ("(on-board divider scales to the XADC's 0–1 V)",
             "(on-board divider scales to the XADC's 0–1 V; see @sec-analog)"),
            ("This guide describes how", "This chapter describes how"),
        ]
        for old, new in REFS:
            if old not in body:
                print("WARNING: reference-repair pattern not found: %r" % old)
            body = body.replace(old, new)
        for ln in body.splitlines():
            if "§" in ln:
                print("WARNING: residual '§' reference: %s" % ln.strip()[:100])
        with open(os.path.join(build, "body.typ"), "w") as f:
            f.write('#import "defs.typ": *\n\n' + body)
        with open(os.path.join(build, "meta.typ"), "w") as f:
            for key in ("platform", "processor", "system", "toolchain"):
                f.write("#let meta-%s = [%s]\n"
                        % (key, inline(meta.get(key, ""), conv.code)))
        for name in ("datasheet.typ", "defs.typ"):
            shutil.copy(os.path.join(HERE, name), build)
        subprocess.run([typst, "compile",
                        os.path.join(build, "datasheet.typ"), OUT_PDF],
                       check=True)
        print("OK %s (%d bytes)" % (OUT_PDF, os.path.getsize(OUT_PDF)))
        ok = True
    finally:
        if ok:
            shutil.rmtree(build)
        else:
            print("build dir kept for debugging: %s" % build)


main()
