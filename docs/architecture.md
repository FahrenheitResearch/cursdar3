# Cursdar3 Architecture

## Intent

Cursdar3 starts by separating reusable radar engine code from workstation shell code.

## Working split

### Reuse from Cursdar2

- `src/cuda/`
- `src/nexrad/`
- `src/net/`
- most of `src/render/`

### Rewrite for Cursdar3

- shell layout
- workspace routing
- pane/link model
- context dock
- global time deck
- command surfaces

## Shell model

The shell is built from five permanent regions:

- `TopCommandBar`
- `WorkspaceRail`
- `RadarCanvas`
- `ContextDock`
- `TimeDeck`

These regions are fixed. Features move between them contextually rather than spawning permanent windows.

## State model direction

The target is explicit session state:

- global workspace id
- global time controller
- pane layout and pane link masks
- active context selection
- dock tab state
- asset drawer state

This replaces the current `cursdar2` model where live/archive/snapshot/3D/cross-section/loop/show-all/compare are mostly independent flags.
