# Mini Machines

A top-down 2D racing game with an arcade drift physics model, built in C++ with SDL2. Features a map editor, multiplayer via ENet, and supports Windows, Linux, and Android.

## Features

- **Arcade Drift Physics** — Forward/lateral velocity decomposition with per-surface grip, acceleration, and speed modifiers
- **Map Editor** — Full tile-based editor with brush size control, rectangle fill (shift+drag), road+wall brush preset, minimap, spawn/checkpoint placement
- **Multiplayer** — Dedicated server with master server browser, ENet networking, up to 8 players per server
- **Bot AI** — Single-player bots with checkpoint steering, raycast wall avoidance, and stuck detection
- **Race System** — Multi-lap races with checkpoints, countdown, position tracking, and finish detection
- **Surface Types** — Grass, Road, Dirt, Sand, Ice, Water with different physics; Wall, Barrier, Ramp, Boost, Oil objects
- **Random Track Generation** — Procedural loop tracks with guaranteed fallback
- **Android Support** — Cross-compiles for arm64-v8a via Android NDK, gamepad navigation

## Project Structure

```
minimachines/
├── core/                   # Shared library: tiles, map data, car physics, race logic, networking
│   ├── include/core/
│   │   ├── car.h           # CarState, CarConfig, carUpdate()
│   │   ├── race.h          # RaceState, RacerState, RaceData, raceInit/raceUpdate
│   │   ├── collision.h     # SurfaceInfo, isSolid(), circle-vs-grid collision
│   │   ├── bot.h           # BotConfig, BotState, botComputeInput()
│   │   ├── map_data.h      # MapData class, Spawn, Checkpoint structs
│   │   ├── map_serializer.h# JSON save/load
│   │   ├── tiles.h         # TileType enum, TileInfo, Layer enum
│   │   └── net/
│   │       ├── packet.h    # PacketType enum, DEFAULT_PORT/MASTER, MAX_PLAYERS
│   │       └── net_serial.h# StatePacket, InputPacket, pack/unpack functions
│   └── src/                # Implementations
├── game/                   # Main game client (SDL2 + ImGui HUD)
│   ├── src/
│   │   ├── main.cpp        # SDL2 init, ImGui setup, fixed timestep loop
│   │   ├── game_app.h/cpp  # Game state machine, rendering, multiplayer client
│   │   ├── network_client.h/cpp  # ENet client, master server queries
│   │   ├── camera.h/cpp    # Smooth-follow camera with adaptive zoom
│   │   ├── input.h/cpp     # Keyboard + gamepad input polling
│   │   └── ui/             # SDL2_ttf menu system
│   │       ├── ui_context.h/cpp  # Font loading, text rendering
│   │       ├── ui_widget.h/cpp   # Button, Label, TextInput, ListBox, Panel
│   │       ├── menu_screen.h/cpp # Main menu with map list
│   │       └── server_screen.h/cpp # Server browser with map preview
│   └── CMakeLists.txt
├── editor/                 # Map editor (SDL2 + ImGui docking)
│   ├── src/
│   │   ├── main.cpp        # SDL2 window, ImGui docking init
│   │   ├── editor_app.h/cpp# Map rendering, input dispatch, menus, save/load
│   │   ├── tools.h/cpp     # Paint, erase, rectangle fill, spawn/checkpoint stamps
│   │   ├── palette.h/cpp   # Tile selection, brush size, road+wall preset
│   │   ├── properties.h/cpp# Map metadata, new map, clear spawns/checkpoints
│   │   └── camera.h/cpp    # Pan/zoom camera
│   └── CMakeLists.txt
├── dedicated_server/       # Headless game server (ENet)
│   ├── src/
│   │   ├── main.cpp        # CLI args, master registration, heartbeat, tick loop
│   │   └── server_game.h/cpp # Player slots, spawn assignment, race state machine
│   ├── maps/               # Server map files (copy maps here)
│   └── CMakeLists.txt
├── master_server/          # HTTP master server for server browser
│   ├── src/
│   │   ├── main.cpp        # cpp-httplib REST API
│   │   └── registry.h/cpp  # Server list with heartbeat cleanup
│   └── CMakeLists.txt
├── android/                # Android build (Gradle + SDL2 from source)
├── assets/
│   └── fonts/              # DejaVuSans.ttf (actually arial.ttf)
├── cmake/
│   ├── deps.cmake          # FetchContent: ImGui, nlohmann/json, cpp-httplib
│   └── FindENet.cmake      # Custom ENet find module
└── CMakeLists.txt          # Top-level build config
```

## Building

### Prerequisites (MSYS2 / MinGW64)

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_ttf \
          mingw-w64-x86_64-enet
```

### Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Build individual targets:

```bash
cmake --build build --target game              # Game client
cmake --build build --target editor            # Map editor
cmake --build build --target dedicated_server  # Dedicated server
cmake --build build --target master_server     # Master server
```

## Running

### Single Player

Run `build/game/game.exe`. Select a map from the menu and click Play. Use WASD or arrow keys to drive. Press R to reset to spawn, ESC to return to menu.

### Map Editor

Run `build/editor/editor.exe`. Tools:

| Key | Action |
|-----|--------|
| P | Paint tool |
| E | Erase tool |
| S | Place spawn (click+drag to aim direction) |
| C | Place checkpoint (click+drag for line segment) |
| D | Delete entity |
| G | Toggle grid |
| 1 | Ground layer |
| 2 | Objects layer |
| Shift+drag | Rectangle fill |
| Right-drag | Pan camera |
| Scroll wheel | Zoom |

The palette panel has a brush size slider (1-8) and a Road+Wall preset toggle that paints road in the center with wall tiles on the edges.

### Multiplayer

**Start a master server:**
```bash
build/master_server/master_server.exe --port 8080
```

**Start a dedicated server:**
```bash
build/dedicated_server/dedicated_server.exe --map path/to/map.json --port 27015 --max 8
```

Place your map JSON files in `build/dedicated_server/maps/` so the server auto-detects them.

The server registers with the master server and sends heartbeats every 10 seconds with current player count and map data. If the map file changes on disk, the server auto-reloads it.

The default master server address is `http://192.168.1.240:8080`. Override with `--master`.

**Connect from the game client:** Open the Server Browser from the main menu. Servers on the master server are listed with map name and player count. Select a server and click Connect, or enter an IP directly.

### Race Rules

Multiplayer requires at least 2 connected players to start. The race goes through: Waiting (showing "Waiting for players") → Countdown (3 seconds) → Racing → Finished. Each player gets their own spawn point; if there are more players than spawn points, they stagger behind the first spawn.

## Map Format

Maps are JSON files with this structure:

```json
{
  "formatVersion": 1,
  "name": "My Track",
  "author": "Author",
  "width": 32,
  "height": 32,
  "tileSize": 32,
  "laps": 3,
  "ground": [1, 1, 2, ...],
  "objects": [0, 0, 7, ...],
  "spawns": [
    {"x": 10, "y": 5, "angle": 0.0}
  ],
  "checkpoints": [
    {"x1": 3, "y1": 5, "x2": 8, "y2": 5}
  ]
}
```

Tile type IDs: 0=Empty, 1=Grass, 2=Road, 3=Dirt, 4=Sand, 5=Ice, 6=Water, 7=Wall, 8=Barrier, 9=Ramp, 10=Boost, 11=Oil, 12=Start, 13=Checkpoint, 14=Finish.

Ground and objects are flat arrays in row-major order (y * width + x). Checkpoints are line segments drawn with Bresenham's algorithm.

## Dependencies

Fetched automatically via CMake FetchContent:
- [ImGui](https://github.com/ocornut/imgui) (docking branch) — HUD overlay and editor UI
- [nlohmann/json](https://github.com/nlohmann/json) v3.11.3 — Map serialization
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) v0.18.3 — HTTP client/server for master server

System dependencies (via package manager):
- SDL2, SDL2_image, SDL2_ttf
- ENet

## License

See individual dependency licenses for ImGui, nlohmann/json, cpp-httplib, SDL2, and ENet.
