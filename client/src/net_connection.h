#pragma once

#include "external/fix_win32_compatibility.h"
#include "raylib.h"

#include <cstdint>
#include "event_source.h"

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

	char* GetPlayerName();

	void BeginConnect(const char* address, uint16_t port);

	void Disconnect();

	void Update();

	bool HadTimeout();

	ConnectionState GetState();

	uint64_t GetRTT();

	Vector2 GetSpawn();

	struct Events
	{
		EventSoure<bool> OnConnect;
		EventSoure<uint64_t> OnJoin;
		EventSoure<Vector2> OnSpawn;
	};

	Events& GetEvents();
}