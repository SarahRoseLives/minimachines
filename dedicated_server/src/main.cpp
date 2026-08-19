#include "server_game.h"
#include <enet/enet.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

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

static bool registerWithMaster(const std::string& masterUrl, int port, const std::string& mapName,
                                int players, int maxPlayers, const std::string& mapData) {
    httplib::Client cli(masterUrl.c_str());
    cli.set_connection_timeout(3, 0);
    cli.set_read_timeout(3, 0);

    nlohmann::json body;
    body["port"] = port;
    body["mapName"] = mapName;
    body["players"] = players;
    body["maxPlayers"] = maxPlayers;
    body["mapData"] = mapData;

    auto res = cli.Post("/register", body.dump(), "application/json");
    if (res && res->status == 200) {
        fprintf(stderr, "Registered with master server at %s\n", masterUrl.c_str());
        return true;
    }
    fprintf(stderr, "Failed to register with master server at %s\n", masterUrl.c_str());
    return false;
}

static bool heartbeatMaster(const std::string& masterUrl, int port, int players,
                            const std::string& mapName, const std::string& mapData) {
    httplib::Client cli(masterUrl.c_str());
    cli.set_connection_timeout(3, 0);
    cli.set_read_timeout(3, 0);

    nlohmann::json body;
    body["port"] = port;
    body["players"] = players;
    body["mapName"] = mapName;
    body["mapData"] = mapData;

    auto res = cli.Post("/heartbeat", body.dump(), "application/json");
    return res && res->status == 200;
}

int main(int argc, char* argv[]) {
    int port = mm::net::DEFAULT_PORT;
    int maxPlayers = mm::net::MAX_PLAYERS;
    std::string mapFile;
    std::string masterUrl = mm::net::DEFAULT_MASTER;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--max") == 0 && i + 1 < argc) maxPlayers = atoi(argv[++i]);
        else if (strcmp(argv[i], "--map") == 0 && i + 1 < argc) mapFile = argv[++i];
        else if (strcmp(argv[i], "--master") == 0 && i + 1 < argc) masterUrl = argv[++i];
    }

    if (enet_initialize() != 0) {
        fprintf(stderr, "Failed to initialize ENet\n");
        return 1;
    }

    std::string mapsDir = getExeDir() + "\\maps";
    if (!fs::exists(mapsDir)) {
        fs::create_directory(mapsDir);
        fprintf(stderr, "Created maps directory: %s\n", mapsDir.c_str());
    }

    if (mapFile.empty()) {
        for (auto& entry : fs::directory_iterator(mapsDir)) {
            if (entry.path().extension() == ".json") {
                mapFile = entry.path().string();
                break;
            }
        }
        if (mapFile.empty()) {
            fprintf(stderr, "No maps found in %s\n", mapsDir.c_str());
            enet_deinitialize();
            return 1;
        }
    }

    mm::ServerGame server;
    if (!server.init(port, maxPlayers, mapFile)) {
        enet_deinitialize();
        return 1;
    }

    std::string mapData = server.getMapData();
    registerWithMaster(masterUrl, port, server.getMapName(), 0, maxPlayers, mapData);

    auto tickDuration = std::chrono::microseconds(1000000 / mm::net::TICK_RATE);
    auto lastTick = std::chrono::steady_clock::now();
    auto lastHeartbeat = std::chrono::steady_clock::now();

    while (server.isRunning()) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTick).count();
        lastTick = now;

        server.update(dt);
        server.sendState();

        auto elapsedSinceHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat).count();
        if (elapsedSinceHeartbeat >= 10 || server.takePlayerCountChanged()) {
            heartbeatMaster(masterUrl, port, server.getPlayerCount(),
                            server.getMapName(), server.getMapData());
            lastHeartbeat = now;
        }

        if (server.checkMapChanged()) {
            fprintf(stderr, "Map file changed on disk, reloading...\n");
            server.reloadMap();
            heartbeatMaster(masterUrl, port, server.getPlayerCount(),
                            server.getMapName(), server.getMapData());
            lastHeartbeat = now;
        }

        auto elapsed = std::chrono::steady_clock::now() - now;
        if (elapsed < tickDuration)
            std::this_thread::sleep_for(tickDuration - elapsed);
    }

    server.shutdown();
    enet_deinitialize();
    return 0;
}
