#include "registry.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = atoi(argv[++i]);
    }

    mm::Registry registry;
    httplib::Server svr;

    svr.Post("/register", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string ip = j.value("ip", req.remote_addr);
            int sport = j.value("port", 27015);
            std::string mapName = j.value("mapName", "Unknown");
            std::string mapData = j.value("mapData", "");
            int players = j.value("players", 0);
            int maxPlayers = j.value("maxPlayers", 8);

            registry.registerServer(ip, sport, mapName, mapData, players, maxPlayers);
            fprintf(stderr, "Registered server: %s:%d map=%s players=%d/%d\n",
                    ip.c_str(), sport, mapName.c_str(), players, maxPlayers);

            res.set_content("{\"ok\":true}", "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid json\"}", "application/json");
        }
    });

    svr.Post("/heartbeat", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string ip = j.value("ip", req.remote_addr);
            int sport = j.value("port", 27015);
            int players = j.value("players", 0);

            registry.heartbeat(ip, sport, players);
            res.set_content("{\"ok\":true}", "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid json\"}", "application/json");
        }
    });

    svr.Get("/servers", [&](const httplib::Request&, httplib::Response& res) {
        registry.cleanup();
        auto servers = registry.getServers();
        nlohmann::json arr = nlohmann::json::array();
        for (auto& s : servers) {
            arr.push_back({
                {"ip", s.ip},
                {"port", s.port},
                {"mapName", s.mapName},
                {"players", s.players},
                {"maxPlayers", s.maxPlayers},
                {"hasMapData", !s.mapData.empty()}
            });
        }
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get("/mapdata", [&](const httplib::Request& req, httplib::Response& res) {
        std::string ip = req.get_param_value("ip");
        std::string portStr = req.get_param_value("port");
        int sport = atoi(portStr.c_str());
        std::string data = registry.getMapData(ip, sport);
        if (data.empty()) {
            res.status = 404;
            res.set_content("{\"error\":\"not found\"}", "application/json");
        } else {
            res.set_content(data, "application/json");
        }
    });

    std::thread cleanupThread([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            registry.cleanup();
        }
    });
    cleanupThread.detach();

    fprintf(stderr, "Master server listening on port %d\n", port);
    svr.listen("0.0.0.0", port);

    return 0;
}
