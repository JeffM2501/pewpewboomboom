#pragma once

#include <cstdint>

static constexpr int kDefaultTickRate = 60;
static constexpr float kDefaultTickTime = 1.0f / 60;
static constexpr int kMaxPlayers = 32;

static constexpr uint32_t kProtocolVersion = 4; // version number of the network protocol, increment this when making breaking changes to packets

static constexpr size_t kMaxNameSize = 32;
static constexpr size_t kMaxChatLineSize = 256;

static constexpr float kRegularShotCooldown = 0.35f;
static constexpr float kRegularShotDamage = 25.0f;
static constexpr float kMachineGunCooldown = 0.175f;
static constexpr float kMachineGunDamage = 6.25f;
static constexpr float kMachineGunTracerDuration = 0.25f;
