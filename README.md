# PewPewBoomBoom - 2D Top-Down Multiplayer Tank Combat Game

Welcome to the development repository for **PewPewBoomBoom**, a 2D top-down multiplayer tank combat game built with C++, **raylib** for rendering/audio, and **ENet** for reliable/unreliable UDP network communication. 

This document serves as both the **Game Design Document (GDD)** and the **Step-by-Step Implementation Plan** to guide development from start to a polished release supporting up to 32 players over the network.

---

## Table of Contents
1. [Game Design Document (GDD)](#1-game-design-document-gdd)
   - [Core Gameplay Mechanics](#core-gameplay-mechanics)
   - [Weapons & Powerups](#weapons--powerups)
   - [Arena & Environment](#arena--environment)
   - [Technical Architecture](#technical-architecture)
   - [Network Synchronization Model](#network-synchronization-model)
   - [Network Protocol & Message Specification](#network-protocol--message-specification)
2. [Project Architecture](#2-project-architecture)
   - [Shared Library (sharedLib)](#shared-library-sharedlib)
   - [Server Component (server)](#server-component-server)
   - [Client Component (client)](#client-component-client)
3. [Implementation Plan (Roadmap)](#3-implementation-plan-roadmap)
   - [Phase 1: Setup & Networking Foundations](#phase-1-setup--networking-foundations)
   - [Phase 2: Network Protocol & Shared Data Structures](#phase-2-network-protocol--shared-data-structures)
   - [Phase 3: Fixed-Tick Server Simulation & Client Skeleton](#phase-3-fixed-tick-server-simulation--client-skeleton)
   - [Phase 4: Player Input Replication, Prediction, and Reconciliation](#phase-4-player-input-replication-prediction-and-reconciliation)
   - [Phase 5: Collision Detection & Map Obstacles](#phase-5-collision-detection--map-obstacles)
   - [Phase 6: Combat System, Bullets, & Damage](#phase-6-combat-system-bullets--damage)
   - [Phase 7: Weapons & Powerups Sandbox](#phase-7-weapons--powerups-sandbox)
   - [Phase 8: HUD, UI, Audio, & Visual Polish](#phase-8-hud-ui-audio--visual-polish)
   - [Phase 9: Stress Testing, Optimization, & Lag Simulation](#phase-9-stress-testing-optimization--lag-simulation)
4. [Development Milestones & Success Criteria](#4-development-milestones--success-criteria)

---

## 1. Game Design Document (GDD)

### Core Gameplay Mechanics
- **Player Representation**: Every player controls a tank composed of two sprites: a **Chassis** (base) and a **Turret**.
- **Movement (WASD)**: 
  - Tank moves forward/backward relative to its chassis orientation (`W`/`S`).
  - Tank rotates left/right (`A`/`D`) to change the chassis orientation.
  - Movement is governed by physics constants: acceleration, max speed, and friction/drag.
- **Aiming (Mouse)**: The turret rotates independently to face the screen cursor, allowing players to drive in one direction while shooting in another.
- **Shooting (Left Mouse Button)**: Fires the currently active weapon in the direction of the turret's rotation.
- **Player Limit**: Up to 32 players concurrently on a single server instance.

### Weapons & Powerups

| Weapon Type | Ammo | Cooldown | Velocity | Behavior |
| :--- | :--- | :--- | :--- | :--- |
| **Standard Cannon** | Infinite | 0.35s | Fast | Moderate damage, straight projectile trajectory. |
| **Rapid-Fire MG** | 100 max | 0.08s | High | Low damage per round, slight spread, high fire rate. |
| **Heavy Rocket** | 5 max | 1.20s | Slow | High damage, explodes on contact dealing AoE damage. |
| **Laser Railgun** | 3 max | 1.80s | Instant | Hitscan line trace, penetrates enemies, high damage. |

| Powerup Type | Spawn Rate | Duration | Behavior |
| :--- | :--- | :--- | :--- |
| **Health Repair** | High | Instant | Restores 50% of the tank's maximum health. |
| **Speed Boost** | Medium | 8.0s | Increases chassis acceleration and max speed by 50%. |
| **Shield Generator**| Low | 5.0s | Grants total damage immunity; visual shield bubble active. |
| **Weapon Crates** | Medium | N/A | Refills ammo for special weapons or equips them. |

### Arena & Environment
- **Top-Down Map Layout**: The arena is bounded by outer walls and filled with obstacles.
- **Obstacle Categories**:
  - **Indestructible Blocks**: Solid walls (concrete/metal) that reflect projectiles or absorb damage. Blocks both movement and bullets.
  - **Destructible Blocks**: Crates or brick walls that crumble after taking a specific amount of cumulative damage, revealing new paths or powerups.
  - **Bushes/Cover**: Visual-only overlays that hide tanks inside them from players outside (line-of-sight visual mechanics).
- **Spawn Zones**: Multiple spawn points spread across the arena to prevent spawn camping.

---

### Technical Architecture
The codebase is structured as a multi-project workspace managed by **Premake5**:
1. **`sharedLib`**: Houses packet definitions, math functions, collision detection logic, and state definitions. This shared library compiles as a static library and is linked by both the server and client.
2. **`server`**: A headless C++ application hosting the authoritative game simulation. It processes network inputs and distributes game states.
3. **`client`**: A graphical C++ application using **raylib** to render the game world, accept local player inputs, play audio, and represent the UI/menus.

### Network Synchronization Model
A server-authoritative simulation is used to prevent cheating and maintain a singular source of truth. However, relying purely on server responses creates a laggy experience for clients. The following techniques will be implemented:

```mermaid
sequenceDiagram
    participant Client
    participant Server

    Note over Client: User presses W (Frame 100)
    Client->>Client: Apply input locally (Client Prediction)<br/>Store input & predicted state in history buffer
    Client->>Server: Send Input Packet (Input: W, Frame: 100)
    Note over Server: Server receives Input Packet
    Server->>Server: Update player state using Input W (Tick 100)
    Server->>Client: Broadcast World Snapshot (Tick 100, Authoritative Position)
    Note over Client: Client receives Snapshot for Tick 100
    alt Server State Matches Client Predicted State
        Note over Client: Keep playing smoothly
    else Mismatch Detected (Server Reconciliation)
        Note over Client: Snap to Server Position<br/>Replay all inputs from Tick 101 to Current Frame
    end
```

1. **Client-Side Prediction**: The local player moves immediately in response to their keyboard input, without waiting for the server to reply.
2. **Server Reconciliation**: The server constantly broadcasts authoritative positions. The client stores a history buffer of sent inputs and predicted states. When a server update arrives, the client compares the historical state with the server's authoritative state. If they differ, the client snaps to the server's state and replays the inputs stored in the buffer up to the current tick.
3. **Entity Interpolation**: Other tanks and projectiles are not predicted. Instead, the client buffers incoming server snapshots (typically 100ms behind the real-time tick) and interpolates their positions smoothly between the two most recently received ticks to prevent jittering.
4. **Lag Compensation (Hit Registration)**: The server keeps a short history of player hitboxes. When a player fires a hitscan weapon (like the Laser Railgun), the client sends the tick number when they fired. The server rewinds the world to that tick to evaluate if the laser hit the target, neutralizing latency disadvantages for high-ping players.

### Network Protocol & Message Specification

To implement real-time tank battles with up to 32 players, **PewPewBoomBoom** uses a structured binary protocol over **ENet**. This protocol minimizes bandwidth overhead and aligns variables to prevent platform padding issues.

#### ENet Channel Configurations
* **Channel 0 (Reliable)**: Used for critical, non-frequent state updates and handshakes (connections, leaves, combat events, chat). Message delivery is guaranteed and ordered.
* **Channel 1 (Unreliable)**: Used for high-frequency game states (player inputs, world snapshots). Lost packets are discarded, avoiding head-of-line blocking.

```mermaid
graph TD
    subgraph ENet Channels
        C0[Channel 0: Reliable]
        C1[Channel 1: Unreliable]
    end

    subgraph Client-to-Server
        C0 -->|JoinRequest, RespawnRequest, ChatMessage| Server
        C1 -->|InputState| Server
    end

    subgraph Server-to-Client
        Server -->|JoinResponse, EventNotification, ChatMessage, PlayerDisconnected| C0
        Server -->|WorldSnapshot| C1
    end
```

#### Common Packet Header
All network packets begin with a single byte indicating the packet type:
* **Byte Offset 0**: `PacketType` (1 byte, unsigned integer)

---

#### 1. Client-to-Server (C2S) Messages

##### C2S_JoinRequest (Channel 0 - Reliable)
Sent by a client attempting to connect and register with the server.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::C2S_JoinRequest` (value: 0) |
| **ProtocolVersion** | `uint32_t` | 4 | Protocol version hash to check compatibility. |
| **PlayerNameLength**| `uint8_t` | 1 | Length of player name string (max 16). |
| **PlayerName** | `char[]` | Variable | UTF-8 string containing the user's nickname. |

##### C2S_InputState (Channel 1 - Unreliable)
Sent every frame/tick (60Hz) by clients to drive the player's tank.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::C2S_InputState` (value: 2) |
| **ClientTick** | `uint32_t` | 4 | The local tick number when this input was generated. |
| **InputMask** | `uint8_t` | 1 | Bitfield for keys: `W` (bit 0), `S` (bit 1), `A` (bit 2), `D` (bit 3), `Shoot` (bit 4). |
| **TurretAngle** | `float` | 4 | Current angle of the turret in radians (0 to $2\pi$). |
| **SelectedWeapon**| `uint8_t` | 1 | Numeric ID of the weapon slots currently active. |

##### C2S_ChatMessage (Channel 0 - Reliable)
Sent by a player to broadcast text in the lobby or in-game chat.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::C2S_ChatMessage` (value: 5) |
| **MessageLength** | `uint8_t` | 1 | Length of message content (max 128 characters). |
| **MessageText** | `char[]` | Variable | UTF-8 encoded text string. |

##### C2S_RespawnRequest (Channel 0 - Reliable)
Sent by a dead client to request respawning in the game.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::C2S_RespawnRequest` (value: 7) |

---

#### 2. Server-to-Client (S2C) Messages

##### S2C_JoinResponse (Channel 0 - Reliable)
Server response containing authorization status, assigned ID, and general map constants.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::S2C_JoinResponse` (value: 1) |
| **ResponseCode** | `uint8_t` | 1 | `0`: Success, `1`: Server Full, `2`: Version Mismatch. |
| **AssignedID** | `uint8_t` | 1 | Unique network ID (0-31) representing the client's tank. |
| **SpawnX** | `float` | 4 | Initial coordinate X on spawning. |
| **SpawnY** | `float` | 4 | Initial coordinate Y on spawning. |
| **MapSeed** | `uint32_t` | 4 | Seed to generate identical procedural map details on client. |

##### S2C_WorldSnapshot (Channel 1 - Unreliable)
Broadcast at 60Hz from the server to synchronize overall world state.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::S2C_WorldSnapshot` (value: 3) |
| **ServerTick** | `uint32_t` | 4 | Authoritative server tick count. |
| **LastAckedTick** | `uint32_t` | 4 | The last client tick processed by the server (for reconciliation). |
| **PlayerCount** | `uint8_t` | 1 | Number of active tanks in this snapshot ($N_p$). |
| **PlayerArray** | `PlayerNetState[]` | $N_p \times 23$ | Array of active tank coordinates and states (see structure below). |
| **BulletCount** | `uint16_t` | 2 | Number of active bullets ($N_b$). |
| **BulletArray** | `BulletNetState[]` | $N_b \times 13$ | Array of active bullets (see structure below). |
| **PowerupCount** | `uint8_t` | 1 | Number of spawned powerup items on map ($N_m$). |
| **PowerupArray** | `PowerupNetState[]`| $N_m \times 10$| Array of spawned powerups (see structure below). |

##### S2C_EventNotification (Channel 0 - Reliable)
Sent when high-impact game events occur that require perfect delivery.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::S2C_EventNotification` (value: 4) |
| **EventType** | `uint8_t` | 1 | Event: `0` (Damage Taken), `1` (Kill Event), `2` (Powerup Collected), `3` (Match Finished). |
| **SourceID** | `uint8_t` | 1 | Player/entity ID that initiated the event. |
| **TargetID** | `uint8_t` | 1 | Player/entity ID affected by the event. |
| **EventValue** | `float` | 4 | Payload variable (e.g., damage amount, weapon ammo type). |

##### S2C_PlayerDisconnected (Channel 0 - Reliable)
Broadcast when a peer disconnects to clean up the client state immediately.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::S2C_PlayerDisconnected` (value: 6) |
| **PlayerID** | `uint8_t` | 1 | The ID of the player who disconnected. |

##### S2C_ChatMessage (Channel 0 - Reliable)
Broadcast of a text message from a user.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PacketType** | `uint8_t` | 1 | Set to `PacketType::S2C_ChatMessage` (value: 8) |
| **SenderID** | `uint8_t` | 1 | Client ID who sent the message (or `255` for Server/System). |
| **MessageLength** | `uint8_t` | 1 | Length of message content. |
| **MessageText** | `char[]` | Variable | UTF-8 encoded message text. |

---

#### 3. Nested Network State Data Structures

##### PlayerNetState (23 Bytes)
Contains the network state representation of a single tank.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PlayerID** | `uint8_t` | 1 | Network entity index (0 to 31). |
| **PositionX** | `float` | 4 | Current X coordinate of the tank chassis. |
| **PositionY** | `float` | 4 | Current Y coordinate of the tank chassis. |
| **ChassisAngle** | `float` | 4 | Yaw angle of the tank chassis (radians). |
| **TurretAngle** | `float` | 4 | Yaw angle of the tank turret (radians). |
| **Health** | `uint8_t` | 1 | Tank health percentage (0 to 100). |
| **ActiveBuffs** | `uint8_t` | 1 | Bitfield flags for active states: Shielded, Speed Boost, etc. |
| **WeaponIndex** | `uint8_t` | 1 | Currently active weapon slot ID. |
| **WeaponAmmo** | `uint16_t` | 2 | Remaining ammunition count. |

##### BulletNetState (13 Bytes)
Contains the minimal network footprint representing a moving bullet.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **BulletID** | `uint16_t` | 2 | Unique bullet serial ID. |
| **OwnerID** | `uint8_t` | 1 | Player ID of the tank that fired this bullet. |
| **BulletType** | `uint8_t` | 1 | Weapon projectile class (e.g. Standard, MG, Rocket). |
| **PositionX** | `float` | 4 | Current X coordinate of the bullet. |
| **PositionY** | `float` | 4 | Current Y coordinate of the bullet. |
| **ChassisAngle** | `uint8_t` | 1 | Bullet direction angle packed into a single byte ($0\text{-}255$ representing $0\text{-}2\pi$). |

##### PowerupNetState (10 Bytes)
Contains the coordinates and state of an item spawned on the map.

| Field | Data Type | Bytes | Description |
| :--- | :--- | :--- | :--- |
| **PowerupID** | `uint8_t` | 1 | Unique spawned item index. |
| **PowerupType** | `uint8_t` | 1 | Type index (Health, Speed, Shield, Weapon Crate). |
| **PositionX** | `float` | 4 | Center X coordinate. |
| **PositionY** | `float` | 4 | Center Y coordinate. |

---


## 2. Project Architecture

### Directory Structure
```text
pewpewboomboom/
├── premake5.lua                # Main Premake configuration
├── premake5.exe                # Premake executable (Windows)
├── sharedLib/
│   ├── premake5.lua
│   ├── include/
│   │   ├── Protocol.h          # Network packet structures & IDs
│   │   ├── Common.h            # Config constants (FPS, Tickrate, Max Players)
│   │   ├── EntityState.h       # Structs representing tanks, projectiles, etc.
│   │   └── Physics.h           # Lightweight 2D collision logic (AABB, circle, raycast)
│   └── src/
│       ├── Physics.cpp
│       └── Protocol.cpp
├── server/
│   ├── premake5.lua
│   ├── include/
│   │   ├── ServerInstance.h    # Core server manager
│   │   └── World.h             # Authoritative simulation state
│   └── src/
│       ├── main.cpp            # Entrypoint (initializes ENet & runs tick loop)
│       ├── ServerInstance.cpp
│       └── World.cpp
└── client/
    ├── premake5.lua
    ├── include/
    │   ├── ClientInstance.h    # Core client manager
    │   ├── Render.h            # Sprites, animations, and particle systems
    │   └── LocalPlayer.h       # Prediction & Input buffering
    └── src/
        ├── main.cpp            # Entrypoint (initializes raylib, ENet & main loop)
        ├── ClientInstance.cpp
        ├── Render.cpp
        └── LocalPlayer.cpp
```

---

## 3. Implementation Plan (Roadmap)

### Phase 1: Setup & Networking Foundations
**Goal**: Integrate ENet into the project build system, initialize ENet on client and server, and establish a successful connection handshake.

- [ ] **1.1 ENet Premake Setup**
  - Download ENet source code or integrate it as a submodule.
  - Modify `premake5.lua` to compile ENet as a static library (like Box2D) or link it dynamically/statically.
  - Link ENet to both `client` and `server` build scripts.
- [ ] **1.2 Headless Server Initialization**
  - Modify [server/src/main.cpp](file:///c:/Users/jeffm/Desktop/pewpewboomboom/server/src/main.cpp).
  - Add initialization of ENet: `enet_initialize()`.
  - Create an ENet host: `enet_host_create()` listening on a configurable port (default: `7777`).
  - Create a basic server tick loop that polls ENet events (`enet_host_service`) and logs client connection and disconnection events.
- [ ] **1.3 Client Connection System**
  - Modify [client/src/main.cpp](file:///c:/Users/jeffm/Desktop/pewpewboomboom/client/src/main.cpp).
  - Add initialization of ENet: `enet_initialize()`.
  - Create an ENet client host: `enet_host_create(NULL, 1, 2, 0, 0)`.
  - Add a connection routine targeting `localhost:7777` using `enet_host_connect()`.
  - Print connection success/failure logs to console and display status on screen using raylib text rendering.
- [ ] **1.4 Simple Echo Handshake**
  - Define a test packet in the shared module.
  - Send a "Hello Server" message from the client upon successful connection.
  - The server receives it, prints the message, and sends back an echo packet ("Hello Client").
  - Verify handshake reliability.

---

### Phase 2: Network Protocol & Shared Data Structures
**Goal**: Standardize the message format, sequence ordering, and serialization utility logic.

- [ ] **2.1 Configuration Constant Defs**
  - Create [sharedLib/include/Common.h](file:///c:/Users/jeffm/Desktop/pewpewboomboom/sharedLib/include/Common.h).
  - Define global variables and constants:
    - `TICK_RATE = 60` (ticks per second)
    - `TICK_TIME = 1.0f / TICK_RATE` (seconds per tick)
    - `MAX_PLAYERS = 32`
    - `MAX_PROJECTILES = 128`
    - `MAP_WIDTH = 2000`, `MAP_HEIGHT = 2000`
- [ ] **2.2 Packet Struct Serialization**
  - Create [sharedLib/include/Protocol.h](file:///c:/Users/jeffm/Desktop/pewpewboomboom/sharedLib/include/Protocol.h) containing the `PacketType` enum:
    ```cpp
    enum class PacketType : uint8_t {
        C2S_JoinRequest = 0,
        S2C_JoinResponse = 1,
        C2S_InputState = 2,
        S2C_WorldSnapshot = 3,
        S2C_EventNotification = 4,
        C2S_ChatMessage = 5,
        S2C_PlayerDisconnected = 6,
        C2S_RespawnRequest = 7,
        S2C_ChatMessage = 8
    };
    ```
  - Implement a manual bitstream/binary writer/reader helper class to pack and unpack values into flat `uint8_t` buffers. This ensures cross-platform byte alignment (no compiler padding issues).
- [ ] **2.3 Define Core Data Packets**
  - `C2S_InputState`: Tick count, flags (WASD, shoot), turret angle.
  - `S2C_WorldSnapshot`: Current tick count, active player entities (ID, position, chassis angle, turret angle, health, current weapon, states), projectile list (ID, type, position, velocity).
  - `S2C_JoinResponse`: Assigned Player ID, spawned coordinates, arena map data configuration.

---

### Phase 3: Fixed-Tick Server Simulation & Client Skeleton
**Goal**: Implement a robust game loop ticking at exactly 60Hz on the server, and prepare the client to render networked entities visually using primitive shapes.

- [ ] **3.1 Server Fixed-Tick Loop**
  - Implement a high-precision accumulator-based game loop in the server:
    ```cpp
    double accumulator = 0.0;
    double currentTime = GetTime(); // using raylib time or high-resolution clock
    while (running) {
        double newTime = GetTime();
        double frameTime = newTime - currentTime;
        currentTime = newTime;
        accumulator += frameTime;
        while (accumulator >= TICK_TIME) {
            ServerTick();
            accumulator -= TICK_TIME;
        }
        enet_host_service(host, &event, 1); // poll events between steps
    }
    ```
- [ ] **3.2 Server World State Structure**
  - In [server/include/World.h](file:///c:/Users/jeffm/Desktop/pewpewboomboom/server/include/World.h), maintain a map or array of connected player objects, bullets, and powerup structures.
  - On each `ServerTick`, advance the simulation time (tick count) and serialize the state to send a `S2C_WorldSnapshot` to all connected ENet peers.
- [ ] **3.3 Client Scene Skeleton**
  - Transition the client from the default 3D template to a 2D viewport.
  - Implement a connection configuration screen using simple raylib GUI elements (TextBox for IP address, Button for "Connect").
  - Once connected, draw a grid representing the arena background, and draw simple rectangles for any tanks/projectiles reported in the latest `S2C_WorldSnapshot` packet.

---

### Phase 4: Player Input Replication, Prediction, and Reconciliation
**Goal**: Smooth out local movement and replicate all player coordinates across the network.

- [ ] **4.1 Client Input Sampling & Transmission**
  - Every update frame, read keyboard WASD state and compute turret target direction (mouse screen position translated to world coordinates using camera transformation).
  - Package inputs with the current Client Tick number and transmit `C2S_InputState` immediately via ENet (unreliable channel).
- [ ] **4.2 Server Movement Simulation**
  - The server collects client inputs.
  - In `ServerTick()`, read the input for each client.
  - Calculate tank acceleration, update velocity, apply friction, and move the tank base. Rotate the chassis and turret to match input.
- [ ] **4.3 Client-Side Prediction**
  - The client runs the same physics movement code locally immediately upon sampling input.
  - Maintain a circular history buffer of `(Input, PredictedState, Tick)` on the client.
- [ ] **4.4 Server Reconciliation**
  - When the client receives a `S2C_WorldSnapshot` containing server position for tick $T$:
    - Locate the history entry for tick $T$.
    - If the difference between the client predicted position and server position exceeds a small threshold (e.g., 0.1 units):
      1. Overwrite the predicted position for tick $T$ with the server's authoritative position.
      2. Re-simulate the movement physics for all frames starting from tick $T+1$ up to the current local tick using the cached inputs in the buffer.
- [ ] **4.5 Entity Interpolation for Remote Players**
  - Do not predict other players.
  - Store a queue of received snapshots with their server timestamps.
  - Render other tanks by interpolating between `Snapshot_A` and `Snapshot_B` with an interpolation delay (e.g., $100\text{ms}$). This ensures perfectly smooth movement even with jittery network packets.

---

### Phase 5: Collision Detection & Map Obstacles
**Goal**: Add physical collision barriers to prevent tanks from going out of bounds, passing through walls, or overlapping.

- [ ] **5.1 Simple 2D Collision Math**
  - Implement standard Axis-Aligned Bounding Box (AABB) and Circle collision routines in `sharedLib/src/Physics.cpp`.
  - Add circle-to-AABB collision resolve functions (returns a displacement vector to push overlapping entities out of walls).
- [ ] **5.2 Server Arena Construction**
  - Define static array map layouts in `sharedLib`.
  - Fill the map with indestructible walls (represented as AABB boxes) and boundaries.
  - Add destructible walls with health attributes.
- [ ] **5.3 Wall Collision Processing**
  - In the server physics update step (run immediately after applying player inputs):
    - Check each player tank circle against all wall AABBs.
    - If a collision occurs, resolve it by shifting the player tank back until it no longer intersects.
    - Check player-to-player circle intersections and resolve them to prevent tanks stacking on top of each other.
  - Perform the exact same collision check on the client prediction path to avoid predicted tanks from glitching into walls prior to server correction.

---

### Phase 6: Combat System, Bullets, & Damage
**Goal**: Enable players to fire weapons, track projectiles on the server, detect bullet hits, and manage tank damage/respawn.

- [ ] **6.1 Shooting Handshake**
  - When the client presses the Left Mouse Button, set the shooting flag in `C2S_InputState`.
  - On the server tick, if a player's shoot flag is active and their current weapon cooldown is 0:
    - Spawn a Bullet entity at the turret muzzle position.
    - Assign the bullet a unique ID, owner ID, velocity vector, and weapon type.
    - Set the player's weapon cooldown timer.
- [ ] **6.2 Projectile Simulation & Obstacle Hits**
  - Projectiles are updated on the server during each tick.
  - Move bullet: `position += velocity * TICK_TIME`.
  - Test bullet collisions:
    - If a bullet hits a solid wall: destroy the bullet. If it is a destructible wall, subtract wall health; destroy the wall if health drops to 0.
    - If a bullet hits a player tank (other than the owner):
      - Inflict damage to target player's health.
      - Destroy the bullet.
      - Trigger an explosion event.
- [ ] **6.3 Bullet Replication**
  - Include bullet array states in the `S2C_WorldSnapshot`.
  - The client parses the bullet array and draws them on screen.
  - Implement bullet spawn visual effects locally on client immediately to make shooting feel snappy.
- [ ] **6.4 Death & Respawn Loop**
  - If a player's health reaches 0 on the server:
    - Set player state to `Dead`.
    - Broadcast an `S2C_EventNotification` packet (e.g., `Player A killed Player B`).
    - Increment target player's death count and source player's kill count.
    - Wait 5 seconds, then respawn the dead player tank at a random spawn point with full health.

---

### Phase 7: Weapons & Powerups Sandbox
**Goal**: Add different projectile behaviors (machine gun, rockets) and implement spawn systems for weapons and powerups.

- [ ] **7.1 Weapon Custom Behaviors**
  - **Machine Gun**: Modify firing loop to support continuous spray. Apply high fire rates and small random angular spread offsets to bullet velocities.
  - **Heavy Rocket**: Modify projectile destruction logic. When a rocket hit is registered:
    - Check overlap circles around the rocket impact point.
    - Apply falloff damage to all tanks within the splash radius.
  - **Laser Railgun**: Implement server-side hitscan raycast:
    - Perform a raycast query against all player circles and wall AABBs along the turret line of sight.
    - Register immediate damage on the closest hit player.
- [ ] **7.2 Powerup Spawners**
  - Add powerup entities to the server world state.
  - Create powerup spawning pads at fixed coordinates.
  - If a pad is empty, start a respawn timer (e.g., 15 seconds) after which a random powerup (Shield, Speed, Health, or Weapon Ammo) appears.
- [ ] **7.3 Powerup Pickup and State Effects**
  - Detect tank-to-powerup circle intersections on the server.
  - On pickup:
    - Destroy powerup entity.
    - Apply modifiers to tank states (e.g., activate temporary invincibility flag for Shield, increase top speed multiplier for Speed Boost, set active weapon type).
    - Send an event packet to play pickup sounds on the client.

---

### Phase 8: HUD, UI, Audio, & Visual Polish
**Goal**: Create an engaging player interface, implement animations, particle effects, and sound playback.

- [ ] **8.1 Tank Sprites and Rotation**
  - Replace raw shapes with 2D sprites.
  - Render the tank base sprite rotated by the chassis angle.
  - Render the turret sprite centered on the chassis but rotated by the turret angle.
  - Add animation frames for moving treads.
- [ ] **8.2 Particle System**
  - Implement a local client-side particle engine:
    - Tread tracks left on the ground.
    - Muzzle flashes when firing.
    - Particle explosions (debris, fire, smoke) when tanks or walls break.
    - Spark particles when bullets hit indestructible walls.
- [ ] **8.3 HUD & In-game UI**
  - Draw a floating health bar above all visible tanks.
  - Render a local player HUD at the bottom of the screen:
    - Large health bar.
    - Currently selected weapon icon and ammunition counters.
    - Active powerup timers (visual countdown bar for Shield/Speed).
  - Scoreboard overlay (toggled with `TAB`) listing all players, ping, kills, and deaths sorted.
- [ ] **8.4 Sound Design**
  - Load audio files using raylib's audio module.
  - Play positional 2D audio cues (sounds attenuate or pan depending on distance from local player) for:
    - Shooting (different sounds for Cannon, Machine Gun, Rocket, and Laser).
    - Explosions and impact hits.
    - Powerup collections.
    - Tank motor hums.

---

### Phase 9: Stress Testing, Optimization, & Lag Simulation
**Goal**: Ensure performance remains stable under full load (32 players) and poor network conditions.

- [ ] **9.1 Artificial Lag Simulation**
  - Add client/server configuration commands to mock bad network environments:
    - Simulate latency: delay processing of outgoing/incoming ENet packets by $50\text{ms}$ - $200\text{ms}$.
    - Simulate packet loss: drop a random $1\%$ - $10\%$ of unreliable packets.
  - Test client reconciliation and entity interpolation under these conditions to tune parameters.
- [ ] **9.2 Packet Payload Optimization**
  - Compress network snapshots:
    - Use byte-packing (e.g. compress float coordinates into 16-bit integers relative to map bounds).
    - Delta compression: only send information about entities that have changed since the client's last acknowledged tick.
- [ ] **9.3 Headless Bot Simulation**
  - Implement a command-line flag on the client to launch "headless bot clients".
  - These bots connect to the server, move randomly, and auto-aim/shoot at the nearest player.
  - Spin up 31 bot clients on one computer connecting to a local server to test performance, CPU overhead, and network bandwidth stability at scale.

---

## 4. Development Milestones & Success Criteria

### Milestone 1: Networking & Handshake (Phase 1)
- **Deliverable**: Server executable running in console; Client executable starting a window and establishing connection.
- **Success Criteria**: Client console says "Connected to server!" and server log prints client's incoming IP address. Closing the client prints "Client disconnected." on the server console.

### Milestone 2: Movement & State Synchronization (Phases 2-3)
- **Deliverable**: Multiple clients can connect. They can see each other represented as colored boxes on screen.
- **Success Criteria**: Moving tank A on Client 1 updates tank A's position on Client 2's screen. The movement is visible and responds immediately.

### Milestone 3: Client Prediction & Smoothing (Phase 4)
- **Deliverable**: Seamless local movement simulation even when latency is set to 150ms.
- **Success Criteria**: Local player moves with zero delay. Moving other tanks shows no stuttering or snapping, thanks to smooth interpolation.

### Milestone 4: Collision & Physics Sandbox (Phase 5)
- **Deliverable**: Map filled with obstacles. Tanks cannot pass through walls or other tanks.
- **Success Criteria**: Driving directly into a wall stops the tank. Sliding along walls feels smooth without getting stuck.

### Milestone 5: Combat & Scoreboards (Phases 6-7)
- **Deliverable**: Fully playable combat cycle with health, bullets, respawns, weapons, powerups, and scoreboard.
- **Success Criteria**: Tank shooting a bullet hits another tank, decreasing health. If health goes to 0, tank is destroyed, kill feeds show the event, scores update, and tank respawns 5 seconds later. Powerups can be collected to gain buffs.

### Milestone 6: Polish, UI, Sound & Scale (Phases 8-9)
- **Deliverable**: Complete polished game client and server ready for distribution.
- **Success Criteria**: Beautiful tank sprites, tread marks, particle explosions, immersive 2D audio, functional matchmaking/lobby screen, and stable performance with 32 simulated players.