#include "core/map_serializer.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace mm {

static nlohmann::json tileTypeToJson(TileType t) {
    return static_cast<uint16_t>(t);
}

static TileType jsonToTileType(const nlohmann::json& j) {
    return static_cast<TileType>(j.get<uint16_t>());
}

bool MapSerializer::saveToFile(const MapData& map, const std::string& path) {
    nlohmann::json j;
    j["formatVersion"] = 1;
    j["name"] = map.name();
    j["author"] = map.author();
    j["width"] = map.width();
    j["height"] = map.height();
    j["tileSize"] = map.tileSize();
    j["laps"] = map.laps();

    nlohmann::json ground = nlohmann::json::array();
    nlohmann::json objects = nlohmann::json::array();
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            ground.push_back(tileTypeToJson(map.getGround(x, y)));
            objects.push_back(tileTypeToJson(map.getObject(x, y)));
        }
    }
    j["ground"] = ground;
    j["objects"] = objects;

    nlohmann::json spawns = nlohmann::json::array();
    for (auto& s : map.spawns()) {
        spawns.push_back({{"x", s.x}, {"y", s.y}, {"angle", s.angle}});
    }
    j["spawns"] = spawns;

    nlohmann::json checks = nlohmann::json::array();
    for (auto& c : map.checkpoints()) {
        checks.push_back({{"x1", c.x1}, {"y1", c.y1}, {"x2", c.x2}, {"y2", c.y2}});
    }
    j["checkpoints"] = checks;

    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << j.dump(2);
    return true;
}

bool MapSerializer::loadFromFile(MapData& map, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    nlohmann::json j;
    try {
        file >> j;
    } catch (...) {
        return false;
    }

    int w = j.value("width", 32);
    int h = j.value("height", 32);
    int ts = j.value("tileSize", 32);

    map = MapData(w, h, ts);
    map.name() = j.value("name", "Untitled");
    map.author() = j.value("author", "");
    map.setLaps(j.value("laps", 3));

    if (j.contains("ground") && j["ground"].is_array()) {
        auto& g = j["ground"];
        for (int i = 0; i < w * h && i < static_cast<int>(g.size()); ++i) {
            int x = i % w;
            int y = i / w;
            map.setGround(x, y, jsonToTileType(g[i]));
        }
    }

    if (j.contains("objects") && j["objects"].is_array()) {
        auto& o = j["objects"];
        for (int i = 0; i < w * h && i < static_cast<int>(o.size()); ++i) {
            int x = i % w;
            int y = i / w;
            map.setObject(x, y, jsonToTileType(o[i]));
        }
    }

    if (j.contains("spawns") && j["spawns"].is_array()) {
        for (auto& s : j["spawns"]) {
            Spawn sp;
            sp.x = s.value("x", 0);
            sp.y = s.value("y", 0);
            sp.angle = s.value("angle", 0.0f);
            map.spawns().push_back(sp);
        }
    }

    if (j.contains("checkpoints") && j["checkpoints"].is_array()) {
        for (auto& c : j["checkpoints"]) {
            Checkpoint cp;
            cp.x1 = c.value("x1", 0);
            cp.y1 = c.value("y1", 0);
            cp.x2 = c.value("x2", cp.x1);
            cp.y2 = c.value("y2", cp.y1);
            map.checkpoints().push_back(cp);
        }
    }

    return true;
}

} // namespace mm
