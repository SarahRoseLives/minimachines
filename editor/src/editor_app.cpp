#include "editor_app.h"
#include "core/map_serializer.h"
#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

namespace mm {

static const TileInfo* findTileInfo(TileType type) {
    for (auto& info : allTileInfos()) {
        if (info.type == type) return &info;
    }
    return nullptr;
}

bool EditorApp::init(SDL_Renderer* renderer) {
    m_renderer = renderer;
    m_map = MapData(32, 32, 32);
    m_camera.offsetX = -100.0f;
    m_camera.offsetY = -100.0f;
    m_camera.zoom = 2.0f;
    recreateMapTexture(800, 600);
    return true;
}

void EditorApp::shutdown() {
    if (m_mapTexture) {
        SDL_DestroyTexture(m_mapTexture);
        m_mapTexture = nullptr;
    }
}

void EditorApp::recreateMapTexture(int w, int h) {
    if (w <= 0 || h <= 0) return;
    if (m_texW == w && m_texH == h) return;
    if (m_mapTexture) SDL_DestroyTexture(m_mapTexture);
    m_mapTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET, w, h);
    m_texW = w;
    m_texH = h;
}

void EditorApp::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        m_quit = true;
    }
}

void EditorApp::update() {
    if (m_statusTimer > 0.0f)
        m_statusTimer -= 1.0f / 60.0f;
}

void EditorApp::showStatus(const char* msg) {
    m_statusMsg = msg;
    m_statusTimer = 4.0f;
}

static std::string getExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string path(buf);
    auto pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(0, pos) : ".";
#else
    return ".";
#endif
}

static bool showSaveDialog(std::string& outPath) {
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "json";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn)) {
        outPath = buf;
        return true;
    }
    return false;
#else
    outPath = "map.json";
    return true;
#endif
}

static bool showOpenDialog(std::string& outPath) {
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) {
        outPath = buf;
        return true;
    }
    return false;
#else
    outPath = "map.json";
    return true;
#endif
}

void EditorApp::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
    SDL_RenderClear(renderer);

    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (ImGui::IsKeyPressed(ImGuiKey_P)) m_toolState.currentTool = ToolType::Paint;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) m_toolState.currentTool = ToolType::Erase;
        if (ImGui::IsKeyPressed(ImGuiKey_S)) m_toolState.currentTool = ToolType::StampSpawn;
        if (ImGui::IsKeyPressed(ImGuiKey_C)) m_toolState.currentTool = ToolType::StampCheckpoint;
        if (ImGui::IsKeyPressed(ImGuiKey_D)) m_toolState.currentTool = ToolType::DeleteEntity;
        if (ImGui::IsKeyPressed(ImGuiKey_G)) m_gridEnabled = !m_gridEnabled;
        if (ImGui::IsKeyPressed(ImGuiKey_1)) m_toolState.activeLayer = Layer::Ground;
        if (ImGui::IsKeyPressed(ImGuiKey_2)) m_toolState.activeLayer = Layer::Objects;
    }

    drawMenuBar();
    beginMapWindow();
    renderGridToTexture();
    endMapWindow();
    if (m_showPalette)
        m_palette.draw(m_toolState);
    if (m_showProperties) {
        if (m_properties.draw(m_map))
            m_currentFile.clear();
    }

    if (m_statusTimer > 0.0f) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                       ImGui::GetIO().DisplaySize.y - 60),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        float alpha = m_statusTimer < 1.0f ? m_statusTimer : 1.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::Begin("##status", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
        ImGui::Text("%s", m_statusMsg.c_str());
        ImGui::End();
        ImGui::PopStyleVar();
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void EditorApp::renderGridToTexture() {
    SDL_SetRenderTarget(m_renderer, m_mapTexture);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 50, 255);
    SDL_RenderClear(m_renderer);

    int ts = m_map.tileSize();
    float zoom = m_camera.zoom;

    int startX = static_cast<int>(std::floor(m_camera.offsetX / ts)) - 1;
    int startY = static_cast<int>(std::floor(m_camera.offsetY / ts)) - 1;
    int endX = static_cast<int>(std::ceil((m_camera.offsetX + m_texW / zoom) / ts)) + 1;
    int endY = static_cast<int>(std::ceil((m_camera.offsetY + m_texH / zoom) / ts)) + 1;

    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    if (endX > m_map.width()) endX = m_map.width();
    if (endY > m_map.height()) endY = m_map.height();

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            TileType ground = m_map.getGround(x, y);
            TileType obj = m_map.getObject(x, y);
            TileType display = (obj != TileType::Empty) ? obj : ground;
            drawTile(m_renderer, x, y, display);
        }
    }

    if (m_gridEnabled) {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 80);
        for (int x = startX; x <= endX; ++x) {
            SDL_Point sp = m_camera.worldToScreen(x, startY, ts);
            SDL_Point ep = m_camera.worldToScreen(x, endY, ts);
            SDL_RenderDrawLine(m_renderer, sp.x, sp.y, ep.x, ep.y);
        }
        for (int y = startY; y <= endY; ++y) {
            SDL_Point sp = m_camera.worldToScreen(startX, y, ts);
            SDL_Point ep = m_camera.worldToScreen(endX, y, ts);
            SDL_RenderDrawLine(m_renderer, sp.x, sp.y, ep.x, ep.y);
        }
    }

    SDL_Rect mapBorder;
    mapBorder.x = m_camera.worldToScreen(0, 0, ts).x;
    mapBorder.y = m_camera.worldToScreen(0, 0, ts).y;
    mapBorder.w = static_cast<int>(m_map.width() * ts * zoom);
    mapBorder.h = static_cast<int>(m_map.height() * ts * zoom);
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 120);
    SDL_RenderDrawRect(m_renderer, &mapBorder);

    drawSpawns(m_renderer);
    drawCheckpoints(m_renderer);
    drawSpawnAim(m_renderer);
    drawCheckpointAim(m_renderer);

    if (m_hoverTileX >= 0 && m_hoverTileY >= 0 &&
        (m_toolState.currentTool == ToolType::Paint || m_toolState.currentTool == ToolType::Erase) &&
        !m_toolState.rectFilling && !m_toolState.painting) {
        const TileInfo* info = findTileInfo(m_toolState.selectedTile);
        if (info && m_toolState.currentTool == ToolType::Paint) {
            int half = m_toolState.brushSize / 2;
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            for (int dy = -half; dy < m_toolState.brushSize - half; ++dy) {
                for (int dx = -half; dx < m_toolState.brushSize - half; ++dx) {
                    int tx = m_hoverTileX + dx;
                    int ty = m_hoverTileY + dy;
                    if (!m_map.inBounds(tx, ty)) continue;
                    SDL_Point sp = m_camera.worldToScreen(tx, ty, ts);
                    int w = static_cast<int>(ts * zoom);
                    SDL_Rect dst = {sp.x, sp.y, w, w};
                    SDL_SetRenderDrawColor(m_renderer, info->r, info->g, info->b, 80);
                    SDL_RenderFillRect(m_renderer, &dst);
                    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 120);
                    SDL_RenderDrawRect(m_renderer, &dst);
                }
            }
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        } else if (m_toolState.currentTool == ToolType::Erase) {
            int half = m_toolState.brushSize / 2;
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            for (int dy = -half; dy < m_toolState.brushSize - half; ++dy) {
                for (int dx = -half; dx < m_toolState.brushSize - half; ++dx) {
                    int tx = m_hoverTileX + dx;
                    int ty = m_hoverTileY + dy;
                    if (!m_map.inBounds(tx, ty)) continue;
                    SDL_Point sp = m_camera.worldToScreen(tx, ty, ts);
                    int w = static_cast<int>(ts * zoom);
                    SDL_Rect dst = {sp.x, sp.y, w, w};
                    SDL_SetRenderDrawColor(m_renderer, 255, 50, 50, 60);
                    SDL_RenderFillRect(m_renderer, &dst);
                    SDL_SetRenderDrawColor(m_renderer, 255, 100, 100, 120);
                    SDL_RenderDrawRect(m_renderer, &dst);
                }
            }
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }
    }

    if (m_toolState.rectFilling) {
        const TileInfo* info = findTileInfo(m_toolState.selectedTile);
        if (info) {
            int minX = std::min(m_toolState.rectStartX, m_toolState.rectEndX);
            int maxX = std::max(m_toolState.rectStartX, m_toolState.rectEndX);
            int minY = std::min(m_toolState.rectStartY, m_toolState.rectEndY);
            int maxY = std::max(m_toolState.rectStartY, m_toolState.rectEndY);

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            for (int y = minY; y <= maxY; ++y) {
                for (int x = minX; x <= maxX; ++x) {
                    if (!m_map.inBounds(x, y)) continue;
                    SDL_Point sp = m_camera.worldToScreen(x, y, ts);
                    int w = static_cast<int>(ts * zoom);
                    SDL_Rect dst = {sp.x, sp.y, w, w};
                    SDL_SetRenderDrawColor(m_renderer, info->r, info->g, info->b, 100);
                    SDL_RenderFillRect(m_renderer, &dst);
                    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 150);
                    SDL_RenderDrawRect(m_renderer, &dst);
                }
            }
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        }
    }

    SDL_SetRenderTarget(m_renderer, nullptr);
}

void EditorApp::drawTile(SDL_Renderer* r, int x, int y, TileType type) {
    const TileInfo* info = findTileInfo(type);
    if (!info) return;

    int ts = m_map.tileSize();
    SDL_Point sp = m_camera.worldToScreen(x, y, ts);
    int w = static_cast<int>(ts * m_camera.zoom);
    int h = static_cast<int>(ts * m_camera.zoom);

    SDL_Rect dst = {sp.x, sp.y, w, h};
    SDL_SetRenderDrawColor(r, info->r, info->g, info->b, 255);
    SDL_RenderFillRect(r, &dst);
}

void EditorApp::drawSpawns(SDL_Renderer* r) {
    int ts = m_map.tileSize();
    for (auto& s : m_map.spawns()) {
        SDL_Point sp = m_camera.worldToScreen(s.x, s.y, ts);
        int w = static_cast<int>(ts * m_camera.zoom);
        int cx = sp.x + w / 2;
        int cy = sp.y + w / 2;
        float radius = w * 0.4f;
        float cosA = std::cos(s.angle);
        float sinA = std::sin(s.angle);

        auto rotate = [&](float lx, float ly) -> SDL_Vertex {
            SDL_Vertex v;
            v.position.x = cx + (lx * cosA - ly * sinA);
            v.position.y = cy + (lx * sinA + ly * cosA);
            v.color = {0, 150, 255, 255};
            v.tex_coord = {0, 0};
            return v;
        };

        SDL_Vertex verts[3] = {
            rotate(radius, 0),
            rotate(-radius * 0.7f, -radius * 0.5f),
            rotate(-radius * 0.7f, radius * 0.5f),
        };
        SDL_RenderGeometry(r, nullptr, verts, 3, nullptr, 0);
    }
}

void EditorApp::drawCheckpoints(SDL_Renderer* r) {
    int ts = m_map.tileSize();
    int w = static_cast<int>(ts * m_camera.zoom);

    for (auto& c : m_map.checkpoints()) {
        int dx = std::abs(c.x2 - c.x1);
        int dy = std::abs(c.y2 - c.y1);
        int sx = (c.x1 < c.x2) ? 1 : -1;
        int sy = (c.y1 < c.y2) ? 1 : -1;
        int err = dx - dy;
        int cx = c.x1, cy = c.y1;

        while (true) {
            SDL_Point sp = m_camera.worldToScreen(cx, cy, ts);
            SDL_Rect dst = {sp.x, sp.y, w, w};
            SDL_SetRenderDrawColor(r, 255, 140, 0, 80);
            SDL_RenderFillRect(r, &dst);
            SDL_SetRenderDrawColor(r, 255, 200, 0, 200);
            SDL_RenderDrawRect(r, &dst);

            if (cx == c.x2 && cy == c.y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; cx += sx; }
            if (e2 < dx)  { err += dx; cy += sy; }
        }
    }
}

void EditorApp::drawSpawnAim(SDL_Renderer* r) {
    if (m_toolState.currentTool != ToolType::StampSpawn) return;
    if (!m_toolState.painting) return;
    if (m_toolState.spawnTileX < 0) return;

    int ts = m_map.tileSize();
    SDL_Point sp = m_camera.worldToScreen(m_toolState.spawnTileX, m_toolState.spawnTileY, ts);
    int w = static_cast<int>(ts * m_camera.zoom);
    int cx = sp.x + w / 2;
    int cy = sp.y + w / 2;

    SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
    SDL_RenderDrawLine(r, cx, cy, m_toolState.aimX, m_toolState.aimY);

    float radius = w * 0.4f;
    float angle = std::atan2(m_toolState.aimY - cy, m_toolState.aimX - cx);
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    auto rotate = [&](float lx, float ly) -> SDL_Vertex {
        SDL_Vertex v;
        v.position.x = cx + (lx * cosA - ly * sinA);
        v.position.y = cy + (lx * sinA + ly * cosA);
        v.color = {255, 255, 0, 200};
        v.tex_coord = {0, 0};
        return v;
    };

    SDL_Vertex verts[3] = {
        rotate(radius, 0),
        rotate(-radius * 0.7f, -radius * 0.5f),
        rotate(-radius * 0.7f, radius * 0.5f),
    };
    SDL_RenderGeometry(r, nullptr, verts, 3, nullptr, 0);
}

void EditorApp::drawCheckpointAim(SDL_Renderer* r) {
    if (m_toolState.currentTool != ToolType::StampCheckpoint) return;
    if (!m_toolState.painting) return;
    if (m_toolState.checkTileX < 0) return;

    int ts = m_map.tileSize();
    SDL_Point sp = m_camera.worldToScreen(m_toolState.checkTileX, m_toolState.checkTileY, ts);
    int w = static_cast<int>(ts * m_camera.zoom);
    int cx = sp.x + w / 2;
    int cy = sp.y + w / 2;

    SDL_SetRenderDrawColor(r, 255, 200, 0, 255);
    SDL_RenderDrawLine(r, cx, cy, m_toolState.aimX, m_toolState.aimY);
}

void EditorApp::beginMapWindow() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Map", nullptr, ImGuiWindowFlags_NoScrollbar);

    m_mapFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 1.0f) canvasSize.x = 1.0f;
    if (canvasSize.y < 1.0f) canvasSize.y = 1.0f;

    int texW = static_cast<int>(canvasSize.x);
    int texH = static_cast<int>(canvasSize.y);
    recreateMapTexture(texW, texH);
}

void EditorApp::endMapWindow() {
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 1.0f) canvasSize.x = 1.0f;
    if (canvasSize.y < 1.0f) canvasSize.y = 1.0f;

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)(intptr_t)m_mapTexture, canvasSize);

    drawCheckpointIndices(canvasPos);
    drawMinimap(canvasPos, canvasSize);

    if (ImGui::IsItemHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        int localX = static_cast<int>(mousePos.x - canvasPos.x);
        int localY = static_cast<int>(mousePos.y - canvasPos.y);

        SDL_Point hoverTile = m_camera.screenToWorld(localX, localY, m_map.tileSize());
        m_hoverTileX = hoverTile.x;
        m_hoverTileY = hoverTile.y;

        ImGuiIO& io = ImGui::GetIO();

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            m_camera.beginPan(localX, localY);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
            m_camera.updatePan(localX, localY);
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            m_camera.endPan();

        if (io.MouseWheel != 0.0f)
            m_camera.handleZoom(static_cast<int>(io.MouseWheel), localX, localY);

        bool shiftHeld = io.KeyShift;

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            m_tools.onMouseButton(m_camera, m_map, m_toolState, localX, localY, true, true, shiftHeld);
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            m_tools.onMouseButton(m_camera, m_map, m_toolState, localX, localY, false, true, shiftHeld);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            m_tools.onMouseMove(m_camera, m_map, m_toolState, localX, localY);
    } else {
        m_hoverTileX = -1;
        m_hoverTileY = -1;
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorApp::drawCheckpointIndices(ImVec2 canvasPos) {
    int ts = m_map.tileSize();
    float zoom = m_camera.zoom;
    for (int i = 0; i < static_cast<int>(m_map.checkpoints().size()); ++i) {
        auto& c = m_map.checkpoints()[i];
        float midWorldX = (c.x1 + c.x2 + 1.0f) * 0.5f * ts;
        float midWorldY = (c.y1 + c.y2 + 1.0f) * 0.5f * ts;
        float screenX = canvasPos.x + (midWorldX - m_camera.offsetX) * zoom;
        float screenY = canvasPos.y + (midWorldY - m_camera.offsetY) * zoom;

        char buf[16];
        snprintf(buf, sizeof(buf), "%d", i + 1);
        ImVec2 textSize = ImGui::CalcTextSize(buf);

        ImGui::GetWindowDrawList()->AddText(
            ImVec2(screenX - textSize.x / 2, screenY - textSize.y / 2),
            IM_COL32(0, 0, 0, 255), buf);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(screenX - textSize.x / 2 - 1, screenY - textSize.y / 2 - 1),
            IM_COL32(255, 255, 255, 255), buf);
    }
}

void EditorApp::drawMinimap(ImVec2 canvasPos, ImVec2 canvasSize) {
    int mapW = m_map.width();
    int mapH = m_map.height();
    int minimapSize = 160;
    float scale = (float)minimapSize / std::max(mapW, mapH);
    int drawW = (int)(mapW * scale);
    int drawH = (int)(mapH * scale);

    float margin = 10.0f;
    float ox = canvasPos.x + canvasSize.x - drawW - margin;
    float oy = canvasPos.y + canvasSize.y - drawH - margin;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(ImVec2(ox - 2, oy - 2), ImVec2(ox + drawW + 2, oy + drawH + 2),
                      IM_COL32(0, 0, 0, 180));

    int ts = m_map.tileSize();
    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            TileType ground = m_map.getGround(x, y);
            TileType obj = m_map.getObject(x, y);
            TileType display = (obj != TileType::Empty) ? obj : ground;
            const TileInfo* info = findTileInfo(display);
            if (!info) continue;

            float px = ox + x * scale;
            float py = oy + y * scale;
            ImU32 col = IM_COL32(info->r, info->g, info->b, 220);
            dl->AddRectFilled(ImVec2(px, py), ImVec2(px + std::max(1.0f, scale), py + std::max(1.0f, scale)), col);
        }
    }

    float vx = ox + (m_camera.offsetX / ts) * scale;
    float vy = oy + (m_camera.offsetY / ts) * scale;
    float vw = (canvasSize.x / m_camera.zoom / ts) * scale;
    float vh = (canvasSize.y / m_camera.zoom / ts) * scale;
    dl->AddRect(ImVec2(vx, vy), ImVec2(vx + vw, vy + vh), IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

    ImVec2 mmMin(ox, oy);
    ImVec2 mmMax(ox + drawW, oy + drawH);
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool hovering = mousePos.x >= mmMin.x && mousePos.x <= mmMax.x &&
                    mousePos.y >= mmMin.y && mousePos.y <= mmMax.y;

    if (hovering) {
        dl->AddRect(mmMin, mmMax, IM_COL32(255, 255, 0, 200), 0.0f, 0, 2.0f);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            float localMX = (mousePos.x - ox) / scale;
            float localMY = (mousePos.y - oy) / scale;
            m_camera.offsetX = localMX * ts - canvasSize.x / m_camera.zoom / 2.0f;
            m_camera.offsetY = localMY * ts - canvasSize.y / m_camera.zoom / 2.0f;
        }
    }
}

void EditorApp::drawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                m_map = MapData(32, 32, 32);
                m_currentFile.clear();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) doSave();
            if (ImGui::MenuItem("Save As...")) {
                std::string path;
                if (showSaveDialog(path)) {
                    m_currentFile = path;
                    doSave();
                }
            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                std::string path;
                if (showOpenDialog(path)) {
                    m_currentFile = path;
                    doLoad();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) m_quit = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Map", nullptr, true);
            ImGui::MenuItem("Palette", nullptr, &m_showPalette);
            ImGui::MenuItem("Properties", nullptr, &m_showProperties);
            ImGui::MenuItem("Grid", "G", &m_gridEnabled);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tool")) {
            bool isPaint = m_toolState.currentTool == ToolType::Paint;
            bool isErase = m_toolState.currentTool == ToolType::Erase;
            if (ImGui::MenuItem("Paint", "P", isPaint)) m_toolState.currentTool = ToolType::Paint;
            if (ImGui::MenuItem("Erase", "E", isErase)) m_toolState.currentTool = ToolType::Erase;
            ImGui::Separator();
            bool isGround = m_toolState.activeLayer == Layer::Ground;
            bool isObjects = m_toolState.activeLayer == Layer::Objects;
            if (ImGui::MenuItem("Ground Layer", "1", isGround)) m_toolState.activeLayer = Layer::Ground;
            if (ImGui::MenuItem("Objects Layer", "2", isObjects)) m_toolState.activeLayer = Layer::Objects;
            ImGui::EndMenu();
        }

        ImGui::Separator();
        const char* toolName = "Paint";
        switch (m_toolState.currentTool) {
        case ToolType::Paint: toolName = "Paint"; break;
        case ToolType::Erase: toolName = "Erase"; break;
        case ToolType::StampSpawn: toolName = "Spawn (click+drag to aim)"; break;
        case ToolType::StampCheckpoint: toolName = "Checkpoint (click to place)"; break;
        case ToolType::DeleteEntity: toolName = "Delete Entity (click to remove)"; break;
        }
        const char* layerName = m_toolState.activeLayer == Layer::Ground ? "Ground" : "Objects";
        ImGui::Text("Tool: %s | Layer: %s | Brush: %d", toolName, layerName, m_toolState.brushSize);
        if (m_toolState.roadWallPreset)
            ImGui::SameLine(), ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1), "| Road+Walls");

        if (m_statusTimer > 0.0f) {
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(m_statusMsg.c_str()).x - 20);
            ImGui::TextColored(ImVec4(1, 1, 0.4f, 1), "%s", m_statusMsg.c_str());
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorApp::doSave() {
    if (m_currentFile.empty())
        m_currentFile = getExeDir() + "\\map.json";
    if (MapSerializer::saveToFile(m_map, m_currentFile)) {
        char buf[512];
        snprintf(buf, sizeof(buf), "Saved: %s", m_currentFile.c_str());
        showStatus(buf);
    } else {
        char buf[512];
        snprintf(buf, sizeof(buf), "FAILED to save: %s", m_currentFile.c_str());
        showStatus(buf);
    }
}

void EditorApp::doLoad() {
    if (m_currentFile.empty())
        m_currentFile = getExeDir() + "\\map.json";
    MapData loaded;
    if (MapSerializer::loadFromFile(loaded, m_currentFile)) {
        m_map = std::move(loaded);
        char buf[512];
        snprintf(buf, sizeof(buf), "Loaded: %s", m_currentFile.c_str());
        showStatus(buf);
    } else {
        char buf[512];
        snprintf(buf, sizeof(buf), "FAILED to load: %s", m_currentFile.c_str());
        showStatus(buf);
    }
}

void EditorApp::doNewMap(int w, int h, int tileSize) {
    m_map = MapData(w, h, tileSize);
    m_currentFile.clear();
}

} // namespace mm
