#pragma once

#include <cstdint>

#include "constants.h"

enum class PacketType : uint8_t
{
	INVALID = 0,
	C2S_Ping = 1,
	S2C_Pong = 2,
	C2S_Goodbye = 3,
	C2S_JoinRequest = 4,
	S2C_JoinResponse = 5,
	C2S_InputState = 6,
	S2C_WorldSnapshot = 7,
	S2C_EventNotification = 8,
	C2S_ChatMessage = 9,
	S2C_PlayerJoined = 10,
	S2C_PlayerDisconnected = 11,
	C2S_RespawnRequest = 12,
	S2C_ChatMessage = 13
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
	uint64_t serverTick = 0;
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

struct C2S_JoinRequest
{
	uint8_t type = static_cast<uint8_t>(PacketType::C2S_JoinRequest);
	uint8_t protocolVersion = kProtocolVersion;
	char desriredName[kMaxNameSize] = {};
};

struct S2C_JoinResponse
{
	enum class Result : uint8_t
	{
		Failure = 0,
		Success = 1
	};
	uint8_t type = static_cast<uint8_t>(PacketType::S2C_JoinResponse);
	Result result = Result::Failure;

	uint64_t playerId = 0;
	float spawn[2] = { 0.0f, 0.0f };

	char actualName[kMaxNameSize] = {};
};

struct S2C_PlayerJoined
{
    uint8_t type = static_cast<uint8_t>(PacketType::S2C_PlayerJoined);
	uint64_t playerId;
	char name[kMaxNameSize] = {};
};

struct S2C_PlayerDisconnected
{
    uint8_t type = static_cast<uint8_t>(PacketType::S2C_PlayerDisconnected);

	uint64_t playerId;
    enum class Reason : uint8_t
    {
        Dissconnect = 0,
        Quit = 1
    };
	Reason reason = Reason::Quit;
};

struct C2S_ChatMessage
{
    uint8_t type = static_cast<uint8_t>(PacketType::C2S_ChatMessage);
	uint64_t senderId = 0;
	char message[kMaxChatLineSize] = {};
};

#pragma pack(pop)
