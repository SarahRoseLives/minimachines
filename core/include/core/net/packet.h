#pragma once

#include <cstdint>

namespace mm {
namespace net {

constexpr int TICK_RATE = 30;
constexpr float TICK_DT = 1.0f / TICK_RATE;
constexpr int DEFAULT_PORT = 27015;
constexpr int MAX_PLAYERS = 8;
constexpr int CHANNEL_RELIABLE = 0;
constexpr int CHANNEL_UNRELIABLE = 1;
constexpr const char* DEFAULT_MASTER = "http://192.168.1.240:8080";

enum class PacketType : uint8_t {
    // Reliable (channel 0)
    C2S_JOIN = 1,
    S2C_MAP_DATA = 2,
    S2C_RACE_STATE = 3,
    S2C_PLAYER_LIST = 4,

    // Unreliable (channel 1)
    C2S_INPUT = 10,
    S2C_STATE = 11,
};

struct PacketHeader {
    PacketType type;
    uint8_t playerIndex;
    uint16_t sequence;
};

} // namespace net
} // namespace mm
