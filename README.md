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

This is the initial workstation-shell rewrite on top of the existing radar engine. `cursdar3` already builds and runs against the current ingest/render/rendering stack, and the shell now has a real session model instead of just being a different skin over `cursdar2`.

What is in place now:

- shell-owned `ConsoleSession` / pane / transport / alert-focus state
- pane-aware command bar and context dock
- center-canvas ownership for single, dual, and quad layouts
- a unified time deck that drives live review, archive review, and snapshot state through one shell transport model
- alert selection with candidate-station resolution and a first real `Tornado Interrogate` workspace action

## Design rules

- The radar owns the screen.
- Time is global.
- Every feature gets exactly one home.
- Compare panes are linked views, not separate worlds.
- Advanced tools stay contextual, not permanently visible.

## Planned work

1. Finish moving workflow authority out of `App` and into the shell/session layer.
2. Promote pane links from shell state into a real engine-facing pane/view graph.
3. Unify live loop, archive playback, and snapshots behind a single frame-stream controller.
4. Turn alert focus and interrogation templates into first-class workflows instead of ad hoc actions.
