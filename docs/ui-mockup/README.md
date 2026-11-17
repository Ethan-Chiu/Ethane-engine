# Editor UI mockup

The editor's look is designed here first, in HTML/CSS, and replicated in the engine (ImGui, through
`Ethane-Editor/src/Theme/EditorTheme.h`) once it's settled.

- `project/Main.dc.html`: the editor window at 1440×900 (dark). Its interactive bits (tools, stele tabs,
  play / stop, hierarchy rows, content tiles) are wired in the script at the bottom.
- `project/canvas.json`: the canvas index.

It's a Design canvas file: it renders on the canvas at
https://claude.ai/artifact/LFRvBzC5EYkMRbEoeBwjHh (private to the owner), which provides `support.js` and the
`<x-dc>` runtime. Opening the file directly in a browser shows the markup without that runtime.

To iterate: edit `project/Main.dc.html` here and republish it to the canvas (or edit on the canvas and copy
the result back), so this folder stays the source of truth.

## Design language

- Dark, neutral charcoal backgrounds; rounded panes (10 px) floating on a darker window with 6 px gaps.
- Palette by role: cream `#FFF3E3` text; mint `#D5E8E6` selection tint, play, folders; periwinkle
  `#9FA0FF` small marks (active tool, slider fill, toggles, the selection outline); coral `#FF7171` stop,
  unsaved, errors. Axes everywhere: X coral, Y mint, Z periwinkle.
- Tools as a compact vertical glass capsule in the viewport's top-right corner (icon segments: Move,
  Rotate, Scale, then World / Local; the selected one in a lighter rounded square), after Apple's tab bars.
- Inspector: components as grouped cards; labels left, values right; X / Y / Z fields for vectors.
- Type: Inter for UI, JetBrains Mono for numbers and ids.
