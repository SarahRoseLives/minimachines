#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>

namespace mm {

struct ServerEntry {
    std::string ip;
    int port;
    std::string mapName;
    std::string mapData;
    int players;
    int maxPlayers;
    std::chrono::steady_clock::time_point lastHeartbeat;
};

class Registry {
public:
    void registerServer(const std::string& ip, int port, const std::string& mapName,
                        const std::string& mapData, int players, int maxPlayers);
    void heartbeat(const std::string& ip, int port, int players,
                   const std::string& mapName = "", const std::string& mapData = "");
    void cleanup(int timeoutSeconds = 60);
    std::vector<ServerEntry> getServers() const;
    std::string getMapData(const std::string& ip, int port) const;

private:
    mutable std::mutex m_mutex;
    std::vector<ServerEntry> m_servers;
};

} // namespace mm
