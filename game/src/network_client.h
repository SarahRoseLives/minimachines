#pragma once

#include "core/net/packet.h"
#include "core/net/net_serial.h"
#include "core/car.h"
#include "core/race.h"
#include <enet/enet.h>
#include <string>
#include <vector>
#include <functional>

namespace mm {

struct ServerBrowserEntry {
    std::string ip;
    int port;
    std::string mapName;
    int players;
    int maxPlayers;
    std::string mapData;
};

class NetworkClient {
public:
    bool init();
    void shutdown();
    void update();

    bool connect(const std::string& ip, int port);
    void disconnect();
    bool isConnected() const { return m_connected; }

    void sendInput(const PlayerInput& input);

    std::vector<ServerBrowserEntry> queryMaster(const std::string& masterUrl);
    std::string fetchMapData(const std::string& masterUrl, const std::string& ip, int port);

    const std::string& getReceivedMapData() const { return m_receivedMapData; }
    bool hasReceivedMap() const { return m_hasReceivedMap; }
    void clearReceivedMap() { m_hasReceivedMap = false; m_receivedMapData.clear(); }

    const std::vector<CarState>& getCarStates() const { return m_carStates; }
    const RaceData& getRaceData() const { return m_raceData; }
    int getPlayerIndex() const { return m_playerIndex; }

    void setOnMapReceived(std::function<void(const std::string&)> cb) { m_onMapReceived = cb; }
    void setOnState(std::function<void()> cb) { m_onState = cb; }

private:
    ENetHost* m_client = nullptr;
    ENetPeer* m_peer = nullptr;
    bool m_connected = false;
    int m_playerIndex = 0;

    std::string m_receivedMapData;
    bool m_hasReceivedMap = false;

    std::vector<CarState> m_carStates;
    RaceData m_raceData;

    std::function<void(const std::string&)> m_onMapReceived;
    std::function<void()> m_onState;
};

} // namespace mm
