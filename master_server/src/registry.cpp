#include "registry.h"

namespace mm {

void Registry::registerServer(const std::string& ip, int port, const std::string& mapName,
                              const std::string& mapData, int players, int maxPlayers) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& s : m_servers) {
        if (s.ip == ip && s.port == port) {
            s.mapName = mapName;
            s.mapData = mapData;
            s.players = players;
            s.maxPlayers = maxPlayers;
            s.lastHeartbeat = std::chrono::steady_clock::now();
            return;
        }
    }
    ServerEntry entry;
    entry.ip = ip;
    entry.port = port;
    entry.mapName = mapName;
    entry.mapData = mapData;
    entry.players = players;
    entry.maxPlayers = maxPlayers;
    entry.lastHeartbeat = std::chrono::steady_clock::now();
    m_servers.push_back(entry);
}

void Registry::heartbeat(const std::string& ip, int port, int players,
                         const std::string& mapName, const std::string& mapData) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& s : m_servers) {
        if (s.ip == ip && s.port == port) {
            s.players = players;
            if (!mapName.empty()) s.mapName = mapName;
            if (!mapData.empty()) s.mapData = mapData;
            s.lastHeartbeat = std::chrono::steady_clock::now();
            return;
        }
    }
}

void Registry::cleanup(int timeoutSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();
    auto it = m_servers.begin();
    while (it != m_servers.end()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->lastHeartbeat).count();
        if (elapsed > timeoutSeconds) {
            fprintf(stderr, "Removing stale server: %s:%d\n", it->ip.c_str(), it->port);
            it = m_servers.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<ServerEntry> Registry::getServers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_servers;
}

std::string Registry::getMapData(const std::string& ip, int port) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& s : m_servers) {
        if (s.ip == ip && s.port == port)
            return s.mapData;
    }
    return {};
}

} // namespace mm
