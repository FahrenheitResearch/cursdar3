#pragma once

namespace ui {

enum class WorkspaceId {
    Live = 0,
    Compare,
    Archive,
    Warning,
    Volume,
    Tools,
    Assets
};

enum class ContextDockTab {
    Inspect = 0,
    Alerts,
    Layers,
    Assets,
    Session
};

enum class TimeDeckMode {
    LiveTail = 0,
    Review,
    Archive,
    Snapshot
};

struct PaneLinkState {
    bool geo = true;
    bool time = true;
    bool station = true;
    bool tilt = true;
};

struct WorkstationShellState {
    WorkspaceId workspace = WorkspaceId::Live;
    ContextDockTab dockTab = ContextDockTab::Inspect;
    TimeDeckMode timeMode = TimeDeckMode::LiveTail;
    PaneLinkState links = {};
    bool contextDockOpen = true;
};

struct WorkstationRegionRects {
    int top_x = 0;
    int top_y = 0;
    int top_w = 0;
    int top_h = 0;
    int rail_x = 0;
    int rail_y = 0;
    int rail_w = 0;
    int rail_h = 0;
    int canvas_x = 0;
    int canvas_y = 0;
    int canvas_w = 0;
    int canvas_h = 0;
    int dock_x = 0;
    int dock_y = 0;
    int dock_w = 0;
    int dock_h = 0;
    int time_x = 0;
    int time_y = 0;
    int time_w = 0;
    int time_h = 0;
};

WorkstationShellState defaultWorkstationShellState();
WorkstationRegionRects computeWorkstationShellRects(int viewportWidth, int viewportHeight,
                                                    bool dockOpen);

} // namespace ui
