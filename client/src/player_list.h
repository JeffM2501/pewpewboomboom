#pragma once

#include "player_state.h"
#include "client_world.h"

#include <unordered_map>
#include <memory>
#include <functional>

class PlayerList;

static constexpr uint64_t RemotePlayerHistoryOffset = kRemotePlayerHistoryOffsetTicks;
class ClientPlayerState : public PlayerState
{
public:
    bool IsLocalPlayer = false;

    double RenderTick = 0.0;
    bool InitializedInterp = false;

    Vector2 PositionError = { 0.0f, 0.0f };
    float RotationError[2] = { 0.0f, 0.0f };
    PlayerTransform SmoothedTransform;

    const PlayerTransform& GetRenderTransform() const
    {
        return IsLocalPlayer ? Transform : SmoothedTransform;
    }

    void UpdateInterpolatedTransform(float deltaTime);

    void UpdateForTick(uint64_t currentTick);

    virtual void AddServerStateUpdate(uint64_t tick, PlayerTransform& transform);
};

class ClientLocalPlayerState : public ClientPlayerState
{
public:
    ClientLocalPlayerState()
    {
        IsLocalPlayer = true;
    }

    std::map<uint64_t, InputState> InputHistory;

    void AddServerStateUpdate(uint64_t tick, PlayerTransform& transform) override;

    ClientWorld* World = nullptr;
    PlayerList* OwnerList = nullptr;
};

class PlayerList
{
private:
    std::unordered_map<uint64_t, std::unique_ptr<ClientPlayerState>> Players;
    ClientLocalPlayerState* LocalPlayer = nullptr;
public:

    ClientPlayerState* AddPlayer(uint64_t playerId, bool local = false);
    ClientPlayerState* GetPlayer(uint64_t playerId);
    void RemovePlayer(uint64_t playerId);
    void UpdatePlayerInfo(uint64_t playerId);

    void DoForEachPlayer(std::function<void(ClientPlayerState*)> callback);

    size_t Size()
    {
        return Players.size();
    }

    ClientLocalPlayerState* GetLocalPlayer() const
    {
        return LocalPlayer;
    }
};