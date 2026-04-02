# Cursdar3

Cursdar3 is the next shell for the CUDA radar engine: a radar interrogation console, not a panel farm.

This repo starts with the new workstation architecture first:

- top command bar
- left workspace rail
- center radar canvas
- right context dock
- bottom time deck

The goal is to keep the proven ingest/rendering engine from `cursdar2`, while replacing the window-heavy UI and flag-heavy mode model with explicit workspaces, shared time control, and linked panes.

## Current status

This is the initial workstation-shell rewrite on top of the existing radar engine. `cursdar3` already builds and runs against the current ingest/render/rendering stack; what is still being rebuilt is the shell, workspace model, and interaction architecture.

## Design rules

- The radar owns the screen.
- Time is global.
- Every feature gets exactly one home.
- Compare panes are linked views, not separate worlds.
- Advanced tools stay contextual, not permanently visible.

## Planned work

1. Lock the workspace/state model.
2. Replace the remaining flag-heavy app wiring with explicit workspaces and a unified temporal state machine.
3. Turn compare panes into linked interrogation views instead of layout toggles.
4. Rebuild warning interrogation, archive/live transport, and asset flows on top of the new shell.
