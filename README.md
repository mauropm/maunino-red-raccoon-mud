# Little Red Raccoon MUD

A standalone Multi-User Dungeon (MUD) server running on **ESP32-C5-WROOM-1**.

Originally developed for the **Pwnterrey Badge** by **Electronic Cats** — a conference badge for Pwnterrey 2025 (Monterrey, Mexico).

No internet required. The ESP32 creates its own WiFi access point. Players connect with any device and play via a browser-based captive portal.

---

## The Lore

Deep in the forest, a peculiar ecosystem thrives. The **Little Red Raccoon** — not content with the traditional fairy tale script — has turned to a life of trash liberation. Armed with a red hood and questionable morals, she navigates a world where:

- **The Wolf** maintains a suspiciously tidy home, grows tomatoes with obsessive care, and is absolutely NOT the villain (he just wants to be left alone with his gardening and his sailor boyfriend).
- **The Oregon Sailor** sails the SS Trash Panda, has terrible navigation skills, and is in a surprisingly functional relationship with the Wolf.
- The **Raccoon Council** meets in a hidden chamber beneath the lighthouse, takes snacking very seriously, and has declared all trash bags as "community property."
- **Trash** is currency. Shiny bottle caps are treasure. The Golden Acorn is the ultimate prize.

You are a raccoon. Your mission: collect trash, explore 16 interconnected rooms, chat with other raccoons, avoid the Wolf's suspicious hospitality, and earn your rank from "Novice Dumpster Diver" to "Master Trash Panda."

---

## Features

- **WiFi Access Point**: SSID `RACCOON_MUD`, password `trashpanda`
- **Captive Portal**: Connect to WiFi, the game opens automatically in your browser
- **DNS Redirector**: All domain names resolve to the ESP32 — no internet needed
- **Browser-based terminal UI**: Black background, green text, classic BBS feel
- **8 concurrent players**
- **16 rooms** with humorous descriptions and lore
- **16 collectible items** with a scoring system
- **3 NPCs** that roam and speak
- **Random events** broadcast to all players
- **Real-time push** via Server-Sent Events (SSE)
- **Session persistence** via browser localStorage

---

## Hardware Requirements

- **ESP32-C5-WROOM-1** module or development board
- USB-C cable for power and flashing
- Any device with WiFi (phone, laptop, tablet) to play

---

## Prerequisites

- ESP-IDF 5.5.x installed and configured
- Python 3.11+ (for ESP-IDF tools)

---

## Build

```bash
idf.py set-target esp32c5
idf.py build
```

---

## Flash

Due to the ESP32-C5 revision (v1.0), you need to use ESP-IDF version 5.5.3+ (but keep using 5.x): 

```bash
esptool.py --chip esp32c5 -p <PORT> -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x2000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/red_raccoon_mud.bin
```

(Replace `<PORT>` with your serial port, e.g. `/dev/cu.usbmodem*` on macOS.)

For subsequent flashes after the first, you can use the shorter form:

```bash
idf.py -p <PORT> flash
```

---

## Play

1. Power the ESP32-C5-WROOM-1
2. Connect to WiFi: **RACCOON_MUD** / **trashpanda**
3. A captive portal should open automatically — if not, open `http://192.168.4.1/`
4. Enter your raccoon name and start playing

---

## Commands

| Command | Alias | Description |
|---------|-------|-------------|
| `help` | `?` | Show all commands |
| `look` | `l` | Look around the room |
| `who` | | See who is online |
| `say <msg>` | | Talk to room occupants |
| `shout <msg>` | | Shout to everyone online |
| `north` | `n` | Go north |
| `south` | `s` | Go south |
| `east` | `e` | Go east |
| `west` | `w` | Go west |
| `inventory` | `i` | Check your items |
| `take <item>` | `get` | Pick up an item |
| `drop <item>` | | Drop an item |
| `examine <item>` | `x` | Examine an item |
| `score` | | Check your score and rank |
| `quit` | `q` | Leave the game |

---

## Architecture

### Before: Telnet-based

```
Client → Telnet (TCP port 1111) → Command Parser → Game Engine
```

### After: Captive Portal Web

```
Client → Browser → HTTP (port 80) / SSE → Command Parser → Game Engine
                  → DNS (port 53) redirects all domains → ESP32
```

### Project Structure

```
main/
  main.c              - Entry point, task creation
  wifi.c / wifi.h     - WiFi AP initialization

  -- New Transport Layer --
  dns_server.c/.h     - DNS responder (port 53), all domains → ESP32 IP
  http_server.c/.h    - HTTP server (port 80), captive portal, SSE, REST API
  web_ui.h            - Embedded HTML/CSS/JS (~3 KB)

  -- Preserved Game Engine --
  player.c / player.h - Player management (fixed array of 8)
  world.c / world.h   - World state, room-player queries
  rooms.c / rooms.h   - Room definitions (16 rooms)
  items.c / items.h   - Item definitions (16 items)
  npc.c / npc.h       - NPC AI, messages, movement
  commands.c/.h       - Command parser and handlers
```

### FreeRTOS Tasks

| Task | Stack | Priority | Description |
|------|-------|----------|-------------|
| `dns_srv` | 4 KB | 5 | Responds to all DNS queries with the ESP32 IP |
| `http_srv` | 12 KB | 5 | HTTP server: serves UI, handles API, manages SSE |
| `random_evt` | 4 KB | 3 | Broadcasts random events every 2-3 minutes |

### Data Flow

```
Player → Browser → HTTP POST /api/command → command_process()
                                               → player_send() → SSE push to browser
                                               → player_broadcast_room() → SSE to all in room

Browser → GET /api/events?token=...   → SSE stream (real-time game output)
```

### Transport Abstraction

The game engine targets the `player_t` structure. All output goes through `player_send()`, which dispatches to the appropriate transport:

- **Telnet** (legacy): writes to TCP socket
- **Web**: writes to SSE connection via `sse_send_to_player()`

This allows the same `commands.c`, `world.c`, `rooms.c`, `items.c`, and `npc.c` to work unchanged regardless of transport.

---

## Rooms

| # | Name | Description |
|---|------|-------------|
| 0 | Forest Path | Your starting point. Four exits lead to adventure. |
| 1 | Berry Bushes | Lush berry bushes with suspicious banana evidence. |
| 2 | Creek Crossing | Shallow creek with wobbling stepping stones. |
| 3 | Abandoned Canoe | The Oregon Sailor's former vessel, now full of regret. |
| 4 | Wolf Front Yard | Suspiciously tidy. Welcome mat says "GO AWAY (politely)." |
| 5 | Wolf Kitchen | Recipe book open to "How to Cook for Your Sailor Partner." |
| 6 | Wolf Living Room | Fireplace, sofa, and framed photos of the Wolf with a sailor. |
| 7 | Trash Storage Shed | The raccoon holy grail. DO NOT TOUCH (sorted by day). |
| 8 | Oregon Sailor Dock | Sailboats bob in the water. Sailor hats everywhere. |
| 9 | Sailboat Deck | The SS Trash Panda. String lights and a small grill. |
| 10 | Lighthouse Hill | Spectacular view. Graffiti about the Wolf and Sailor. |
| 11 | Secret Raccoon Council | Round table. Tiny chairs. Meeting minutes on the wall. |
| 12 | Dark Alley | Perfect for sneaking. You feel watched. |
| 13 | Wolf Bedroom | Diary entry: "I left her the fancy ones. She seemed happy." |
| 14 | Secret Garden | Tomatoes named after Oregon sailors. |
| 15 | Riverbank | "BEST FISHING - WOLF NOT ALLOWED" |

---

## Items

| Item | Points | Trash? |
|------|--------|--------|
| Half-Eaten Sandwich | 5 | Yes |
| Shiny Bottle Cap | 10 | Yes |
| Fish Skeleton | 20 | Yes |
| Sailor Hat | 30 | No |
| Fancy Trash Bag | 50 | Yes |
| Rubber Boot | 15 | Yes |
| Suspicious Banana | 25 | Yes |
| Wolf Portrait | 35 | No |
| Rusty Key | 20 | No |
| Mossy Stone | 10 | Yes |
| Old Newspaper | 15 | Yes |
| Golden Acorn | 100 | No |
| Broken Compass | 20 | Yes |
| Tin Whistle | 25 | No |
| Red Hood | 40 | No |
| Pet Rock | 5 | Yes |

---

## Ranks

| Score | Rank |
|-------|------|
| 500+ | Master Trash Panda |
| 200+ | Senior Raccoon |
| 50+ | Trash Enthusiast |
| < 50 | Novice Dumpster Diver |

---

## Design Decisions

- **No dynamic allocation**: All game data uses fixed arrays allocated at compile time. No malloc/free during gameplay.
- **Non-blocking I/O**: The HTTP server uses `select()` — no task blocks on a single client.
- **Simple NPC AI**: NPCs move between rooms on timers and emit messages periodically.
- **Server-Sent Events over WebSockets**: SSE is simpler, lower memory, and sufficient for MUD push updates.
- **Transport abstraction**: Game engine communicates through function pointers — can support Telnet and Web simultaneously.

---

## Credits

Originally created for the **Pwnterrey Badge** by **Electronic Cats**.

The Pwnterrey badge is a conference badge distributed at Pwnterrey, a cybersecurity and technology conference in Monterrey, Mexico.

---

## License

MIT
