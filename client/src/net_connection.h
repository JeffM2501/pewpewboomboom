#pragma once

#include "external/fix_win32_compatibility.h"
#include "raylib.h"

#include <vector>
#include <cstdint>
#include "event_source.h"
#include "text_utils.h"
#include "constants.h"
#include "player_list.h"

enum class ConnectionState
{
	Idle,
	Disconnected,
	Connecting,
	Connected,
};

namespace NetConnection
{
	void Init();
	void Shutdown();

	FixedSizeString<kMaxPlayers>& GetPlayerName();

	void BeginConnect(const char* address, uint16_t port);

	void Disconnect();

	void Update();

	bool HadTimeout();

	ConnectionState GetState();

	uint64_t GetRTT();

	Vector2 GetSpawn();

	uint64_t GetCurrentServerTick();

	struct Events
	{
		EventSoure<bool> OnConnect;
		EventSoure<uint64_t> OnJoin;
		EventSoure<Vector2> OnSpawn;
		EventSoure<uint64_t> OnTick;
	};

	Events& GetEvents();

	PlayerList& GetPlayerList();
}