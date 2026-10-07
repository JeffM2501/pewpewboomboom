#pragma once

#include <cstdint>
#include <cstddef>

static constexpr int kSimulationTickRate = 60;
static constexpr float kSimulationTickTime = 1.0f / float(kSimulationTickRate);
static constexpr int kDefaultTickRate = kSimulationTickRate;
static constexpr float kDefaultTickTime = kSimulationTickTime;

static constexpr int kNetworkSendRate = 30;
static constexpr int kNetworkSendIntervalTicks = kSimulationTickRate / kNetworkSendRate;
static constexpr float kNetworkSendIntervalTime = 1.0f / float(kNetworkSendRate);

static constexpr uint64_t kRemotePlayerHistoryOffsetTicks = 3 * kNetworkSendIntervalTicks; // 6 ticks = 100ms interpolation buffer
static constexpr uint64_t kLagCompensationToleranceTicks = kNetworkSendIntervalTicks + 1;  // 3 ticks tolerance window

static constexpr int kMaxPlayers = 32;

static constexpr uint32_t kProtocolVersion = 7; // version number of the network protocol, increment this when making breaking changes to packets

static constexpr size_t kMaxNameSize = 32;
static constexpr size_t kMaxChatLineSize = 256;

static constexpr float kRegularShotCooldown = 0.5f;
static constexpr float kRegularShotDamage = 25.0f;
static constexpr float kMachineGunCooldown = 0.175f;
static constexpr float kMachineGunDamage = 3.0f;
static constexpr float kMachineGunTracerDuration = 0.25f;

static constexpr float kBulletRadius = 0.5f;
static constexpr float kBulletLifetime = 3.0f;
