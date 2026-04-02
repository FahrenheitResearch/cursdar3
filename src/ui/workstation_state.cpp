#include "workstation_state.h"

#include <algorithm>

namespace ui {

WorkstationShellState defaultWorkstationShellState() {
    WorkstationShellState state;
    state.workspace = WorkspaceId::Live;
    state.dockTab = ContextDockTab::Inspect;
    state.timeMode = TimeDeckMode::LiveTail;
    state.contextDockOpen = true;
    return state;
}

WorkstationRegionRects computeWorkstationShellRects(int viewportWidth, int viewportHeight,
                                                    bool dockOpen) {
    WorkstationRegionRects rects;

    const int topH = 72;
    const int timeH = std::min(112, std::max(92, viewportHeight / 6));
    const int railW = 86;
    const int dockW = dockOpen ? 360 : 0;

    rects.top_x = 0;
    rects.top_y = 0;
    rects.top_w = viewportWidth;
    rects.top_h = topH;

    rects.time_x = 0;
    rects.time_h = timeH;
    rects.time_y = std::max(0, viewportHeight - rects.time_h);
    rects.time_w = viewportWidth;

    rects.rail_x = 0;
    rects.rail_y = topH;
    rects.rail_w = railW;
    rects.rail_h = std::max(0, rects.time_y - topH);

    rects.dock_w = dockW;
    rects.dock_x = std::max(0, viewportWidth - dockW);
    rects.dock_y = topH;
    rects.dock_h = std::max(0, rects.time_y - topH);

    rects.canvas_x = railW;
    rects.canvas_y = topH;
    rects.canvas_w = std::max(0, viewportWidth - railW - dockW);
    rects.canvas_h = std::max(0, rects.time_y - topH);

    return rects;
}

} // namespace ui
