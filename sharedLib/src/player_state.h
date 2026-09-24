#pragma once
#include "external/fix_win32_compatibility.h"
#include "text_utils.h"
#include "enet.h"
#include "raylib.h"
#include "data_utils.h"
#include "constants.h"

#include <map>

struct PlayerTransform
{
	Vector2 Position = { 0.0f, 0.0f };
	float Rotation[2] = { 0.0f, 0.0f };
	Vector2 Velocity = { 0.0f, 0.0f };
};

struct PlayerMovementRules
{
    float MaxSpeed = 20;
    float TurnSpeed = 90.0f; // degrees per second
    float BoostMultiplier = 2.0f;
};

struct PlayerState
{
	uint64_t PlayerID = uint64_t(-1);
	ENetPeer* Peer = nullptr;
	FixedSizeString<kMaxNameSize> Name;
	int Team = -1;

    PlayerMovementRules Rules;
    float CollisionRadius = 3.0f;

	PlayerTransform Transform;

	// Transform history.....
    std::map<uint64_t, PlayerTransform> TransformHistory;
};

struct InputState
{
    float Foward = 0.0f;
    float Turn = 0.0f;
    bool Boost = false;
    bool Shoot = false;
    float TurretAngle = 0.0f;
};

Vector2 UpdatePlayerTransform(PlayerTransform& transform, const InputState& input, float deltaTime, PlayerMovementRules& rules);

