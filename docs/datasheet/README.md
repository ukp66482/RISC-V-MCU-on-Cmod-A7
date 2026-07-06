# Unified MCU Datasheet

`Cmod_A7_MCU_Datasheet.pdf` is a single datasheet built from the markdown
under `docs/` — not a concatenation: the build parses every file into
sections and reassembles them into a real datasheet outline (overview →
system architecture → pinout → peripherals → electrical/power → boot →
development), drops sections that duplicate others, and rewrites
cross-references that only made sense inside the standalone files into live
internal section links.

The markdown files remain the single source of truth. Do **not** edit the
PDF — edit the markdown, then rebuild:

```bash
python3 build_datasheet.py        # needs the `typst` CLI on PATH
```

| File | Role |
|------|------|
| `build_datasheet.py` | section-level markdown → Typst assembler; the datasheet outline lives in `RECIPE`, deliberate omissions in `DROPPED` |
| `overview.md` | Chapter 1 source (introduction, features, block diagram, reference documents) — the only datasheet-specific content |
| `datasheet.typ` | page template: title page, headers/footers, styles |
| `defs.typ` | shared building blocks (note boxes) |
| `Cmod_A7_MCU_Datasheet.pdf` | build output |
| `archive/` | previous edition (v1, chapter-per-file concatenation) |

Safety nets built into the build:

- **Coverage check** — every section of every source file must be used in
  `RECIPE` or listed in `DROPPED` with a reason, otherwise the build fails.
  Renaming a section in a source file therefore breaks the build loudly
  instead of silently losing content.
- **Reference repair** — phrases like "see the Standalone Boot Mode guide"
  or "(§2)" are rewritten to live `@` cross-references; a warning is printed
  if a pattern no longer matches the sources.

Typst is a single static binary: <https://github.com/typst/typst/releases>.
