#pragma once

#include <cstdint>
#include <type_traits>

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
	S2C_SetWorldInfo = 7,
	S2C_SetWorldObject = 8,
	S2C_EventNotification = 9,
	C2S_ChatMessage = 10,
	S2C_PlayerJoined = 11,
	S2C_PlayerDisconnected = 12,
    S2C_BeginStateSnapshot = 13,
	S2C_PlayerSnapshot = 14,
	S2C_ChatMessage = 15
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

	float collisionRadius = 2.0f;
    float maxSpeed = 20;
    float turnSpeed = 90.0f; // degrees per second
    float bostMultiplier = 2.0f;

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

enum class Movement : uint8_t
{
    None = 0,
    Up = 1 << 0,
    Down = 1 << 1,
    Left = 1 << 2,
    Right = 1 << 3
};

template <typename T>
struct enable_bitmask_operators : std::false_type {};

template <typename T>
typename std::enable_if_t<enable_bitmask_operators<T>::value, T>
operator|(T lhs, T rhs) {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template <typename T>
typename std::enable_if_t<enable_bitmask_operators<T>::value, T>
operator&(T lhs, T rhs) {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template <typename T>
typename std::enable_if_t<enable_bitmask_operators<T>::value, T>
operator^(T lhs, T rhs) {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template <typename T>
typename std::enable_if_t<enable_bitmask_operators<T>::value, T>
operator~(T val) {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(~static_cast<underlying>(val));
}

// Compound assignment operators
template <typename T>
typename std::enable_if_t<enable_bitmask_operators<T>::value, T&>
operator|=(T& lhs, T rhs) {
    lhs = lhs | rhs;
    return lhs;
}

template <typename T>
typename std::enable_if_t<enable_bitmask_operators<T>::value, T&>
operator&=(T& lhs, T rhs) {
    lhs = lhs & rhs;
    return lhs;
}

template <>
struct enable_bitmask_operators<Movement> : std::true_type {};

enum class Action : uint8_t
{
    None = 0,
    Shoot = 1 << 0,
    Boost = 1 << 1,
    DropPowerup = 1 << 2,
};

template <>
struct enable_bitmask_operators<Action> : std::true_type {};

struct C2S_InputState
{
	uint8_t type = static_cast<uint8_t>(PacketType::C2S_InputState);
	uint64_t clientTick = 0;

	float forward = 0.0f;
	float turn = 0.0f;
	float aimDirection = 0.0f;

	bool shoot = false;
	bool boost = false;
};

struct S2C_SetWorldInfo
{
	uint8_t type = static_cast<uint8_t>(PacketType::S2C_SetWorldInfo);
	uint64_t objectCount = 0;
	char name[kMaxChatLineSize];
};

struct S2C_SetWorldObject
{
	uint8_t type = static_cast<uint8_t>(PacketType::S2C_SetWorldObject);

	uint64_t id = 0;

	enum class ObjectType : uint8_t
	{
		Walls,
		Building,
		Box,
		Barrel,
	};
	ObjectType objType = ObjectType::Box;
	float position[2] = { 0.0f, 0.0f };
	float rotation = 0.0f;
	float scale = 1.0f;
	uint8_t color[4] = { 255, 255, 255, 255 };
};

struct S2C_BeginStateSnapshot
{
    uint8_t type = static_cast<uint8_t>(PacketType::S2C_BeginStateSnapshot);
    uint64_t snapshotTick = 0;
    uint8_t playerCount = 0;
};

struct S2C_PlayerSnapshot
{
    uint8_t type = static_cast<uint8_t>(PacketType::S2C_PlayerSnapshot);
	uint64_t serverTick = 0;
    uint64_t playerId = 0;
    float position[2] = { 0.0f, 0.0f };
    float rotation[2] = { 0.0f, 0.0f };
    float velocity[2] = { 0.0f, 0.0f };
};
#pragma pack(pop)
