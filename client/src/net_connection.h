#pragma once
#include <cstdint>

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

	void BeginConnect(const char* address, uint16_t port);

	void Disconnect();

	void Update();

	bool HadTimeout();

	ConnectionState GetState();

	uint64_t GetRTT();
}