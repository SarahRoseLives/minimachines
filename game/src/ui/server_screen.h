#pragma once

#include "ui_screen.h"
#include "core/map_data.h"
#include "core/net/net_serial.h"
#include "network_client.h"
#include <string>
#include <vector>
#include <functional>

namespace mm {

class ServerScreen : public UIScreen {
public:
    using ConnectCallback = std::function<void(const std::string&, int)>;
    using BackCallback = std::function<void()>;

    void setCallbacks(ConnectCallback connect, BackCallback back);
    void setMasterUrl(const std::string& url);

    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(UIContext& ui) override;

    void refreshServers(NetworkClient& net);

private:
    std::vector<ServerBrowserEntry> m_servers;
    int m_selectedServer = 0;
    int m_focusIndex = 0;

    char m_masterUrl[128] = "http://192.168.1.240:8080";
    char m_directIp[64] = "192.168.1.240";
    char m_directPort[8] = "27015";

    ConnectCallback m_connect;
    BackCallback m_back;

    UIInput m_input;
    std::string m_status;
    float m_statusTimer = 0.0f;

    MapData m_previewMap;
    bool m_hasPreview = false;

    void renderMapPreview(UIContext& ui, int x, int y, int w, int h);
};

} // namespace mm
