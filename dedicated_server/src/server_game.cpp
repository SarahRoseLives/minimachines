#include "server_game.h"
#include "core/map_serializer.h"
#include "core/collision.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace fs = std::filesystem;

namespace mm {

using net::PacketHeader;
using net::PacketType;

static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

bool ServerGame::init(int port, int maxPlayers, const std::string& mapPath) {
    m_maxPlayers = maxPlayers;
    m_players.resize(maxPlayers);
    for (int i = 0; i < maxPlayers; ++i) {
        m_players[i].index = i;
        m_players[i].connected = false;
    }

    if (!MapSerializer::loadFromFile(m_map, mapPath)) {
        fprintf(stderr, "Failed to load map: %s\n", mapPath.c_str());
        return false;
    }
    m_mapName = m_map.name();
    m_mapData = readFile(mapPath);
    m_mapPath = mapPath;
    m_mapModTime = fs::last_write_time(mapPath);

    fprintf(stderr, "Map loaded: %s (%dx%d, ts=%d, spawns=%d, checkpoints=%d)\n",
            m_mapName.c_str(), m_map.width(), m_map.height(), m_map.tileSize(),
            (int)m_map.spawns().size(), (int)m_map.checkpoints().size());

    raceInit(m_race, m_map, maxPlayers);
    m_race.state = RaceState::Waiting;

    ENetAddress addr;
    addr.host = ENET_HOST_ANY;
    addr.port = port;
    m_host = enet_host_create(&addr, maxPlayers, 2, 0, 0);
    if (!m_host) {
        fprintf(stderr, "Failed to create ENet host on port %d\n", port);
        return false;
    }

    m_running = true;
    fprintf(stderr, "Dedicated server started on port %d, map: %s (waiting for players)\n", port, m_mapName.c_str());
    return true;
}

void ServerGame::shutdown() {
    if (m_host) {
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
    m_running = false;
}

void ServerGame::onConnect(ENetPeer* peer) {
    int slot = -1;
    for (int i = 0; i < m_maxPlayers; ++i) {
        if (!m_players[i].connected) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        fprintf(stderr, "Server full, rejecting connection\n");
        enet_peer_disconnect_now(peer, 0);
        return;
    }

    auto& p = m_players[slot];
    p.peer = peer;
    p.connected = true;
    snprintf(p.name, sizeof(p.name), "Player%d", slot);
    peer->data = reinterpret_cast<void*>(static_cast<intptr_t>(slot));

    float ts = m_map.tileSize();
    float spawnX = m_map.width() / 2.0f * ts;
    float spawnY = m_map.height() / 2.0f * ts;
    float spawnAngle = 0.0f;

    if (!m_map.spawns().empty()) {
        if (slot < static_cast<int>(m_map.spawns().size())) {
            auto& s = m_map.spawns()[slot];
            spawnX = (s.x + 0.5f) * ts;
            spawnY = (s.y + 0.5f) * ts;
            spawnAngle = s.angle;
        } else {
            auto& s = m_map.spawns()[0];
            float cosA = std::cos(s.angle);
            float sinA = std::sin(s.angle);
            float depth = slot * 20.0f;
            float side = ((slot % 2) == 0 ? -1.0f : 1.0f) * 15.0f;
            spawnX = (s.x + 0.5f) * ts - cosA * depth + (-sinA) * side;
            spawnY = (s.y + 0.5f) * ts - sinA * depth + cosA * side;
            spawnAngle = s.angle;
        }
    }

    p.car.x = spawnX;
    p.car.y = spawnY;
    p.car.heading = spawnAngle;
    p.lastInput = {};
    m_playerCountChanged = true;

    PacketHeader hdr;
    hdr.type = PacketType::S2C_MAP_DATA;
    hdr.playerIndex = slot;
    hdr.sequence = 0;

    std::vector<uint8_t> buf(sizeof(PacketHeader) + m_mapData.size());
    memcpy(buf.data(), &hdr, sizeof(PacketHeader));
    memcpy(buf.data() + sizeof(PacketHeader), m_mapData.data(), m_mapData.size());

    ENetPacket* pkt = enet_packet_create(buf.data(), buf.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, net::CHANNEL_RELIABLE, pkt);

    fprintf(stderr, "Player %d connected: %s\n", slot, p.name);
}

void ServerGame::onDisconnect(ENetPeer* peer) {
    int idx = static_cast<int>(reinterpret_cast<intptr_t>(peer->data));
    if (idx >= 0 && idx < m_maxPlayers) {
        m_players[idx].connected = false;
        m_players[idx].peer = nullptr;
        m_playerCountChanged = true;
        fprintf(stderr, "Player %d disconnected\n", idx);
    }
}

void ServerGame::onReceive(ENetPeer* peer, const uint8_t* data, size_t size) {
    if (size < sizeof(PacketHeader)) return;
    PacketHeader hdr;
    memcpy(&hdr, data, sizeof(PacketHeader));

    int idx = static_cast<int>(reinterpret_cast<intptr_t>(peer->data));
    if (idx < 0 || idx >= m_maxPlayers) return;

    if (hdr.type == PacketType::C2S_INPUT && size >= sizeof(PacketHeader) + sizeof(net::InputPacket)) {
        net::InputPacket inp;
        memcpy(&inp, data + sizeof(PacketHeader), sizeof(net::InputPacket));
        m_players[idx].lastInput = net::unpackInput(inp);
    } else if (hdr.type == PacketType::C2S_JOIN && size >= sizeof(PacketHeader) + sizeof(net::JoinPacket)) {
        net::JoinPacket jp;
        memcpy(&jp, data + sizeof(PacketHeader), sizeof(net::JoinPacket));
        snprintf(m_players[idx].name, sizeof(m_players[idx].name), "%s", jp.name);
    }
}

void ServerGame::update(float dt) {
    ENetEvent event;
    while (enet_host_service(m_host, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            onConnect(event.peer);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            onDisconnect(event.peer);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            onReceive(event.peer, event.packet->data, event.packet->dataLength);
            enet_packet_destroy(event.packet);
            break;
        default:
            break;
        }
    }

    int connected = getPlayerCount();

    if (connected < 2) {
        if (m_race.state != RaceState::Waiting) {
            m_race.state = RaceState::Waiting;
            m_race.countdown = 0.0f;
            m_race.raceTime = 0.0f;
            m_race.finishedCount = 0;
            for (auto& r : m_race.racers) {
                r.currentCheckpoint = 0;
                r.currentLap = 0;
                r.finished = false;
                r.finishTime = 0.0f;
            }
            fprintf(stderr, "Not enough players, waiting...\n");
        }
        sendState();
        return;
    }

    if (m_race.state == RaceState::Waiting && connected >= 2) {
        m_race.state = RaceState::Countdown;
        m_race.countdown = 3.0f;
        fprintf(stderr, "Enough players, starting countdown\n");
    }

    std::vector<CarPosition> carPositions;
    for (int i = 0; i < m_maxPlayers; ++i) {
        CarPosition cp;
        cp.x = m_players[i].car.x;
        cp.y = m_players[i].car.y;
        cp.prevX = cp.x;
        cp.prevY = cp.y;
        carPositions.push_back(cp);
    }

    for (int i = 0; i < m_maxPlayers; ++i) {
        if (!m_players[i].connected) continue;
        if (m_race.state == RaceState::Racing) {
            carUpdate(m_players[i].car, m_players[i].lastInput, m_carCfg, m_map, dt);
        } else if (m_race.state == RaceState::Countdown) {
            carUpdate(m_players[i].car, PlayerInput{}, m_carCfg, m_map, dt);
        }
        carPositions[i].x = m_players[i].car.x;
        carPositions[i].y = m_players[i].car.y;
    }

    raceUpdate(m_race, carPositions, m_map, dt);
}

void ServerGame::sendState() {
    std::vector<CarState> cars;
    std::vector<int> indices;
    for (int i = 0; i < m_maxPlayers; ++i) {
        if (m_players[i].connected) {
            cars.push_back(m_players[i].car);
            indices.push_back(i);
        }
    }

    if (cars.empty()) return;

    net::StatePacket state = net::packState(cars, m_race, indices);

    PacketHeader hdr;
    hdr.type = PacketType::S2C_STATE;
    hdr.playerIndex = 0;
    hdr.sequence = 0;

    uint8_t buf[sizeof(PacketHeader) + sizeof(net::StatePacket)];
    memcpy(buf, &hdr, sizeof(PacketHeader));
    memcpy(buf + sizeof(PacketHeader), &state, sizeof(net::StatePacket));

    ENetPacket* pkt = enet_packet_create(buf, sizeof(buf), 0);
    enet_host_broadcast(m_host, net::CHANNEL_UNRELIABLE, pkt);
}

int ServerGame::getPlayerCount() const {
    int count = 0;
    for (int i = 0; i < m_maxPlayers; ++i)
        if (m_players[i].connected) count++;
    return count;
}

bool ServerGame::checkMapChanged() {
    try {
        auto modTime = fs::last_write_time(m_mapPath);
        return modTime != m_mapModTime;
    } catch (...) {
        return false;
    }
}

void ServerGame::reloadMap() {
    MapData loaded;
    if (!MapSerializer::loadFromFile(loaded, m_mapPath)) {
        fprintf(stderr, "Failed to reload map: %s\n", m_mapPath.c_str());
        return;
    }
    m_map = std::move(loaded);
    m_mapName = m_map.name();
    m_mapData = readFile(m_mapPath);
    m_mapModTime = fs::last_write_time(m_mapPath);

    raceInit(m_race, m_map, m_maxPlayers);
    m_race.state = RaceState::Waiting;

    for (int i = 0; i < m_maxPlayers; ++i) {
        if (m_players[i].connected) {
            float ts = m_map.tileSize();
            if (!m_map.spawns().empty()) {
                if (i < static_cast<int>(m_map.spawns().size())) {
                    auto& s = m_map.spawns()[i];
                    m_players[i].car.x = (s.x + 0.5f) * ts;
                    m_players[i].car.y = (s.y + 0.5f) * ts;
                    m_players[i].car.heading = s.angle;
                } else {
                    auto& s = m_map.spawns()[0];
                    float cosA = std::cos(s.angle);
                    float sinA = std::sin(s.angle);
                    float depth = i * 20.0f;
                    float side = ((i % 2) == 0 ? -1.0f : 1.0f) * 15.0f;
                    m_players[i].car.x = (s.x + 0.5f) * ts - cosA * depth + (-sinA) * side;
                    m_players[i].car.y = (s.y + 0.5f) * ts - sinA * depth + cosA * side;
                    m_players[i].car.heading = s.angle;
                }
            }
            m_players[i].car.vx = 0;
            m_players[i].car.vy = 0;
            m_players[i].car.speed = 0;
        }
    }

    fprintf(stderr, "Map reloaded: %s\n", m_mapName.c_str());
}

void ServerGame::broadcastReliable(const void* data, size_t size) {
    ENetPacket* pkt = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(m_host, net::CHANNEL_RELIABLE, pkt);
}

void ServerGame::sendReliable(ENetPeer* peer, const void* data, size_t size) {
    ENetPacket* pkt = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, net::CHANNEL_RELIABLE, pkt);
}

} // namespace mm
