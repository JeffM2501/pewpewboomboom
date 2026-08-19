#pragma once

static constexpr int kDefaultTickRate = 60;
static constexpr float kDefaultTickTime = 1.0f / 60;
static constexpr int kMaxPlayers = 32;

static constexpr uint32_t kProtocolVersion = 2; // version number of the network protocol, increment this when making breaking changes to packets

static constexpr size_t kMaxNameSize = 32;