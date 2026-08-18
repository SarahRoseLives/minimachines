#include "network_client.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>

namespace mm {

using net::PacketHeader;
using net::PacketType;
using net::StatePacket;
using net::InputPacket;
using net::packInput;
using net::unpackState;

bool NetworkClient::init() {
    m_client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!m_client) {
        fprintf(stderr, "Failed to create ENet client host\n");
        return false;
    }
    return true;
}

void NetworkClient::shutdown() {
    disconnect();
    if (m_client) {
        enet_host_destroy(m_client);
        m_client = nullptr;
    }
}

bool NetworkClient::connect(const std::string& ip, int port) {
    if (!m_client) return false;

    ENetAddress addr;
    enet_address_set_host(&addr, ip.c_str());
    addr.port = port;

    m_peer = enet_host_connect(m_client, &addr, 2, 0);
    if (!m_peer) {
        fprintf(stderr, "Failed to connect to %s:%d\n", ip.c_str(), port);
        return false;
    }

    ENetEvent event;
    if (enet_host_service(m_client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        m_connected = true;
        fprintf(stderr, "Connected to %s:%d\n", ip.c_str(), port);
        return true;
    }

    enet_peer_reset(m_peer);
    m_peer = nullptr;
    fprintf(stderr, "Connection to %s:%d failed\n", ip.c_str(), port);
    return false;
}

void NetworkClient::disconnect() {
    if (m_peer && m_connected) {
        enet_peer_disconnect(m_peer, 0);
        ENetEvent event;
        while (enet_host_service(m_client, &event, 1000) > 0) {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) break;
        }
        m_connected = false;
        m_peer = nullptr;
    }
}

void NetworkClient::update() {
    if (!m_client) return;

    ENetEvent event;
    while (enet_host_service(m_client, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_DISCONNECT:
            m_connected = false;
            m_peer = nullptr;
            fprintf(stderr, "Disconnected from server\n");
            break;

        case ENET_EVENT_TYPE_RECEIVE: {
            if (event.packet->dataLength < sizeof(PacketHeader)) {
                enet_packet_destroy(event.packet);
                break;
            }

            PacketHeader hdr;
            memcpy(&hdr, event.packet->data, sizeof(PacketHeader));

            if (hdr.type == PacketType::S2C_MAP_DATA) {
                size_t mapSize = event.packet->dataLength - sizeof(PacketHeader);
                m_receivedMapData.assign(
                    reinterpret_cast<const char*>(event.packet->data + sizeof(PacketHeader)),
                    mapSize);
                m_hasReceivedMap = true;
                m_playerIndex = hdr.playerIndex;
                if (m_onMapReceived) m_onMapReceived(m_receivedMapData);
            } else if (hdr.type == PacketType::S2C_STATE) {
                if (event.packet->dataLength >= sizeof(PacketHeader) + sizeof(StatePacket)) {
                    StatePacket state;
                    memcpy(&state, event.packet->data + sizeof(PacketHeader), sizeof(StatePacket));
                    unpackState(state, m_carStates, m_raceData);
                    if (m_onState) m_onState();
                }
            }

            enet_packet_destroy(event.packet);
            break;
        }
        default:
            break;
        }
    }
}

void NetworkClient::sendInput(const PlayerInput& input) {
    if (!m_connected || !m_peer) return;

    PacketHeader hdr;
    hdr.type = PacketType::C2S_INPUT;
    hdr.playerIndex = m_playerIndex;
    hdr.sequence = 0;

    InputPacket inp = packInput(input);

    uint8_t buf[sizeof(PacketHeader) + sizeof(InputPacket)];
    memcpy(buf, &hdr, sizeof(PacketHeader));
    memcpy(buf + sizeof(PacketHeader), &inp, sizeof(InputPacket));

    ENetPacket* pkt = enet_packet_create(buf, sizeof(buf), 0);
    enet_peer_send(m_peer, net::CHANNEL_UNRELIABLE, pkt);
}

std::vector<ServerBrowserEntry> NetworkClient::queryMaster(const std::string& masterUrl) {
    std::vector<ServerBrowserEntry> results;

    httplib::Client cli(masterUrl.c_str());
    cli.set_connection_timeout(3, 0);
    cli.set_read_timeout(3, 0);
    auto res = cli.Get("/servers");
    if (!res) {
        fprintf(stderr, "Master server query failed: no response from %s\n", masterUrl.c_str());
        return results;
    }
    if (res->status != 200) {
        fprintf(stderr, "Master server returned status %d\n", res->status);
        return results;
    }

    try {
        auto arr = nlohmann::json::parse(res->body);
        for (auto& s : arr) {
            ServerBrowserEntry entry;
            entry.ip = s.value("ip", "");
            entry.port = s.value("port", 0);
            entry.mapName = s.value("mapName", "Unknown");
            entry.players = s.value("players", 0);
            entry.maxPlayers = s.value("maxPlayers", 0);

            if (s.value("hasMapData", false)) {
                entry.mapData = fetchMapData(masterUrl, entry.ip, entry.port);
            }

            results.push_back(entry);
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to parse server list: %s\n", e.what());
    }

    return results;
}

std::string NetworkClient::fetchMapData(const std::string& masterUrl,
                                         const std::string& ip, int port) {
    httplib::Client cli(masterUrl.c_str());
    std::string path = "/mapdata?ip=" + ip + "&port=" + std::to_string(port);
    auto res = cli.Get(path.c_str());
    if (res && res->status == 200) return res->body;
    return {};
}

} // namespace mm
