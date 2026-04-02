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

## State model

The shell now centers on explicit session state in `src/ui/workstation.h`:

- `ConsoleSession`
- `TransportState`
- `AlertFocusState`
- `PaneState`
- `StationWorkflowState`

The current shell model is:

- the shell owns workflow intent
- the engine owns data, render surfaces, and GPU orchestration
- `App` exposes transport and selected-alert controller surfaces back to the shell

This replaces the old `cursdar2` pattern where live/archive/snapshot/3D/cross-section/loop/show-all/compare were mostly independent flags.

## Implemented direction

The current bootstrap already includes:

- shell-owned workspace selection
- shell-owned focused / hovered / locked station workflow state
- pane-scoped product/tilt selection
- a unified time deck over live review / archive / snapshot transport
- selected-alert state bridged into `App`
- a first alert-driven workspace action: `Tornado Interrogate`

## Next seam to finish

The remaining big architecture step is to replace the last global-engine assumptions with pane-aware engine state:

- pane view transforms
- pane station bindings that are not forced back to the global active station
- transport binding per pane/group
- template application as an engine-facing controller instead of imperative UI macros
