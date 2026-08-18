#include "server_screen.h"
#include "core/map_serializer.h"
#include "core/tiles.h"
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mm {

static std::string getExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string path(buf);
    auto pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(0, pos) : ".";
#elif defined(__ANDROID__)
    return ".";
#else
    return ".";
#endif
}

void ServerScreen::setCallbacks(ConnectCallback connect, BackCallback back) {
    m_connect = connect;
    m_back = back;
}

void ServerScreen::setMasterUrl(const std::string& url) {
    snprintf(m_masterUrl, sizeof(m_masterUrl), "%s", url.c_str());
}

void ServerScreen::refreshServers(NetworkClient& net) {
    m_servers = net.queryMaster(m_masterUrl);
    m_selectedServer = 0;
    m_hasPreview = false;
    m_status = "Refreshed";
    m_statusTimer = 2.0f;
}

void ServerScreen::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.scancode) {
        case SDL_SCANCODE_TAB:
            m_focusIndex = (m_focusIndex + 1) % 5;
            break;
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
            m_input.action = UIAction::Up; break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            m_input.action = UIAction::Down; break;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
            m_input.action = UIAction::Back; break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
            m_input.action = UIAction::Confirm; break;
        case SDL_SCANCODE_RETURN:
            m_input.action = UIAction::Confirm; break;
        case SDL_SCANCODE_ESCAPE:
            m_input.action = UIAction::Back; break;
        default: break;
        }
    }

    if (e.type == SDL_TEXTINPUT) {
        m_input.textChar = e.text.text[0];
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE) {
        m_input.textChar = '\b';
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        m_input.mouseClicked = true;

    if (e.type == SDL_MOUSEMOTION) {
        m_input.mouseX = e.motion.x;
        m_input.mouseY = e.motion.y;
    }
}

void ServerScreen::update(float dt) {
    if (m_statusTimer > 0.0f)
        m_statusTimer -= dt;
}

void ServerScreen::render(UIContext& ui) {
    SDL_GetMouseState(&m_input.mouseX, &m_input.mouseY);

    int sw = ui.screenW;
    int sh = ui.screenH;
    int panelW = 750;
    int panelH = 550;
    int px = (sw - panelW) / 2;
    int py = (sh - panelH) / 2;
    int cy = py;

    uiPanel(ui, px - 15, py - 15, panelW + 30, panelH + 30, UICol::bg);
    ui.drawTextCentered(ui.fontLarge, "SERVER BROWSER", sw / 2, cy, UICol::title);
    cy += 45;

    uiLabel(ui, "Master Server:", px, cy, UICol::textDim);
    cy += 25;
    uiTextInput(ui, m_masterUrl, sizeof(m_masterUrl), px, cy, panelW - 110, 30, m_focusIndex == 0, m_input);
    if (uiButton(ui, "Refresh", px + panelW - 100, cy, 100, 30, false, m_input)) {
        if (m_connect) m_connect("__refresh__", 0);
    }
    cy += 40;

    int listW = panelW / 2 - 10;
    int listH = 220;
    int previewX = px + listW + 20;

    std::vector<std::string> serverNames;
    for (auto& s : m_servers) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s:%d\n%s  %d/%d", s.ip.c_str(), s.port,
                 s.mapName.c_str(), s.players, s.maxPlayers);
        serverNames.push_back(buf);
    }

    uiListBox(ui, serverNames, m_selectedServer, px, cy, listW, listH, m_focusIndex == 1, m_input);

    if (m_selectedServer >= 0 && m_selectedServer < static_cast<int>(m_servers.size())) {
        auto& sel = m_servers[m_selectedServer];
        if (!sel.mapData.empty() && !m_hasPreview) {
            std::string tmpPath = getExeDir() + "/_preview_map.json";
            FILE* f = fopen(tmpPath.c_str(), "wb");
            if (f) {
                fwrite(sel.mapData.data(), 1, sel.mapData.size(), f);
                fclose(f);
                if (MapSerializer::loadFromFile(m_previewMap, tmpPath))
                    m_hasPreview = true;
                remove(tmpPath.c_str());
            }
        }
    }

    if (m_hasPreview) {
        renderMapPreview(ui, previewX, cy, listW, listH);
    } else {
        uiPanel(ui, previewX, cy, listW, listH, UICol::listBg);
        ui.drawTextCentered(ui.font, "No preview", previewX + listW / 2, cy + listH / 2 - 9, UICol::textDim);
    }
    cy += listH + 15;

    uiLabel(ui, "Direct Connect:", px, cy, UICol::textDim);
    cy += 25;
    uiLabel(ui, "IP:", px, cy, UICol::text);
    uiTextInput(ui, m_directIp, sizeof(m_directIp), px + 30, cy, 200, 30, m_focusIndex == 2, m_input);
    uiLabel(ui, "Port:", px + 240, cy, UICol::text);
    uiTextInput(ui, m_directPort, sizeof(m_directPort), px + 280, cy, 100, 30, m_focusIndex == 3, m_input);

    if (uiButton(ui, "Connect", px + panelW - 210, cy, 100, 30, m_focusIndex == 4, m_input)) {
        if (m_connect) m_connect(m_directIp, atoi(m_directPort));
    }
    if (uiButton(ui, "Back", px + panelW - 100, cy, 100, 30, false, m_input)) {
        if (m_back) m_back();
    }
    cy += 40;

    if (m_statusTimer > 0.0f) {
        ui.drawTextCentered(ui.font, m_status.c_str(), sw / 2, cy, UICol::accent);
    }

    if (m_input.action == UIAction::Down) m_focusIndex = (m_focusIndex + 1) % 5;
    if (m_input.action == UIAction::Up) m_focusIndex = (m_focusIndex + 4) % 5;
    if (m_input.action == UIAction::Back && m_focusIndex == 0) {
        if (m_back) m_back();
    }

    m_input.mouseClicked = false;
    m_input.action = UIAction::None;
}

void ServerScreen::renderMapPreview(UIContext& ui, int x, int y, int w, int h) {
    uiPanel(ui, x, y, w, h, UICol::listBg);

    int mapW = m_previewMap.width();
    int mapH = m_previewMap.height();
    if (mapW == 0 || mapH == 0) return;

    float scale = std::min((float)(w - 4) / mapW, (float)(h - 4) / mapH);
    int drawW = (int)(mapW * scale);
    int drawH = (int)(mapH * scale);
    int ox = x + (w - drawW) / 2;
    int oy = y + (h - drawH) / 2;

    int step = std::max(1, (int)(1.0f / scale));
    for (int ty = 0; ty < mapH; ty += step) {
        for (int tx = 0; tx < mapW; tx += step) {
            TileType ground = m_previewMap.getGround(tx, ty);
            TileType obj = m_previewMap.getObject(tx, ty);
            TileType display = (obj != TileType::Empty) ? obj : ground;

            const TileInfo* info = nullptr;
            for (auto& ti : allTileInfos()) {
                if (ti.type == display) { info = &ti; break; }
            }
            if (!info) continue;

            SDL_SetRenderDrawColor(ui.renderer, info->r, info->g, info->b, 200);
            int px = ox + (int)(tx * scale);
            int py = oy + (int)(ty * scale);
            int ps = std::max(1, (int)(step * scale));
            SDL_Rect dst = {px, py, ps, ps};
            SDL_RenderFillRect(ui.renderer, &dst);
        }
    }
}

} // namespace mm
