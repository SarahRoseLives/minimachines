#pragma once

#include "core/car.h"
#include "core/race.h"
#include "core/map_data.h"
#include "core/net/packet.h"
#include "core/net/net_serial.h"
#include <enet/enet.h>
#include <string>
#include <vector>

namespace mm {

struct ServerPlayer {
    ENetPeer* peer = nullptr;
    uint8_t index = 0;
    char name[32] = {};
    CarState car;
    PlayerInput lastInput;
    bool connected = false;
};

class ServerGame {
public:
    bool init(int port, int maxPlayers, const std::string& mapPath);
    void shutdown();
    void update(float dt);
    void sendState();

    void onConnect(ENetPeer* peer);
    void onDisconnect(ENetPeer* peer);
    void onReceive(ENetPeer* peer, const uint8_t* data, size_t size);

    bool isRunning() const { return m_running; }
    const std::string& getMapName() const { return m_mapName; }
    const std::string& getMapData() const { return m_mapData; }
    int getPlayerCount() const;
    int getMaxPlayers() const { return m_maxPlayers; }

private:
    void broadcastReliable(const void* data, size_t size);
    void sendReliable(ENetPeer* peer, const void* data, size_t size);

    bool m_running = false;
    int m_maxPlayers = 8;
    MapData m_map;
    std::string m_mapName;
    std::string m_mapData;
    RaceData m_race;
    CarConfig m_carCfg;
    std::vector<ServerPlayer> m_players;
    ENetHost* m_host = nullptr;
};

} // namespace mm
