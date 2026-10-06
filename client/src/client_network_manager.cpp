#include "external/fix_win32_compatibility.h"
#include "client_network_manager.h"
#include "enet.h"
#include <cstdio>
#include "protocol.h"
#include "time_utils.h"
#include "packet_processor.h"
#include "text_utils.h"
#include "game.h"
#include "data_utils.h"
#include "raylib.h"
#include "guiControls/chat_window.h"

ClientNetworkManager Network;

ClientNetworkManager::ClientNetworkManager()
	: PingAccumualtor(0.5f, 2)
{
	enet_initialize();

	RegisterProcessor<S2C_Pong>(PacketType::S2C_Pong, ProcessS2C_Pong);
	RegisterProcessor<S2C_JoinResponse>(PacketType::S2C_JoinResponse, ProcessS2C_JoinResponse);
	RegisterProcessor<S2C_PlayerJoined>(PacketType::S2C_PlayerJoined, ProcessS2C_PlayerJoined);
	RegisterProcessor<S2C_PlayerDisconnected>(PacketType::S2C_PlayerDisconnected, ProcessS2C_PlayerDisconnected);
	RegisterProcessor<C2S_ChatMessage>(PacketType::C2S_ChatMessage, ProcessC2S_ChatMessage);
	RegisterProcessor<S2C_SetWorldInfo>(PacketType::S2C_SetWorldInfo, ProcessS2C_SetWorldInfo);
	RegisterProcessor<S2C_SetWorldObject>(PacketType::S2C_SetWorldObject, ProcessS2C_SetWorldObject);
    RegisterProcessor<S2C_BeginStateSnapshot>(PacketType::S2C_BeginStateSnapshot, ProcessS2C_BeginStateSnapshot);
    RegisterProcessor<S2C_PlayerSnapshot>(PacketType::S2C_PlayerSnapshot, ProcessS2C_PlayerSnapshot);
    RegisterProcessor<S2C_BulletSnapshot>(PacketType::S2C_BulletSnapshot, ProcessS2C_BulletSnapshot);
    RegisterProcessor<S2C_BulletDestroyed>(PacketType::S2C_BulletDestroyed, ProcessS2C_BulletDestroyed);
    RegisterProcessor<S2C_HitscanEffect>(PacketType::S2C_HitscanEffect, ProcessS2C_HitscanEffect);
    RegisterProcessor<S2C_PlayerSpawned>(PacketType::S2C_PlayerSpawned, ProcessS2C_PlayerSpawned);
    RegisterProcessor<S2C_PlayerDespawned>(PacketType::S2C_PlayerDespawned, ProcessS2C_PlayerDespawned);
    RegisterProcessor<S2C_ShotCreated>(PacketType::S2C_ShotCreated, ProcessS2C_ShotCreated);
    RegisterProcessor<S2C_PlayerScoreUpdate>(PacketType::S2C_PlayerScoreUpdate, ProcessS2C_PlayerScoreUpdate);
}

ClientNetworkManager::~ClientNetworkManager()
{
	Disconnect();
	enet_deinitialize();
}

FixedSizeString<kMaxPlayers>& ClientNetworkManager::GetPlayerName()
{
	return PlayerName;
}

void ClientNetworkManager::BeginConnect(const char* address, uint16_t port)
{
	WasTimeout = false;
	if (CurrentState != ConnectionState::Disconnected)
	{
		Disconnect();
	}

	ConnectionStartTime = GetTimeMs();

	ClientHost = enet_host_create(nullptr, 1, 2, 0, 0);
	ENetAddress hostAddress = { 0 };
	enet_address_set_host(&hostAddress, address);
	hostAddress.port = port;
	ServerPeer = enet_host_connect(ClientHost, &hostAddress, 2, 0);
    DefaultPeer = ServerPeer;
	CurrentState = ConnectionState::Connecting;
}

void ClientNetworkManager::Disconnect()
{
	PlayerID = uint64_t(-1);
	ServerTickBase = 0;
	ServerTickSyncTimeMs = 0;
	ClientLastProcessedServerTick = 0;

	if (ServerPeer)
	{
		C2S_Goodbye bye;
		SendPacket(ServerPeer, 0, bye);
		enet_host_flush(ClientHost);

		enet_peer_disconnect_now(ServerPeer, 0);
		ServerPeer = nullptr;
	}

	if (ClientHost)
	{
		enet_host_destroy(ClientHost);
		ClientHost = nullptr;
	}

	CurrentState = ConnectionState::Disconnected;
}

ConnectionState ClientNetworkManager::GetState() const
{
	return CurrentState;
}

bool ClientNetworkManager::IsReady() const
{
	return CurrentState == ConnectionState::Connected && PlayerID != uint64_t(-1);
}

bool ClientNetworkManager::HadTimeout() const
{
	return WasTimeout;
}

uint64_t ClientNetworkManager::GetRTT() const
{
	return LastRTT;
}

Vector2 ClientNetworkManager::GetSpawn() const
{
	return Spawn;
}

uint64_t ClientNetworkManager::GetCurrentServerTick() const
{
	if (ServerTickSyncTimeMs == 0)
	{
		return 0;
	}

	uint64_t nowMs = GetTimeMs();
	if (nowMs < ServerTickSyncTimeMs)
	{
		return ServerTickBase;
	}

	double elapsedSec = (nowMs - ServerTickSyncTimeMs) / 1000.0;
	uint64_t ticksElapsed = static_cast<uint64_t>(elapsedSec * kDefaultTickRate);
	return ServerTickBase + ticksElapsed;
}

void ClientNetworkManager::Update()
{
	if (CurrentState != ConnectionState::Disconnected)
	{
		ENetEvent event;
		int count = 0;
		const int maxMessages = 10;
		while (enet_host_service(ClientHost, &event, 0) > 0)
		{
			if (event.type == ENET_EVENT_TYPE_CONNECT)
			{
				CurrentState = ConnectionState::Connected;

				GetLogger().Log(LogLevel::Info, "[Client] Connected to server.");
				SendPing();

				bool valid = true;
				ConnectionEvents.OnConnect.Invoke(valid);

				SendJoin();
			}
			else if (event.type == ENET_EVENT_TYPE_RECEIVE)
			{
				ProcessPacket(event.packet, event.peer);
				enet_packet_destroy(event.packet);
			}
			else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
			{
				CurrentState = ConnectionState::Disconnected;
                GetEvents().OnDisconnect.Invoke(false);
			}
			else if (event.type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
			{
				CurrentState = ConnectionState::Disconnected;
				WasTimeout = true;
				GetEvents().OnDisconnect.Invoke(true);
			}

			count++;
			if (count >= maxMessages)
				break;
		}
		enet_host_flush(ClientHost);

		if (CurrentState == ConnectionState::Connecting)
		{
			// see if we have waited too long
			if (GetTimeMs() - ConnectionStartTime > ConnectionTimeout)
			{
				WasTimeout = true;
				Disconnect();
			}
		}

		if (CurrentState == ConnectionState::Connected && ServerTickSyncTimeMs > 0)
		{
			uint64_t currentTick = GetCurrentServerTick();
			if (ClientLastProcessedServerTick == 0)
			{
				ClientLastProcessedServerTick = currentTick;
			}

			while (ClientLastProcessedServerTick < currentTick)
			{
				ClientLastProcessedServerTick++;
				ConnectionEvents.OnTick.Invoke(ClientLastProcessedServerTick);
			}

			enet_host_flush(ClientHost);
		}
	}

	PingAccumualtor.ProcessTicks([this](double)
		{
			SendPing();
		});
}

ClientNetworkManager::Events& ClientNetworkManager::GetEvents()
{
	return ConnectionEvents;
}

PlayerList& ClientNetworkManager::GetPlayerList()
{
	return Players;
}

void ClientNetworkManager::SendPing()
{
	if (CurrentState == ConnectionState::Disconnected)
		return;

    // Send C2S_Ping on channel 0 reliably upon connection
    C2S_Ping ping;
    ping.type = static_cast<uint8_t>(PacketType::C2S_Ping);
    ping.clientTimeMs = GetTimeMs();

    SendPacket(ServerPeer, 0, ping);

	GetLogger().Log(LogLevel::Verbose, "[Client] Sent C2S_Ping packet on Channel 0 (timestamp: %llu ms).", ping.clientTimeMs);
}

void ClientNetworkManager::SendJoin()
{
    C2S_JoinRequest join;
    CopyFixedSizeString(join.desriredName, PlayerName, sizeof(PlayerName));
    SendPacket(ServerPeer, 0, join);

    GetLogger().Log(LogLevel::Info, "[Client] Sent C2S_JoinRequest packet on Channel 0, desired name %s.", PlayerName.Data());
}

void ClientNetworkManager::SentChatMessage(std::string_view message) 
{
    C2S_ChatMessage chat;
    CopyFixedSizeString(chat.message, message.data(), kMaxChatLineSize);
    chat.senderId = PlayerID;
    SendPacket(ServerPeer, 0, chat);
}

void ClientNetworkManager::ProcessS2C_Pong(PacketProcessor& processor, ENetPeer* sender, const S2C_Pong* pong)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
	uint64_t nowMs = GetTimeMs();
	uint64_t rtt = (nowMs >= pong->clientTimeMs) ? (nowMs - pong->clientTimeMs) : 0;
	if (self.LastRTT == 0)
	{
		self.LastRTT = rtt;
	}
	else
	{
		self.LastRTT = static_cast<uint64_t>(self.LastRTT * 0.8 + rtt * 0.2);
	}

	uint64_t latency = rtt / 2;
	uint64_t syncTime = pong->clientTimeMs + latency;

	if (self.ServerTickSyncTimeMs == 0)
	{
		self.ServerTickBase = pong->serverTick;
		self.ServerTickSyncTimeMs = syncTime;
	}
	else
	{
		uint64_t currentEst = self.GetCurrentServerTick();
		uint64_t incomingEst = pong->serverTick;
		if (nowMs > syncTime)
		{
			incomingEst += static_cast<uint64_t>((nowMs - syncTime) * kDefaultTickRate / 1000.0);
		}

		int64_t drift = static_cast<int64_t>(incomingEst) - static_cast<int64_t>(currentEst);
		if (std::abs(drift) > 3)
		{
			self.ServerTickBase = pong->serverTick;
			self.ServerTickSyncTimeMs = syncTime;
		}
	}

	GetLogger().Log(LogLevel::Info, "[Client] Received S2C_Pong packet! RTT Latency: %llu ms (Server Uptime: %llu ms, Server Tick: %llu)", rtt, pong->serverTimeMs, pong->serverTick);
}

void ClientNetworkManager::ProcessS2C_JoinResponse(PacketProcessor& processor, ENetPeer* sender, const S2C_JoinResponse* responce)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
	self.PlayerName = responce->actualName;
	self.PlayerID = responce->playerId;
	self.Spawn = DataUtils::UnpackVector2(responce->spawn);

	auto localPlayerInfo = self.Players.AddPlayer(self.PlayerID, true);
	localPlayerInfo->Name = self.PlayerName;
    localPlayerInfo->Team = responce->team;
	localPlayerInfo->IsLocalPlayer = true;
    localPlayerInfo->Transform.Position = self.Spawn;
    localPlayerInfo->Transform.Rotation[0] = 0.0f;
    localPlayerInfo->Transform.Rotation[1] = 0.0f;

	localPlayerInfo->CollisionRadius = responce->collisionRadius;

	localPlayerInfo->Rules.BoostMultiplier = responce->bostMultiplier;
	localPlayerInfo->Rules.MaxSpeed = responce->maxSpeed;
	localPlayerInfo->Rules.TurnSpeed = responce->turnSpeed;

	self.ConnectionEvents.OnJoin.Invoke(localPlayerInfo);
	self.ConnectionEvents.OnSpawn.Invoke(self.Spawn);
}

void ClientNetworkManager::ProcessS2C_PlayerJoined(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerJoined* joinInfo)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
	auto localPlayerInfo = self.Players.AddPlayer(joinInfo->playerId);
	localPlayerInfo->Name = joinInfo->name;
	localPlayerInfo->CollisionRadius = joinInfo->collisionRadius;
	localPlayerInfo->Team = joinInfo->team;
	self.ConnectionEvents.OnPlayerJoin.Invoke(joinInfo->playerId);
}

void ClientNetworkManager::ProcessS2C_PlayerDisconnected(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerDisconnected* disconnectInfo)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
	self.Players.RemovePlayer(disconnectInfo->playerId);
	self.ConnectionEvents.OnPlayerDisconnect.Invoke(disconnectInfo->playerId);
}

void ClientNetworkManager::ProcessC2S_ChatMessage(PacketProcessor& processor, ENetPeer* sender, const C2S_ChatMessage* chatMessage)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);

	std::pair<uint64_t, std::string> eventInfo(chatMessage->senderId, chatMessage->message);
	self.GetEvents().OnChatMessage.Invoke(eventInfo, &self);
}

void ClientNetworkManager::ProcessS2C_SetWorldObject(PacketProcessor& processor, ENetPeer* sender, const S2C_SetWorldObject* objectInfo)
{
    World.Receive(*objectInfo);
}

void ClientNetworkManager::ProcessS2C_SetWorldInfo(PacketProcessor& processor, ENetPeer* sender, const S2C_SetWorldInfo* worldInfo)
{
	World.Init(*worldInfo);
}

void ClientNetworkManager::ProcessS2C_BeginStateSnapshot(PacketProcessor& processor, ENetPeer* sender, const S2C_BeginStateSnapshot* snapshot)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);

    if (snapshot->snapshotTick > self.LastReceivedServerTick)
    {
        self.LastReceivedServerTick = snapshot->snapshotTick;
    }
}

void ClientNetworkManager::ProcessS2C_PlayerSnapshot(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerSnapshot* snapshot)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);

    auto* player = self.GetPlayerList().GetPlayer(snapshot->playerId);
    if (player)
    {
        PlayerTransform newTransform;
        newTransform.Position = DataUtils::UnpackVector2(snapshot->position);
        newTransform.Rotation[0] = snapshot->rotation[0];
        newTransform.Rotation[1] = snapshot->rotation[1];
        newTransform.Velocity = DataUtils::UnpackVector2(snapshot->velocity);

        bool wasDead = player->IsDead;
        player->Health = snapshot->health;
        player->IsDead = (snapshot->isDead != 0);

        if (snapshot->serverTick > self.LastReceivedServerTick)
        {
            self.LastReceivedServerTick = snapshot->serverTick;
        }

        player->AddServerStateUpdate(snapshot->serverTick, newTransform);

        if (wasDead && !player->IsDead)
        {
            S2C_PlayerSpawned spawn;
            spawn.serverTick = snapshot->serverTick;
            spawn.playerId = snapshot->playerId;
            spawn.position[0] = snapshot->position[0];
            spawn.position[1] = snapshot->position[1];
            spawn.rotation = snapshot->rotation[0];
            self.ConnectionEvents.OnPlayerSpawned.Invoke(spawn);
        }
        else if (!wasDead && player->IsDead)
        {
            S2C_PlayerDespawned despawn;
            despawn.serverTick = snapshot->serverTick;
            despawn.playerId = snapshot->playerId;
            despawn.position[0] = snapshot->position[0];
            despawn.position[1] = snapshot->position[1];
            despawn.reason = S2C_PlayerDespawned::Reason::Killed;
            despawn.killerId = 0;
            self.ConnectionEvents.OnPlayerDespawned.Invoke(despawn);
        }
    }
}

void ClientNetworkManager::ProcessS2C_BulletSnapshot(PacketProcessor& processor, ENetPeer* sender, const S2C_BulletSnapshot* snapshot)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
    const auto& bState = snapshot->state;

    if (self.RecentlyDestroyedBullets.find(bState.bulletId) != self.RecentlyDestroyedBullets.end())
    {
        return;
    }

    Vector2 serverPos = DataUtils::UnpackVector2(bState.position);
    Vector2 serverVel = DataUtils::UnpackVector2(bState.velocity);

    uint64_t currentTick = self.GetCurrentServerTick();
    if (currentTick == 0)
    {
        currentTick = snapshot->serverTick;
    }

    float ticksElapsed = (currentTick > snapshot->serverTick) ? float(currentTick - snapshot->serverTick) : 0.0f;
    if (ticksElapsed > 10.0f)
    {
        ticksElapsed = 10.0f;
    }
    float timeElapsed = ticksElapsed / float(kDefaultTickRate);

    Vector2 predictedPos = Vector2Add(serverPos, Vector2Scale(serverVel, timeElapsed));

    auto it = self.Bullets.find(bState.bulletId);
    if (it == self.Bullets.end())
    {
        auto& b = self.Bullets[bState.bulletId];
        b.ID = bState.bulletId;
        b.OwnerID = bState.ownerId;
        b.BulletType = bState.bulletType;
        b.Position = predictedPos;
        b.Velocity = serverVel;
        b.LastUpdatedTime = static_cast<float>(GetTime());

        S2C_ShotCreated shot;
        shot.serverTick = snapshot->serverTick;
        shot.bulletId = bState.bulletId;
        shot.ownerId = bState.ownerId;
        shot.bulletType = bState.bulletType;
        shot.position[0] = bState.position[0];
        shot.position[1] = bState.position[1];
        shot.velocity[0] = bState.velocity[0];
        shot.velocity[1] = bState.velocity[1];
        self.ConnectionEvents.OnShotCreated.Invoke(shot);
    }
    else
    {
        auto& b = it->second;
        b.Velocity = serverVel;
        b.LastUpdatedTime = static_cast<float>(GetTime());

        Vector2 diff = Vector2Subtract(predictedPos, b.Position);
        float err = Vector2Length(diff);

        if (err > 4.0f)
        {
            b.Position = predictedPos;
        }
        else if (err > 0.05f)
        {
            b.Position = Vector2Add(b.Position, Vector2Scale(diff, 0.2f));
        }
    }
}

void ClientNetworkManager::ProcessS2C_BulletDestroyed(PacketProcessor& processor, ENetPeer* sender, const S2C_BulletDestroyed* packet)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
    self.Bullets.erase(packet->bulletId);
    self.RecentlyDestroyedBullets[packet->bulletId] = static_cast<float>(GetTime());
    self.ConnectionEvents.OnBulletDestroyed.Invoke(*packet);
}

void ClientNetworkManager::ProcessS2C_HitscanEffect(PacketProcessor& processor, ENetPeer* sender, const S2C_HitscanEffect* packet)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
    self.ConnectionEvents.OnHitscanEffect.Invoke(*packet);

    Vector2 start = DataUtils::UnpackVector2(packet->startPoint);
    Vector2 end = DataUtils::UnpackVector2(packet->endPoint);
    float dist = Vector2Distance(start, end);

    if (packet->hitBuildingId != 0)
    {
        MachineGunHitBuildingEvent buildingEvent;
        buildingEvent.ShooterID = packet->shooterId;
        buildingEvent.BuildingID = packet->hitBuildingId;
        buildingEvent.HitPoint = end;
        buildingEvent.Distance = dist;
        self.ConnectionEvents.OnMachineGunHitBuilding.Invoke(buildingEvent, &self);
    }

    if (packet->targetPlayerId != 0)
    {
        MachineGunHitTankEvent tankEvent;
        tankEvent.ShooterID = packet->shooterId;
        tankEvent.TargetPlayerID = packet->targetPlayerId;
        tankEvent.HitPoint = end;
        tankEvent.Damage = kMachineGunDamage;
        tankEvent.Distance = dist;
        self.ConnectionEvents.OnMachineGunHitTank.Invoke(tankEvent, &self);
    }
}

void ClientNetworkManager::UpdateBullets(float deltaTime)
{
    float dt = fminf(deltaTime, 0.1f);
    float now = static_cast<float>(GetTime());
    for (auto it = Bullets.begin(); it != Bullets.end(); )
    {
        if (now - it->second.LastUpdatedTime > 0.5f)
        {
            it = Bullets.erase(it);
        }
        else
        {
            it->second.Position.x += it->second.Velocity.x * dt;
            it->second.Position.y += it->second.Velocity.y * dt;
            ++it;
        }
    }

    for (auto it = RecentlyDestroyedBullets.begin(); it != RecentlyDestroyedBullets.end(); )
    {
        if (now - it->second > 2.0f)
        {
            it = RecentlyDestroyedBullets.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ClientNetworkManager::ProcessS2C_PlayerSpawned(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerSpawned* packet)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
    auto* player = self.GetPlayerList().GetPlayer(packet->playerId);
    if (player)
    {
        player->IsDead = false;
        player->Transform.Position = DataUtils::UnpackVector2(packet->position);
        player->Transform.Rotation[0] = packet->rotation;

        if (player->IsLocalPlayer)
        {
            self.Spawn = player->Transform.Position;
            self.ConnectionEvents.OnSpawn.Invoke(self.Spawn);
        }
    }

    self.ConnectionEvents.OnPlayerSpawned.Invoke(*packet);
}

void ClientNetworkManager::ProcessS2C_PlayerDespawned(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerDespawned* packet)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
    auto* player = self.GetPlayerList().GetPlayer(packet->playerId);
    if (player)
    {
        player->IsDead = true;
    }

    self.ConnectionEvents.OnPlayerDespawned.Invoke(*packet);
}

void ClientNetworkManager::ProcessS2C_ShotCreated(PacketProcessor& processor, ENetPeer* sender, const S2C_ShotCreated* packet)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
    if (self.RecentlyDestroyedBullets.find(packet->bulletId) != self.RecentlyDestroyedBullets.end())
    {
        return;
    }

    auto it = self.Bullets.find(packet->bulletId);
    if (it == self.Bullets.end())
    {
        auto& b = self.Bullets[packet->bulletId];
        b.ID = packet->bulletId;
        b.OwnerID = packet->ownerId;
        b.BulletType = packet->bulletType;
        b.Position = DataUtils::UnpackVector2(packet->position);
        b.Velocity = DataUtils::UnpackVector2(packet->velocity);
        b.LastUpdatedTime = static_cast<float>(GetTime());
    }

    self.ConnectionEvents.OnShotCreated.Invoke(*packet);
}

void ClientNetworkManager::ProcessS2C_PlayerScoreUpdate(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerScoreUpdate* packet)
{
    ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
    auto* player = self.GetPlayerList().GetPlayer(packet->playerId);
    if (player)
    {
        player->Kills = packet->kills;
        player->Deaths = packet->deaths;
    }
}


