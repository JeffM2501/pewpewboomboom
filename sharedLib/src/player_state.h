#pragma once
#include "external/fix_win32_compatibility.h"
#include "text_utils.h"
#include "raylib.h"
#include "enet.h"
#include "data_utils.h"
#include "constants.h"


struct PlayerTransform
{
	Vector2 Position = { 0.0f, 0.0f };
	float Rotation[2] = { 0.0f, 0.0f };
	Vector2 Velocity = { 0.0f, 0.0f };
};

struct PlayerState
{
	uint64_t PlayerID = uint64_t(-1);
	ENetPeer* Peer = nullptr;
	FixedSizeString<kMaxNameSize> Name;
	int Team = -1;
	PlayerTransform Transform;

	// Transform history.....
};

