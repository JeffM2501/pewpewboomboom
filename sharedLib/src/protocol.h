#pragma once

#include <cstdint>

enum class PacketType : uint8_t
{
	C2S_Ping = 1,
	S2C_Pong = 2,
	C2S_Goodbye = 3,
};

#pragma pack(push, 1)
struct C2S_Ping
{
	uint8_t type = static_cast<uint8_t>(PacketType::C2S_Ping);
	uint64_t clientTimeMs = 0;
};

struct S2C_Pong
{
	uint8_t type = static_cast<uint8_t>(PacketType::S2C_Pong);
	uint64_t clientTimeMs = 0;
	uint64_t serverTimeMs = 0;
};

struct C2S_Goodbye
{
	enum class Reason : uint8_t
	{
		Disconnect,
		Transfer,
	};
    uint8_t type = static_cast<uint8_t>(PacketType::C2S_Goodbye);
	Reason reason = Reason::Disconnect;
};
#pragma pack(pop)
