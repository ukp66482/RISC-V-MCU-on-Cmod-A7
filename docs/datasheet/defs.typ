// Shared building blocks for the datasheet (imported by template and body)
#let note(title: "Note", body) = block(
  width: 100%, fill: rgb("#f3f6f9"), inset: 8pt, radius: 2pt,
  stroke: (left: 2.5pt + rgb("#4a6a8a")),
)[#text(weight: "bold", size: 8.5pt)[#title: ]#text(size: 8.5pt)[#body]]
