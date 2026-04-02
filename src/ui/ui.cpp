#include "ui.h"
#include "workstation.h"
#include "app.h"
#include "nexrad/products.h"
#include "net/warnings.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace ui {
namespace {

bool g_uiWantsMouseCapture = false;

struct ShellRegions {
    ImRect topBar;
    ImRect leftRail;
    ImRect centerCanvas;
    ImRect rightDock;
    ImRect timeDeck;
};

ShellRegions computeRegions(const ImGuiViewport* viewport, bool dockOpen) {
    constexpr float pad = 12.0f;
    constexpr float topH = 70.0f;
    constexpr float leftW = 88.0f;
    constexpr float rightW = 356.0f;
    constexpr float bottomH = 116.0f;
    constexpr float gap = 10.0f;

    const ImVec2 pos = viewport->Pos;
    const ImVec2 size = viewport->Size;
    ShellRegions r{};

    r.topBar = ImRect(ImVec2(pos.x + pad, pos.y + pad),
                      ImVec2(pos.x + size.x - pad, pos.y + pad + topH));

    const float bodyTop = r.topBar.Max.y + gap;
    const float bodyBottom = pos.y + size.y - pad - bottomH - gap;
    const float rightWidth = dockOpen ? rightW : 0.0f;
    const float centerLeft = pos.x + pad + leftW + gap;
    const float centerRight = pos.x + size.x - pad - rightWidth - (dockOpen ? gap : 0.0f);

    r.leftRail = ImRect(ImVec2(pos.x + pad, bodyTop),
                        ImVec2(pos.x + pad + leftW, bodyBottom));
    r.centerCanvas = ImRect(ImVec2(centerLeft, bodyTop),
                            ImVec2(centerRight, bodyBottom));
    r.rightDock = ImRect(ImVec2(pos.x + size.x - pad - rightWidth, bodyTop),
                         ImVec2(pos.x + size.x - pad, bodyBottom));
    r.timeDeck = ImRect(ImVec2(centerLeft, pos.y + size.y - pad - bottomH),
                        ImVec2(pos.x + size.x - pad, pos.y + size.y - pad));
    return r;
}

void beginFixedWindow(const char* name, const ImRect& rect, ImGuiWindowFlags extra = 0, float alpha = 0.96f) {
    ImGui::SetNextWindowPos(rect.Min, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rect.GetWidth(), rect.GetHeight()), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(alpha);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoTitleBar |
                             extra;
    ImGui::Begin(name, nullptr, flags);
}

void endFixedWindow() {
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        g_uiWantsMouseCapture = true;
    ImGui::End();
}

void resetConusView(App& app) {
    app.viewport().center_lat = 39.0;
    app.viewport().center_lon = -98.0;
    app.viewport().zoom = 28.0;
}

const char* performanceProfileLabel(PerformanceProfile profile) {
    switch (profile) {
        case PerformanceProfile::Auto: return "Auto";
        case PerformanceProfile::Quality: return "Quality";
        case PerformanceProfile::Balanced: return "Balanced";
        case PerformanceProfile::Performance: return "Performance";
        default: return "Unknown";
    }
}

std::string formatBytes(size_t bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = (double)bytes;
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    char buffer[32];
    if (unit == 0)
        std::snprintf(buffer, sizeof(buffer), "%llu %s", (unsigned long long)bytes, units[unit]);
    else
        std::snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unit]);
    return buffer;
}

void drawTimeline(App& app, const char* id, float height = 28.0f) {
    const int targetFrames = std::max(1, app.liveLoopLength());
    const int available = std::max(0, app.liveLoopAvailableFrames());
    const int loadedStart = std::max(0, targetFrames - available);
    const int playback = std::clamp(app.liveLoopPlaybackFrame(), 0, std::max(0, available - 1));
    const int currentSlot = available > 0 ? std::clamp(loadedStart + playback, 0, targetFrames - 1) : targetFrames - 1;

    const float width = std::max(260.0f, ImGui::GetContentRegionAvail().x);
    ImGui::InvisibleButton(id, ImVec2(width, height));
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    draw->AddRectFilled(min, max, IM_COL32(18, 22, 30, 230), 6.0f);
    draw->AddRect(min, max, IM_COL32(70, 78, 92, 255), 6.0f);

    const float slotWidth = (max.x - min.x - 8.0f) / (float)targetFrames;
    for (int slot = 0; slot < targetFrames; ++slot) {
        ImU32 col = slot >= loadedStart ? IM_COL32(52, 170, 108, 255) : IM_COL32(46, 52, 62, 255);
        const float x0 = min.x + 4.0f + slotWidth * slot;
        const float x1 = (slot == targetFrames - 1) ? (max.x - 4.0f) : (min.x + 4.0f + slotWidth * (slot + 1));
        draw->AddRectFilled(ImVec2(x0 + 1.0f, min.y + 4.0f), ImVec2(x1 - 1.0f, max.y - 4.0f), col, 2.0f);
    }

    const float currentX = min.x + 4.0f + slotWidth * (float)currentSlot;
    draw->AddRectFilled(ImVec2(currentX, min.y + 1.5f), ImVec2(std::min(max.x - 4.0f, currentX + std::max(2.0f, slotWidth)), max.y - 1.5f),
                        IM_COL32(255, 214, 96, 255), 3.0f);

    if (ImGui::IsItemHovered() && available > 0) {
        const float normalized = std::clamp((ImGui::GetIO().MousePos.x - (min.x + 4.0f)) / std::max(1.0f, max.x - min.x - 8.0f), 0.0f, 0.999999f);
        const int slot = std::clamp((int)std::floor(normalized * targetFrames), 0, targetFrames - 1);
        const int frameIndex = slot < loadedStart ? 0 : std::clamp(slot - loadedStart, 0, available - 1);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            app.setLiveLoopPlaybackFrame(frameIndex);
        }
        g_uiWantsMouseCapture = true;
    }
}

void drawCanvas(App& app, const ShellRegions& regions, const ImGuiViewport* mainViewport) {
    const ImRect rect = regions.centerCanvas;
    app.setRadarCanvasRect((int)(rect.Min.x - mainViewport->Pos.x),
                           (int)(rect.Min.y - mainViewport->Pos.y),
                           (int)rect.GetWidth(),
                           (int)rect.GetHeight());

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    const Viewport root = app.viewport();

    if (app.radarPanelCount() > 1 && !app.mode3D() && !app.crossSection()) {
        for (int pane = 0; pane < app.radarPanelCount(); ++pane) {
            const RadarPanelRect prect = app.radarPanelRect(pane);
            const ImVec2 min(mainViewport->Pos.x + (float)prect.x, mainViewport->Pos.y + (float)prect.y);
            const ImVec2 max(min.x + prect.width, min.y + prect.height);
            const Viewport paneVp = [&]() {
                Viewport vp = root;
                vp.width = prect.width;
                vp.height = prect.height;
                return vp;
            }();
            draw->PushClipRect(min, max, true);
            app.basemap().drawBase(draw, paneVp, min);
            draw->AddImage((ImTextureID)(uintptr_t)app.panelTexture(pane).textureId(), min, max);
            app.basemap().drawOverlay(draw, paneVp, min);
            draw->AddRect(min, max, IM_COL32(35, 42, 56, 180), 0.0f, 0, 1.0f);
            char label[96];
            std::snprintf(label, sizeof(label), "%s | T%d", PRODUCT_INFO[app.radarPanelProduct(pane)].name, app.activeTilt() + 1);
            draw->AddRectFilled(ImVec2(min.x + 10.0f, min.y + 10.0f),
                                ImVec2(min.x + 10.0f + ImGui::CalcTextSize(label).x + 16.0f, min.y + 34.0f),
                                IM_COL32(8, 12, 18, 210), 4.0f);
            draw->AddText(ImVec2(min.x + 18.0f, min.y + 15.0f), IM_COL32(230, 236, 244, 240), label);
            draw->PopClipRect();
        }
    } else {
        draw->PushClipRect(rect.Min, rect.Max, true);
        app.basemap().drawBase(draw, root, rect.Min);
        draw->AddImage((ImTextureID)(uintptr_t)app.outputTexture().textureId(), rect.Min, rect.Max);
        app.basemap().drawOverlay(draw, root, rect.Min);
        draw->AddRect(rect.Min, rect.Max, IM_COL32(35, 42, 56, 180), 0.0f, 0, 1.0f);
        char label[96];
        std::snprintf(label, sizeof(label), "%s | T%d", PRODUCT_INFO[app.activeProduct()].name, app.activeTilt() + 1);
        draw->AddRectFilled(ImVec2(rect.Min.x + 10.0f, rect.Min.y + 10.0f),
                            ImVec2(rect.Min.x + 10.0f + ImGui::CalcTextSize(label).x + 16.0f, rect.Min.y + 34.0f),
                            IM_COL32(8, 12, 18, 210), 4.0f);
        draw->AddText(ImVec2(rect.Min.x + 18.0f, rect.Min.y + 15.0f), IM_COL32(230, 236, 244, 240), label);
        draw->PopClipRect();
    }

    const std::string& attribution = app.basemap().attribution();
    if (!attribution.empty()) {
        const ImVec2 textSize = ImGui::CalcTextSize(attribution.c_str());
        const ImVec2 tl(rect.Min.x + 12.0f, rect.Max.y - textSize.y - 12.0f);
        draw->AddRectFilled(tl, ImVec2(tl.x + textSize.x + 10.0f, tl.y + textSize.y + 6.0f),
                            IM_COL32(6, 10, 16, 168), 4.0f);
        draw->AddText(ImVec2(tl.x + 5.0f, tl.y + 3.0f), IM_COL32(220, 228, 238, 210), attribution.c_str());
    }
}

void renderTopBar(App& app, WorkstationState& state, const ShellRegions& regions,
                  const std::vector<StationUiState>& stations,
                  const std::vector<WarningPolygon>& warnings) {
    beginFixedWindow("##c3_top_bar", regions.topBar);

    ImGui::TextColored(ImVec4(0.85f, 0.91f, 0.98f, 1.0f), "CURSDAR3");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("%s", workspaceLabel(state.activeWorkspace));
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("Site %s", app.activeStationName().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("Alerts %d", (int)warnings.size());
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("Loaded %d", app.stationsLoaded());

    ImGui::Separator();

    if (ImGui::Button(app.autoTrackStation() ? "Auto" : "Locked"))
        app.setAutoTrackStation(!app.autoTrackStation());
    ImGui::SameLine();
    if (ImGui::Button(app.liveLoopPlaying() ? "Pause" : "Play"))
        app.toggleLiveLoopPlayback();
    ImGui::SameLine();
    if (ImGui::Button("Live"))
        app.setLiveLoopPlaybackFrame(std::max(0, app.liveLoopAvailableFrames() - 1));
    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
        app.refreshData();
    ImGui::SameLine();
    if (ImGui::Button(app.mode3D() ? "Exit 3D" : "3D"))
        app.toggle3D();
    ImGui::SameLine();
    if (ImGui::Button(app.crossSection() ? "Hide XS" : "Cross Section"))
        app.toggleCrossSection();

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    for (int product = 0; product < (int)Product::COUNT; ++product) {
        if (product == app.activeProduct())
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.34f, 0.48f, 1.0f));
        if (ImGui::Button(PRODUCT_INFO[product].code))
            app.setProduct(product);
        if (product == app.activeProduct())
            ImGui::PopStyleColor();
        if (product != (int)Product::COUNT - 1)
            ImGui::SameLine();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    for (int tilt = 0; tilt < app.maxTilts(); ++tilt) {
        char label[8];
        std::snprintf(label, sizeof(label), "T%d", tilt + 1);
        if (tilt == app.activeTilt())
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.30f, 0.22f, 1.0f));
        if (ImGui::Button(label))
            app.setTilt(tilt);
        if (tilt == app.activeTilt())
            ImGui::PopStyleColor();
        if (tilt != app.maxTilts() - 1)
            ImGui::SameLine();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    int layoutIdx = app.radarPanelLayout() == RadarPanelLayout::Single ? 0 :
                    app.radarPanelLayout() == RadarPanelLayout::Dual ? 1 : 2;
    const char* layoutLabels[] = {"Single", "Dual", "Quad"};
    ImGui::SetNextItemWidth(96.0f);
    if (ImGui::Combo("##layout_combo", &layoutIdx, layoutLabels, IM_ARRAYSIZE(layoutLabels))) {
        app.setRadarPanelLayout(layoutIdx == 0 ? RadarPanelLayout::Single :
                                layoutIdx == 1 ? RadarPanelLayout::Dual :
                                                 RadarPanelLayout::Quad);
        state.activeWorkspace = layoutIdx == 0 ? WorkspaceId::Live : WorkspaceId::Compare;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Geo", &state.paneLinks[0].geo);
    ImGui::SameLine();
    ImGui::Checkbox("Time", &state.paneLinks[0].time);
    ImGui::SameLine();
    ImGui::Checkbox("Station", &state.paneLinks[0].station);
    ImGui::SameLine();
    ImGui::Checkbox("Tilt", &state.paneLinks[0].tilt);

    (void)stations;
    endFixedWindow();
}

void renderRail(App& app, WorkstationState& state, const ShellRegions& regions) {
    beginFixedWindow("##c3_workspace_rail", regions.leftRail);
    const std::array<WorkspaceId, 7> items = {
        WorkspaceId::Live, WorkspaceId::Compare, WorkspaceId::Archive, WorkspaceId::Warning,
        WorkspaceId::Volume, WorkspaceId::Tools, WorkspaceId::Assets
    };

    for (WorkspaceId id : items) {
        const bool active = (id == state.activeWorkspace);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.34f, 0.48f, 1.0f));
        if (ImGui::Button(workspaceLabel(id), ImVec2(-1.0f, 34.0f))) {
            state.activeWorkspace = id;
            if (id == WorkspaceId::Live) app.setRadarPanelLayout(RadarPanelLayout::Single);
            if (id == WorkspaceId::Compare && app.radarPanelLayout() == RadarPanelLayout::Single)
                app.setRadarPanelLayout(RadarPanelLayout::Dual);
            if (id == WorkspaceId::Warning) app.setRadarPanelLayout(RadarPanelLayout::Quad);
        }
        if (active) ImGui::PopStyleColor();
    }

    ImGui::Separator();
    if (ImGui::Button("CONUS", ImVec2(-1.0f, 32.0f)))
        resetConusView(app);
    if (ImGui::Button("Show All", ImVec2(-1.0f, 32.0f)))
        app.toggleShowAll();

    endFixedWindow();
}

void renderDock(App& app, WorkstationState& state, const ShellRegions& regions,
                const std::vector<StationUiState>& stations,
                const std::vector<WarningPolygon>& warnings) {
    if (!state.contextDockOpen)
        return;

    beginFixedWindow("##c3_context_dock", regions.rightDock);
    if (ImGui::BeginTabBar("##dock_tabs")) {
        if (ImGui::BeginTabItem("Inspect")) {
            state.activeDockTab = ContextDockTab::Inspect;
            ImGui::Text("Active Station: %s", app.activeStationName().c_str());
            ImGui::Text("Product: %s", PRODUCT_INFO[app.activeProduct()].name);
            ImGui::Text("Tilt: %.1f", app.activeTiltAngle());
            ImGui::Text("Loaded: %d / %d", app.stationsLoaded(), app.stationsTotal());
            const auto& mem = app.memoryTelemetry();
            ImGui::Separator();
            ImGui::Text("VRAM %s / %s", formatBytes(mem.gpu_used_bytes).c_str(), formatBytes(mem.gpu_total_bytes).c_str());
            ImGui::Text("RAM %s", formatBytes(mem.process_working_set_bytes).c_str());
            if (app.activeStation() >= 0 && app.activeStation() < (int)stations.size()) {
                const auto& st = stations[app.activeStation()];
                if (!st.latest_scan_utc.empty())
                    ImGui::Text("Latest scan: %s", st.latest_scan_utc.c_str());
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Alerts")) {
            state.activeDockTab = ContextDockTab::Alerts;
            ImGui::Checkbox("Overlays", &app.m_warningOptions.enabled);
            ImGui::Checkbox("Warnings", &app.m_warningOptions.showWarnings);
            ImGui::SameLine();
            ImGui::Checkbox("Watches", &app.m_warningOptions.showWatches);
            ImGui::SameLine();
            ImGui::Checkbox("Statements", &app.m_warningOptions.showStatements);
            ImGui::Separator();
            if (warnings.empty()) {
                ImGui::TextDisabled("No alert polygons loaded.");
            } else {
                for (size_t i = 0; i < warnings.size(); ++i) {
                    ImGui::TextWrapped("%s", warnings[i].headline.c_str());
                    ImGui::Separator();
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Layers")) {
            state.activeDockTab = ContextDockTab::Layers;
            int basemapIdx = (int)app.basemap().style();
            const char* basemapLabels[] = {"Relief", "Ops Dark", "Satellite", "Satellite Hybrid"};
            if (ImGui::Combo("Basemap", &basemapIdx, basemapLabels, IM_ARRAYSIZE(basemapLabels)))
                app.basemap().setStyle((BasemapStyle)basemapIdx);
            float rasterOpacity = app.basemap().rasterOpacity();
            if (ImGui::SliderFloat("Raster", &rasterOpacity, 0.0f, 1.0f, "%.2f"))
                app.basemap().setRasterOpacity(rasterOpacity);
            float overlayOpacity = app.basemap().overlayOpacity();
            if (ImGui::SliderFloat("Overlay", &overlayOpacity, 0.0f, 1.0f, "%.2f"))
                app.basemap().setOverlayOpacity(overlayOpacity);
            bool grid = app.basemap().showGrid();
            if (ImGui::Checkbox("Grid", &grid))
                app.basemap().setShowGrid(grid);
            bool statesOn = app.basemap().showStateLines();
            if (ImGui::Checkbox("State Lines", &statesOn))
                app.basemap().setShowStateLines(statesOn);
            bool citiesOn = app.basemap().showCityLabels();
            if (ImGui::Checkbox("Cities", &citiesOn))
                app.basemap().setShowCityLabels(citiesOn);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Assets")) {
            state.activeDockTab = ContextDockTab::Assets;
            int profileIdx = (int)app.requestedPerformanceProfile();
            const char* profileLabels[] = {"Auto", "Quality", "Balanced", "Performance"};
            if (ImGui::Combo("Profile", &profileIdx, profileLabels, IM_ARRAYSIZE(profileLabels)))
                app.setPerformanceProfile((PerformanceProfile)profileIdx);
            ImGui::TextDisabled("Effective: %s", performanceProfileLabel(app.effectivePerformanceProfile()));
            bool showExperimental = app.showExperimentalSites();
            if (ImGui::Checkbox("Experimental Sites", &showExperimental))
                app.setShowExperimentalSites(showExperimental);
            ImGui::TextWrapped("%s", app.priorityStatus().c_str());
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Session")) {
            state.activeDockTab = ContextDockTab::Session;
            ImGui::BulletText("Solo Live");
            ImGui::BulletText("Dual Linked");
            ImGui::BulletText("Tornado Interrogate");
            ImGui::BulletText("Hail Interrogate");
            ImGui::BulletText("Archive Review");
            ImGui::BulletText("3D Inspect");
            ImGui::BulletText("Cross Section");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    endFixedWindow();
}

void renderTimeDeck(App& app, WorkstationState& state, const ShellRegions& regions) {
    beginFixedWindow("##c3_time_deck", regions.timeDeck);
    ImGui::Text("%s", timeDeckModeLabel(state.timeMode));
    ImGui::Separator();

    if (app.m_historicMode) {
        const int total = app.m_historic.numFrames();
        int frame = app.m_historic.currentFrame();
        if (ImGui::Button(app.m_historic.playing() ? "Pause" : "Play"))
            app.m_historic.togglePlay();
        ImGui::SameLine();
        if (ImGui::Button("Prev"))
            app.m_historic.setFrame(std::max(0, frame - 1));
        ImGui::SameLine();
        if (ImGui::Button("Next"))
            app.m_historic.setFrame(std::min(total - 1, frame + 1));
        ImGui::SameLine();
        ImGui::Text("%s", app.m_historic.currentLabel().c_str());
        if (ImGui::SliderInt("##historic_frame", &frame, 0, std::max(0, total - 1), "Frame %d"))
            app.m_historic.setFrame(frame);
    } else {
        bool liveLoop = app.liveLoopEnabled();
        if (ImGui::Checkbox("Loop", &liveLoop))
            app.setLiveLoopEnabled(liveLoop);
        ImGui::SameLine();
        if (ImGui::Button(app.liveLoopPlaying() ? "Pause" : "Play"))
            app.toggleLiveLoopPlayback();
        ImGui::SameLine();
        if (ImGui::Button("Live"))
            app.setLiveLoopPlaybackFrame(std::max(0, app.liveLoopAvailableFrames() - 1));
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
            app.clearLiveLoop();
        ImGui::SameLine();
        int frames = app.liveLoopLength();
        ImGui::SetNextItemWidth(110.0f);
        if (ImGui::SliderInt("Frames", &frames, 1, app.liveLoopMaxFrames()))
            app.setLiveLoopLength(frames);
        ImGui::SameLine();
        float speed = app.liveLoopSpeed();
        ImGui::SetNextItemWidth(110.0f);
        if (ImGui::SliderFloat("FPS", &speed, 1.0f, 15.0f, "%.0f"))
            app.setLiveLoopSpeed(speed);

        drawTimeline(app, "##c3_timeline");
        ImGui::Text("Ready %d / %d", app.liveLoopAvailableFrames(), app.liveLoopLength());
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::Text("Downloads %d / %d", app.liveLoopBackfillFetchCompleted(), app.liveLoopBackfillFetchTotal());
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::Text("Rendering %d", app.liveLoopBackfillPendingFrames());
    }

    endFixedWindow();
}

void applyKeyboardShortcuts(App& app) {
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    for (int i = 0; i < (int)Product::COUNT; ++i) {
        if (ImGui::IsKeyPressed((ImGuiKey)(ImGuiKey_1 + i)))
            app.setProduct(i);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) app.prevProduct();
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) app.nextProduct();
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) app.nextTilt();
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) app.prevTilt();
    if (ImGui::IsKeyPressed(ImGuiKey_V)) app.toggle3D();
    if (ImGui::IsKeyPressed(ImGuiKey_X)) app.toggleCrossSection();
    if (ImGui::IsKeyPressed(ImGuiKey_A)) app.toggleShowAll();
    if (ImGui::IsKeyPressed(ImGuiKey_R)) app.refreshData();
    if (ImGui::IsKeyPressed(ImGuiKey_S)) app.toggleSRV();
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) resetConusView(app);
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) app.setAutoTrackStation(true);
}

} // namespace

void init() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(8, 8);
    style.WindowPadding = ImVec2(12, 12);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.050f, 0.055f, 0.065f, 0.96f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.070f, 0.080f, 0.100f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.090f, 0.110f, 0.145f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.095f, 0.110f, 0.135f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.130f, 0.155f, 0.190f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.110f, 0.130f, 0.165f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.170f, 0.210f, 0.280f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.200f, 0.255f, 0.350f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.120f, 0.145f, 0.185f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.180f, 0.225f, 0.300f, 1.0f);
    colors[ImGuiCol_Tab] = ImVec4(0.090f, 0.102f, 0.124f, 1.0f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.170f, 0.210f, 0.280f, 1.0f);
}

void render(App& app) {
    g_uiWantsMouseCapture = false;
    static WorkstationState state;
    syncWorkstationStateFromApp(app, state);

    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    const auto stations = app.stations();
    const auto warnings = app.currentWarnings();
    const ShellRegions regions = computeRegions(mainViewport, state.contextDockOpen);

    drawCanvas(app, regions, mainViewport);
    renderTopBar(app, state, regions, stations, warnings);
    renderRail(app, state, regions);
    renderDock(app, state, regions, stations, warnings);
    renderTimeDeck(app, state, regions);
    applyKeyboardShortcuts(app);
}

void shutdown() {
}

bool wantsMouseCapture() {
    return g_uiWantsMouseCapture;
}

} // namespace ui
