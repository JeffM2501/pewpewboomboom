# PewPewBoomBoom - Project Task Backlog (Jira-Style)

This document contains the complete, Jira-formatted issue backlog for **PewPewBoomBoom**. It is designed to track development progress across all 9 project epics.

---

## Progress Overview
- [x] **EPIC 1: Infrastructure & ENet Networking Foundations** (4 / 4 Completed)
- [ ] **EPIC 2: Network Protocol & Binary Serialization** (0 / 3 Completed)
- [ ] **EPIC 3: Fixed-Tick Server Simulation & Rendering Skeleton** (0 / 3 Completed)
- [ ] **EPIC 4: Client Prediction, Server Reconciliation & Interpolation** (0 / 5 Completed)
- [ ] **EPIC 5: 2D Physics & Collision Detection** (0 / 3 Completed)
- [ ] **EPIC 6: Combat Engine, Projectile Pooling & Respawn Flow** (0 / 3 Completed)
- [ ] **EPIC 7: Custom Weapon Variants & Powerup Spawners** (0 / 3 Completed)
- [ ] **EPIC 8: Audio, Visual Polish & User Interface** (0 / 4 Completed)
- [ ] **EPIC 9: Performance Optimization, Network Stressing & Bots** (0 / 3 Completed)

---

## EPIC 1: Infrastructure & ENet Networking Foundations

### [PEW-101] Build System & ENet Dependency Integration
* **Status**: `[x] DONE`
* **Issue Type**: Task
* **Component**: `build-system`, `sharedLib`
* **Priority**: Highest
* **Story**: As a developer, I want ENet integrated into the Premake workspace so that client and server targets link cross-platform networking headers without compilation errors.
* **Technical Acceptance Criteria**:
  - [x] ENet header is embedded into `sharedLib/include/`.
  - [x] `premake5.lua` updated for `client`, `server`, and `sharedLib` to reference ENet header search paths.
  - [x] Platform sockets (`ws2_32.lib`, `winmm.lib` on Windows) linked to executable build targets.
* **Definition of Done**: Project builds cleanly with zero compilation or linker errors.

---

### [PEW-102] Headless Server Host Initialization & Polling Loop
* **Status**: `[x] DONE`
* **Issue Type**: Task
* **Component**: `server`, `networking`
* **Priority**: Highest
* **Story**: As a server administrator, I want a headless ENet host server so that multiple client peers can establish UDP connections.
* **Technical Acceptance Criteria**:
  - [x] Initialize ENet via `enet_initialize()` inside `server/src/main.cpp`.
  - [x] Create an `ENetHost` listening on port `7777` supporting up to 32 concurrent peers and 2 channels.
  - [x] Implement a non-blocking `enet_host_service` loop logging peer connection and disconnection events with timestamped output.
* **Definition of Done**: Launching `server.exe` opens a console process that listens on port `7777` without freezing or leaking socket descriptors.

---

### [PEW-103] Client ENet Connection & Raylib 2D Window Setup
* **Status**: `[x] DONE`
* **Issue Type**: Task
* **Component**: `client`, `networking`
* **Priority**: High
* **Story**: As a player, I want the client application to initialize a Raylib window and attempt a non-blocking network connection to the server.
* **Technical Acceptance Criteria**:
  - [*] Initialize Raylib 2D window (`1280x720` resolution, target 144 FPS).
  - [*] Initialize ENet client host and issue `enet_host_connect()` targeting `127.0.0.1:7777`.
  - [*] Draw connection lifecycle state ("Connecting...", "Connected", "Disconnected") on screen using Raylib `DrawText()`.
* **Definition of Done**: Running `client.exe` opens a Raylib window, connects to a running `server.exe`, and displays "Connected" on screen.

---

### [PEW-104] Reliable Echo Handshake & RTT Ping Test
* **Status**: `[x] DONE`
* **Issue Type**: Technical Spike
* **Component**: `sharedLib`, `networking`
* **Priority**: High
* **Story**: As a network engineer, I want to verify reliable packet delivery on ENet Channel 0 by measuring round-trip time (RTT).
* **Technical Acceptance Criteria**:
  - [x] Define `C2S_Ping` and `S2C_Pong` packet IDs in `sharedLib/include/Protocol.h`.
  - [x] Upon connection, client transmits a reliable `C2S_Ping` containing local system time.
  - [x] Server receives `C2S_Ping` and immediately replies with `S2C_Pong`.
  - [x] Client calculates latency `(CurrentTime - PingSentTime)` and prints RTT in ms.
* **Definition of Done**: Client console displays calculated RTT in milliseconds with 0% packet drop on Channel 0.

---

## EPIC 2: Network Protocol & Binary Serialization

### [PEW-201] Shared Constants & Configuration Header
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `sharedLib`
* **Priority**: High
* **Story**: As a developer, I want a single source of truth for game constants so that client and server operate under identical simulation rules.
* **Technical Acceptance Criteria**:
  - [ ] Create `sharedLib/include/Common.h`.
  - [ ] Define `constexpr` values for `TICK_RATE = 60`, `TICK_TIME = 1.0f / 60.0f`, `MAX_PLAYERS = 32`, `MAX_BULLETS = 256`, `MAP_BOUNDS = 2000.0f`, `TANK_RADIUS = 24.0f`, `BULLET_RADIUS = 4.0f`.
* **Definition of Done**: Header compiles in both `client` and `server` without macro collision errors.

---

### [PEW-202] High-Performance Bitstream BufferWriter / BufferReader API
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `sharedLib`
* **Priority**: Highest
* **Story**: As a network programmer, I want a custom bitstream reader and writer to serialize game state into contiguous byte buffers without padding or endian misalignment.
* **Technical Acceptance Criteria**:
  - [ ] Create `BufferWriter` and `BufferReader` classes in `sharedLib/include/BufferStream.h`.
  - [ ] Add explicit templated methods `Write<T>()` and `Read<T>()` supporting `uint8_t`, `uint16_t`, `uint32_t`, and `float`.
  - [ ] Include array bounds safety checks to prevent buffer overflow exceptions.
* **Definition of Done**: Unit test verifies that writing diverse primitive types into `BufferWriter` and reading via `BufferReader` produces identical values.

---

### [PEW-203] Binary Network Packet Specifications & Struct Serializers
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `sharedLib`, `networking`
* **Priority**: Highest
* **Story**: As a network engineer, I want explicit binary serializers for all client-server packet types.
* **Technical Acceptance Criteria**:
  - [ ] Implement serialization routines in `sharedLib/src/Protocol.cpp` for `C2S_JoinRequest`, `S2C_JoinResponse`, `C2S_InputState`, `S2C_WorldSnapshot`, `S2C_EventNotification`, `C2S_ChatMessage`, `S2C_PlayerDisconnected`, and `C2S_RespawnRequest`.
  - [ ] Keep `PlayerNetState` payload $\le 23$ bytes per player.
  - [ ] Keep `BulletNetState` payload $\le 13$ bytes per projectile.
* **Definition of Done**: Roundtrip serialization unit test passes for all 8 packet structures.

---

## EPIC 3: Fixed-Tick Server Simulation & Rendering Skeleton

### [PEW-301] Fixed-Timestep Accumulator Game Loop on Server
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`
* **Priority**: Highest
* **Story**: As a server engineer, I want the server loop to update at a deterministic 60Hz tick rate to ensure reproducible physics.
* **Technical Acceptance Criteria**:
  - [ ] Implement an accumulator-based loop in `server/src/main.cpp` using `std::chrono::high_resolution_clock`.
  - [ ] Trigger `ServerTick()` exactly once every $16.66\text{ms}$ ($1/60\text{s}$).
  - [ ] Clamp maximum accumulator frame time to $0.25\text{s}$ to prevent server death loops under heavy CPU load.
* **Definition of Done**: Server logs confirm exactly 60 tick iterations per second over a 60-second test run.

---

### [PEW-302] Server Authoritative State Storage & Snapshot Broadcast
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`, `networking`
* **Priority**: High
* **Story**: As a server developer, I want the server to maintain active entity states and broadcast world snapshots to connected clients every tick.
* **Technical Acceptance Criteria**:
  - [ ] Maintain a fixed-size contiguous array `std::array<PlayerState, MAX_PLAYERS>` in `server/include/World.h`.
  - [ ] In `ServerTick()`, advance world tick counter and assemble `S2C_WorldSnapshot`.
  - [ ] Broadcast `S2C_WorldSnapshot` to all connected peers via ENet Channel 1 (unreliable).
* **Definition of Done**: Network packet analyzer verifies server sends a 60Hz stream of `S2C_WorldSnapshot` packets to connected clients.

---

### [PEW-303] Client 2D Viewport Camera & Primitive Entity Rendering
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`
* **Priority**: High
* **Story**: As a player, I want to see connected tanks rendered as 2D shapes inside a scrollable camera viewport.
* **Technical Acceptance Criteria**:
  - [ ] Configure Raylib `Camera2D` to center on the local player tank position.
  - [ ] Draw a background grid representing arena coordinates `(2000x2000)`.
  - [ ] Deserialize incoming `S2C_WorldSnapshot` packets and draw 2D rectangles for each player tank at their server coordinates.
* **Definition of Done**: Connecting two clients to a server displays two colored rectangles in the Raylib window.

---

## EPIC 4: Client Prediction, Server Reconciliation & Interpolation

### [PEW-401] Client Input Sampling & Unreliable Transmission
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `networking`
* **Priority**: Highest
* **Story**: As a player, I want my keyboard and mouse inputs sampled every frame and transmitted to the server.
* **Technical Acceptance Criteria**:
  - [ ] Sample `W`, `A`, `S`, `D`, and `Left Mouse` state every frame.
  - [ ] Compute turret rotation angle in radians using `GetScreenToWorld2D(GetMousePosition())`.
  - [ ] Pack state into `C2S_InputState` with current `clientTick` and send over ENet Channel 1.
* **Definition of Done**: Server receives and prints input flags and turret angles from client at 60Hz.

---

### [PEW-402] Server Player Kinematics & Movement Simulation
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`, `physics`
* **Priority**: Highest
* **Story**: As a server engineer, I want the server to simulate tank acceleration, friction, and rotation based on client inputs.
* **Technical Acceptance Criteria**:
  - [ ] Read `C2S_InputState` for each connected peer in `ServerTick()`.
  - [ ] Apply linear acceleration along chassis forward vector when `W`/`S` pressed.
  - [ ] Apply rotational velocity when `A`/`D` pressed.
  - [ ] Apply linear drag friction and update position `pos += velocity * TICK_TIME`.
* **Definition of Done**: Tank position moves smoothly on server when inputs are applied.

---

### [PEW-403] Local Client Prediction & Input Ring Buffer
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `physics`
* **Priority**: High
* **Story**: As a player, I want my local tank to respond instantly to inputs without waiting for server network latency.
* **Technical Acceptance Criteria**:
  - [ ] Implement a circular ring buffer `InputHistory[128]` storing `(tick, input, predictedPosition, predictedVelocity)` on the client.
  - [ ] Apply movement physics locally immediately on input sampling frame.
* **Definition of Done**: Local tank moves instantly on WASD key press even when simulated latency is added.

---

### [PEW-404] Server Snapshot Reconciliation Loop
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `networking`
* **Priority**: High
* **Story**: As a player, I want my local predicted tank position reconciled against authoritative server snapshots to prevent cheating and desync.
* **Technical Acceptance Criteria**:
  - [ ] Upon receiving `S2C_WorldSnapshot` for tick $T$, compare server position vs `InputHistory[T]`.
  - [ ] If error exceeds threshold `0.05` units: snap local position to server position and re-simulate physics for all stored inputs from tick $T+1$ to current client tick.
* **Definition of Done**: Introducing artificially delayed server packets causes local tank to correct smoothly without persistent position drift.

---

### [PEW-405] Remote Entity Snapshot Buffering & Interpolation
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `networking`
* **Priority**: High
* **Story**: As a player, I want remote tanks to move smoothly on screen without stuttering.
* **Technical Acceptance Criteria**:
  - [ ] Buffer received server snapshots on client with a $100\text{ms}$ ($6$ ticks) interpolation delay.
  - [ ] Interpolate remote tank position between `Snapshot[k]` and `Snapshot[k+1]` using Linear Interpolation (LERP).
  - [ ] Interpolate chassis and turret angles using shortest-path angular lerp.
* **Definition of Done**: Other connected players move smoothly across the viewport without visual jitter.

---

## EPIC 5: 2D Physics & Collision Detection

### [PEW-501] Primitive 2D Physics Intersection & Resolution Library
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `sharedLib`, `physics`
* **Priority**: High
* **Story**: As a developer, I want a lightweight C++ collision library for Circle-vs-Circle and Circle-vs-AABB intersections.
* **Technical Acceptance Criteria**:
  - [ ] Create `sharedLib/include/Physics.h` and `sharedLib/src/Physics.cpp`.
  - [ ] Implement `CheckCircleCircleCollision()`, `ResolveCircleCircleCollision()`.
  - [ ] Implement `CheckCircleAABBCollision()`, `ResolveCircleAABBCollision()`.
  - [ ] Return penetration depth vectors and contact normals.
* **Definition of Done**: Unit tests verify correct contact normal and pushout resolution vectors for overlapping geometry.

---

### [PEW-502] Static Arena Map Layout & Destructible Wall Data
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `sharedLib`
* **Priority**: Medium
* **Story**: As a level designer, I want a structured arena map definition containing outer boundary walls, indestructible barriers, and destructible crates.
* **Technical Acceptance Criteria**:
  - [ ] Create `sharedLib/include/MapData.h`.
  - [ ] Define outer perimeter AABB walls for `2000x2000` arena bounds.
  - [ ] Add static array of interior concrete wall AABBs and destructible wooden crate AABBs with HP fields.
* **Definition of Done**: Arena layout data loads cleanly into both client and server memory.

---

### [PEW-503] Tank-to-Wall and Tank-to-Tank Collision Integration
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`, `physics`
* **Priority**: High
* **Story**: As a server engineer, I want tanks to collide with arena walls and other tanks so they cannot pass through solid geometry.
* **Technical Acceptance Criteria**:
  - [ ] In `ServerTick()`, test each player tank circle against all wall AABBs and resolve overlap.
  - [ ] Test player tank circle against all other player tank circles and apply pushout resolution.
  - [ ] Integrate same collision logic into client prediction loop.
* **Definition of Done**: Driving a tank into a wall or another tank brings it to a physical stop without clipping through geometry.

---

## EPIC 6: Combat Engine, Projectile Pooling & Respawn Flow

### [PEW-600] Client Firing Input & Server Bullet Pool Management
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`, `client`
* **Priority**: High
* **Story**: As a player, I want pressing the Left Mouse Button to fire bullets from my tank's turret.
* **Technical Acceptance Criteria**:
  - [ ] Pre-allocate a contiguous `std::array<Bullet, 256>` pool on server.
  - [ ] When client input `Shoot` flag is set and weapon cooldown is 0, spawn a bullet entity at turret muzzle position.
  - [ ] Assign velocity vector along turret orientation angle.
* **Definition of Done**: Clicking left mouse button spawns a bullet traveling in the direction of the turret.

---

### [PEW-601] Server Projectile Collision & Destruction Lifecycle
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`, `physics`
* **Priority**: High
* **Story**: As a server developer, I want bullets to move every tick, hit walls or players, and trigger damage/destruction events.
* **Technical Acceptance Criteria**:
  - [ ] Advance bullet position `pos += velocity * TICK_TIME` on `ServerTick()`.
  - [ ] Check bullet collisions against wall AABBs (destroy bullet; if destructible wall, subtract wall HP).
  - [ ] Check bullet collisions against enemy tank circles (destroy bullet, subtract 25 HP from target tank).
* **Definition of Done**: Shooting a wall destroys the bullet; shooting an enemy tank reduces its health bar.

---

### [PEW-602] Player Health, Death Event Broadcast & Respawn Timer
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`, `networking`
* **Priority**: High
* **Story**: As a player, I want my tank to explode when health reaches 0, update scores, and respawn after a delay.
* **Technical Acceptance Criteria**:
  - [ ] When player health reaches 0: set state to `Dead`, increment attacker kills, increment victim deaths.
  - [ ] Broadcast `S2C_EventNotification` kill message over ENet Channel 0 (reliable).
  - [ ] Start 5-second server countdown timer, then reset health to 100% and move to a random spawn point.
* **Definition of Done**: Reducing a player's health to 0 logs a kill event, disables their tank, and respawns them 5 seconds later.

---

## EPIC 7: Custom Weapon Variants & Powerup Spawners

### [PEW-701] Specialized Weapon Behaviors (MG, Rocket, Railgun)
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`, `physics`
* **Priority**: Medium
* **Story**: As a player, I want distinct weapon mechanics (Machine Gun, Heavy Rocket, Laser Railgun) for varied tactical gameplay.
* **Technical Acceptance Criteria**:
  - [ ] **Rapid-Fire MG**: High fire rate ($0.08\text{s}$ cooldown), low damage, 5-degree random angular spread.
  - [ ] **Heavy Rocket**: Slow velocity, high damage, explodes on contact dealing area-of-effect damage to tanks within 100px radius.
  - [ ] **Laser Railgun**: Instant hitscan line-trace raycast penetrating targets along line of sight.
* **Definition of Done**: All 4 weapon types function with unique projectile speeds, fire rates, and hit detection logic.

---

### [PEW-702] World Powerup Spawner Pads & Respawn Mechanics
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`
* **Priority**: Medium
* **Story**: As a player, I want powerup items to spawn at designated pads around the map.
* **Technical Acceptance Criteria**:
  - [ ] Define static array of `PowerupSpawner` pads in map data.
  - [ ] Every 15 seconds, spawn a random powerup (Health Repair, Speed Boost, Shield Generator, Weapon Ammo Crate) on empty pads.
  - [ ] Include active powerups in `S2C_WorldSnapshot`.
* **Definition of Done**: Powerups appear on map spawner pads periodically during gameplay.

---

### [PEW-703] Powerup Pickup Collision & Player Buff Timers
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `server`, `physics`
* **Priority**: Medium
* **Story**: As a player, driving over a powerup should grant temporary buffs or restore health.
* **Technical Acceptance Criteria**:
  - [ ] Detect player tank circle intersection with active powerups on server.
  - [ ] **Health Repair**: Restore 50 HP immediately.
  - [ ] **Speed Boost**: Grant 1.5x top speed for 8 seconds.
  - [ ] **Shield Generator**: Grant complete invincibility for 5 seconds.
  - [ ] Broadcast pickup event and active buff bitfield in world snapshot.
* **Definition of Done**: Collecting a Speed Boost icon increases tank movement speed for 8 seconds before expiring.

---

## EPIC 8: Audio, Visual Polish & User Interface

### [PEW-801] Tank Chassis / Turret Sprite Rendering & Track Animations
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `ui`
* **Priority**: Medium
* **Story**: As a player, I want tanks rendered with detailed 2D sprites and animated tread tracks.
* **Technical Acceptance Criteria**:
  - [ ] Load tank chassis and turret textures using Raylib `LoadTexture()`.
  - [ ] Render chassis rotated by `ChassisAngle`; render turret centered on chassis rotated by `TurretAngle`.
  - [ ] Draw moving tread mark sprites behind tanks as they move.
* **Definition of Done**: Tanks render with distinct chassis and turret sprite rotations and leave tread marks on the arena floor.

---

### [PEW-802] Zero-Allocation Client Particle System
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`
* **Priority**: Medium
* **Story**: As a player, I want visual particle effects for muzzle flashes, rocket smoke, and explosions.
* **Technical Acceptance Criteria**:
  - [ ] Implement a pre-allocated 1000-particle pool on client.
  - [ ] Spawn particles for muzzle flashes, explosion sparks, bullet impact debris, and smoke.
  - [ ] Update and render particles using Raylib additive blending modes (`BLEND_ADDITIVE`).
* **Definition of Done**: Firing weapons and destroying crates spawns visual particle explosions without frame rate drops.

---

### [PEW-803] In-Game Player HUD & Tab Scoreboard Overlay
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `ui`
* **Priority**: High
* **Story**: As a player, I want an in-game HUD showing my health, weapon status, and a scoreboard.
* **Technical Acceptance Criteria**:
  - [ ] Draw floating health bars above all visible tanks.
  - [ ] Draw bottom-screen HUD with health bar, current weapon icon, ammo count, and active buff timers.
  - [ ] Implement a TAB scoreboard overlay showing player names, ping, kills, and deaths.
* **Definition of Done**: Pressing TAB displays a sorted scoreboard overlay; HUD updates health and ammo counters in real time.

---

### [PEW-804] 2D Positional Spatial Audio System
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `audio`
* **Priority**: Medium
* **Story**: As a player, I want game sound effects spatially attenuated based on distance from my tank.
* **Technical Acceptance Criteria**:
  - [ ] Initialize Raylib audio system (`InitAudioDevice()`).
  - [ ] Load sound effects for shooting variants, explosions, powerup pickup, and engine hums.
  - [ ] Calculate volume attenuation and stereo panning based on distance/direction to local player camera.
* **Definition of Done**: Off-screen explosions sound quiet and panned to the side, while local weapon shots play at full volume.

---

## EPIC 9: Performance Optimization, Network Stressing & Bots

### [PEW-901] Artificial Latency & Packet Loss Debug Wrapper
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `networking`
* **Priority**: Low
* **Story**: As a developer, I want to simulate high latency and packet loss to test reconciliation stability.
* **Technical Acceptance Criteria**:
  - [ ] Add network simulator wrapper around ENet packet handling.
  - [ ] Add debug UI sliders to artificially delay incoming/outgoing packets by $0\text{-}250\text{ms}$.
  - [ ] Add slider to drop $0\%\text{-}15\%$ of unreliable packets randomly.
* **Definition of Done**: Adjusting latency slider to 150ms delays incoming server packets without crashing client reconciliation.

---

### [PEW-902] World Snapshot Delta Compression
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `sharedLib`, `networking`
* **Priority**: Low
* **Story**: As a network engineer, I want world snapshot packets delta-compressed to conserve bandwidth.
* **Technical Acceptance Criteria**:
  - [ ] Include a change bitmask in `S2C_WorldSnapshot` indicating which entities changed since client's last acknowledged tick.
  - [ ] Compress float coordinates into 16-bit fixed-point integers relative to map dimensions.
* **Definition of Done**: Network bandwidth consumption per client drops by >40% during static or low-movement gameplay.

---

### [PEW-903] Headless Bot Client Generator for 32-Player Load Testing
* **Status**: `[ ] TO DO`
* **Issue Type**: Task
* **Component**: `client`, `networking`
* **Priority**: Low
* **Story**: As a server engineer, I want to spawn 31 automated headless bot clients to stress-test server CPU and network bandwidth under full load.
* **Technical Acceptance Criteria**:
  - [ ] Add `--bot` command-line flag to `client`.
  - [ ] When run with `--bot`, launch headless client without Raylib GUI window.
  - [ ] Bot logic: move randomly around map, auto-aim and fire at nearest player tank.
  - [ ] Support spawning 30 bot processes via shell script.
* **Definition of Done**: Server maintains steady 60Hz tick rate with 31 connected bot clients actively moving and shooting.
