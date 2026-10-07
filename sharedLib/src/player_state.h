#pragma once
#if defined(_WIN32)
#include "external/fix_win32_compatibility.h"
#endif
#include "text_utils.h"
#include "enet.h"
#include "raylib.h"
#include "data_utils.h"
#include "constants.h"

#include <map>
#include <cmath>

enum class TeamColors
{
    Blue = 0,
    Red = 1,
    Purple = 2,
    Yellow = 3,
    White = 4,
    Black = 5,
    Green = 6,
    MAX
};

inline float LerpAngleDeg(float a, float b, float t)
{
	float diff = fmodf(b - a + 180.0f, 360.0f);
	if (diff < 0.0f)
	{
		diff += 360.0f;
	}
	diff -= 180.0f;
	return a + diff * t;
}

struct PlayerTransform
{
	Vector2 Position = { 0.0f, 0.0f };
	float Rotation[2] = { 0.0f, 0.0f };
	Vector2 Velocity = { 0.0f, 0.0f }; // world units per second
	float AngularVelocity[2] = { 0.0f, 0.0f }; // degrees per second for the body and turret rotation
};

// shortest signed difference between two angles in degrees, result is in [-180, 180]
inline float AngleDiffDeg(float a, float b)
{
	float diff = fmodf(b - a + 180.0f, 360.0f);
	if (diff < 0.0f)
	{
		diff += 360.0f;
	}
	return diff - 180.0f;
}

// dead reckons a transform forward in time using its linear and angular velocities, this is what remote clients predict between updates
PlayerTransform PredictTransform(const PlayerTransform& transform, float deltaTime);

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

    uint8_t Health = 100;
    uint8_t MaxHealth = 100;
    bool IsDead = false;
    float WeaponCooldown = 0.0f;
    float MachineGunCooldown = 0.0f;
    float FractionalHealth = 100.0f;
    float RespawnTimer = 0.0f;
    uint16_t Kills = 0;
    uint16_t Deaths = 0;

	// Transform history.....
    std::map<uint64_t, PlayerTransform> TransformHistory;

    PlayerTransform GetTransformAtTick(uint64_t tick) const;
};

struct InputState
{
    float Foward = 0.0f;
    float Turn = 0.0f;
    bool Boost = false;
    bool Shoot = false;
    bool ShootMachineGun = false;
    float TurretAngle = 0.0f;
};

Vector2 UpdatePlayerTransform(PlayerTransform& transform, const InputState& input, float deltaTime, PlayerMovementRules& rules);

struct MachineGunHitBuildingEvent
{
	uint64_t ShooterID = 0;
	uint64_t BuildingID = 0;
	Vector2 HitPoint = { 0.0f, 0.0f };
	float Distance = 0.0f;
};

struct MachineGunHitTankEvent
{
	uint64_t ShooterID = 0;
	uint64_t TargetPlayerID = 0;
	Vector2 HitPoint = { 0.0f, 0.0f };
	float Damage = 0.0f;
	float Distance = 0.0f;
};
