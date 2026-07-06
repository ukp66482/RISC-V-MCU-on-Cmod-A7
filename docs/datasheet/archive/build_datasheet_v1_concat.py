#!/usr/bin/env python3
"""Build the unified Cmod A7 MCU datasheet & reference manual.

The markdown files under docs/ stay the single source of truth; this script
converts them to Typst (one chapter each), applies the datasheet template,
and compiles Cmod_A7_MCU_Datasheet.pdf next to this script.

Requires the `typst` CLI on PATH (https://typst.app - single static binary),
or set the TYPST environment variable to the binary path.
"""
import os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.dirname(HERE)
SOURCES = [  # (markdown file, chapter title, section titles to skip)
    ("IP-Specification/Cmod_A7_IP_Peripheral_Reference.md",
     "IP Peripheral Reference", set()),
    ("Pin-Specification/Cmod_A7_Pin_Specification.md",
     "Pin Specification", set()),
    ("Power-Specification/Cmod_A7_Power_Specification.md",
     "Power Specification", set()),
    ("guides/JTAG-Debug-Mode/JTAG-Debug-Mode.md",
     "JTAG Debug Mode", set()),
    ("guides/Standalone-Boot-Mode/Standalone-Boot-Mode.md",
     "Standalone Boot Mode", set()),
    ("guides/README.md",
     "Vitis Quick Reference", {"Guides in This Directory"}),
]
OUT_PDF = os.path.join(HERE, "Cmod_A7_MCU_Datasheet.pdf")


def esc(t):
    """Escape Typst specials in plain (non-code) text."""
    t = re.sub(r"\\([\\`*_{}\[\]()#+.!~-])", r"\1", t)  # undo markdown escapes
    for ch in "\\#$@[]*_<~":
        t = t.replace(ch, "\\" + ch)
    return t


def inline(t, code):
    """Markdown inline -> Typst inline (code spans kept verbatim as raw)."""
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
        self.meta = {}
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
        self.line('#figure(image("%s", width: 82%%), caption: [%s])'
                  % (path, caption if caption is not None
                     else self.inline(alt)))
        self.line()
        return None

    def convert(self, md_path, chapter_title, skip_sections):
        src_dir = os.path.dirname(md_path)
        self.line("= %s" % self.inline(chapter_title))
        self.line()
        table, quote, para, fig = [], [], [], None
        saw_heading = skipping = in_fence = False
        for ln in open(md_path).read().splitlines():
            if in_fence:
                self.line(ln)
                if ln.startswith("```"):
                    in_fence = False
                    self.line()
                continue
            if skipping and not re.match(r"^##\s", ln):
                continue
            skipping = False
            if ln.startswith("# ") and not saw_heading:
                continue  # document title -> replaced by chapter title
            if ln.startswith("##"):
                saw_heading = True
            m = re.match(r"\*\*(.+?):\*\*\s*(.+?)\s*$", ln)
            if m and not saw_heading:
                self.meta.setdefault(m.group(1).split()[0].lower(),
                                     self.inline(m.group(2)))
                continue
            if ln.startswith("```"):
                self.flush_para(para); fig = self.flush_figure(fig)
                self.flush_table(table); self.flush_quote(quote)
                self.line(ln)
                in_fence = True
                continue
            if ln.startswith("|"):
                self.flush_para(para); fig = self.flush_figure(fig)
                table.append([c.strip()
                              for c in ln.strip().strip("|").split("|")])
                continue
            self.flush_table(table)
            if ln.startswith(">"):
                self.flush_para(para); fig = self.flush_figure(fig)
                quote.append(ln.lstrip("> ").rstrip())
                continue
            self.flush_quote(quote)
            if ln.strip() == "---":
                self.flush_para(para); fig = self.flush_figure(fig)
                continue
            m = re.match(r"^(#{2,3})\s+([\d.]+)?\.?\s*(.*)$", ln)
            if m:
                self.flush_para(para); fig = self.flush_figure(fig)
                if len(m.group(1)) == 2 and m.group(3) in skip_sections:
                    skipping = True
                    continue
                self.line("=" * len(m.group(1)) + " " +
                          self.inline(m.group(3)))
                self.line()
                continue
            m = re.match(r"!\[(.*?)\]\((.*?)\)", ln.strip())
            if m:
                self.flush_para(para); fig = self.flush_figure(fig)
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
                self.flush_para(para)
                continue
            fig = self.flush_figure(fig)
            m = re.match(r"^-\s+(.*)$", ln)
            if m:
                self.flush_para(para)
                self.line("- " + self.inline(m.group(1)))
                continue
            m = re.match(r"^\d+\.\s+(.*)$", ln)
            if m:
                self.flush_para(para)
                self.line("+ " + self.inline(m.group(1)))
                continue
            para.append(ln.strip())
        self.flush_para(para)
        self.flush_table(table)
        self.flush_quote(quote)
        self.flush_figure(fig)


def main():
    typst = os.environ.get("TYPST") or shutil.which("typst")
    if not typst:
        sys.exit("typst CLI not found - install from https://typst.app "
                 "or set TYPST=/path/to/typst")
    build = tempfile.mkdtemp(prefix="cmod_datasheet_")
    ok = False
    try:
        conv = Converter(build)
        for rel, title, skip in SOURCES:
            conv.convert(os.path.join(DOCS, rel), title, skip)
        body = "\n".join(conv.out)
        # live cross-reference: label the INTC section, cite it from I2C
        body = re.sub(r"^(===? AXI Interrupt Controller[^\n]*)",
                      r"\1 <sec-intc>", body, count=1, flags=re.M)
        body = body.replace(
            "Devices are addressed by their 7-bit I2C address.",
            "Devices are addressed by their 7-bit I2C address. "
            "Interrupt routing is listed in @sec-intc.")
        with open(os.path.join(build, "body.typ"), "w") as f:
            f.write('#import "defs.typ": *\n\n' + body)
        with open(os.path.join(build, "meta.typ"), "w") as f:
            for key in ("platform", "processor", "system", "toolchain"):
                f.write("#let meta-%s = [%s]\n"
                        % (key, conv.meta.get(key, "")))
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
