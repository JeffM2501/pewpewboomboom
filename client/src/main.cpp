/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.
-- Game Premake by Jeffery Myers is marked CC0 1.0. To view a copy of this mark, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include <deque>

#include "raylib.h"
#include "raymath.h"

#include "game.h"  
#include "lib.h"    

#include "rlimgui.h" 
#include "imgui.h"
#include "client_network_manager.h" 
#include "connection_dialog.h" 

#include "game_gui.h"
#include "guiControls/chat_window.h"
#include "guiControls/mini_map.h"
#include "tank_manager.h"
#include "world_data.h"
#include "player_state.h"
#include "collisions.h"
#include "rlgl.h"

ClientWorld World;

Camera2D ViewCamera = { 0 };
Texture DetailTexture = { 0 };
Texture GroundTexture = { 0 };
Texture CrosshairTexture = { 0 };

ClientLocalPlayerState* LocalPlayer = nullptr;

InputState CurrentInputState = { 0 };

std::unordered_map<StaticTextures, Texture> TextureCache;

Texture GetTexture(StaticTextures texture)
{
	return TextureCache[texture];
}

void LoadStaticTextures()
{
	TextureCache[StaticTextures::Barrel] = LoadTexture("resources/barrel.png");
	GenTextureMipmaps(&TextureCache[StaticTextures::Barrel]);
	SetTextureFilter(TextureCache[StaticTextures::Barrel], TEXTURE_FILTER_TRILINEAR);

    TextureCache[StaticTextures::Box] = LoadTexture("resources/box.png");
    GenTextureMipmaps(&TextureCache[StaticTextures::Box]);
    SetTextureFilter(TextureCache[StaticTextures::Box], TEXTURE_FILTER_TRILINEAR);

    TextureCache[StaticTextures::Building] = LoadTexture("resources/building.png");
    GenTextureMipmaps(&TextureCache[StaticTextures::Building]);
    SetTextureFilter(TextureCache[StaticTextures::Building], TEXTURE_FILTER_TRILINEAR);

    TextureCache[StaticTextures::RoofDetail] = LoadTexture("resources/rooftop_details.png");
    GenTextureMipmaps(&TextureCache[StaticTextures::RoofDetail]);
    SetTextureFilter(TextureCache[StaticTextures::RoofDetail], TEXTURE_FILTER_TRILINEAR);
}

float CurrentZoom = 16.0f;

void ResetCurrentInput()
{
    CurrentInputState.Boost = false;
    CurrentInputState.Shoot = false;
    CurrentInputState.Foward = 0;
    CurrentInputState.Turn = 0;
}

void PollInputActions()
{
	if (IsKeyDown(KEY_W))
		CurrentInputState.Foward += 1;

    if (IsKeyDown(KEY_S))
		CurrentInputState.Foward -= 1;

	if (IsKeyDown(KEY_A))
		CurrentInputState.Turn -= 1;
	if (IsKeyDown(KEY_D))
		CurrentInputState.Turn += 1;

	if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		CurrentInputState.Shoot = true;
	}	

	if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
	{
		CurrentInputState.Boost = true;
	}

    Vector2 mousePos = GetMousePosition() - (Vector2{ (float)GetScreenWidth(), (float)GetScreenHeight() } / 2);

	CurrentInputState.TurretAngle = atan2f(mousePos.y, mousePos.x) * RAD2DEG;
}

static void GameLogger(std::string_view message, LogLevel level)
{
	ChatWindow::AddLogLine(message);
}

Logger GlobalLogger(GameLogger);
Logger& GetLogger()
{
	return GlobalLogger;
}

void ProcessNetTick(const uint64_t& tick, void* sender);

void ProcessPlayerSpawn(Vector2 spawnPos);

void SetupRlImGuiFonts()
{
	ImGuiIO& io = ImGui::GetIO();

	ImFontConfig defaultConfig;

	static constexpr int DefaultFonSize = 12;

	defaultConfig.SizePixels = DefaultFonSize;
#if !defined(__APPLE__)
	if (!IsWindowState(FLAG_WINDOW_HIGHDPI))
		defaultConfig.SizePixels = ceilf(defaultConfig.SizePixels * GetWindowScaleDPI().y);

	defaultConfig.ExtraSizeScale = GetWindowScaleDPI().y;
#endif

	defaultConfig.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF("resources/fonts/Aileron-SemiBold.otf", defaultConfig.SizePixels, &defaultConfig);
}

void GameInit()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "Pew Pew Boom Boom");
	SetTargetFPS(144);

	rlImGuiSetLoadFontsCallback(SetupRlImGuiFonts);
	rlImGuiSetup(true);
	GameGui::InstallStyle();

	Network.GetEvents().OnTick.Add(ProcessNetTick);

	// load resources
	DetailTexture = LoadTexture("resources/grass_detal2.png");
    GenTextureMipmaps(&DetailTexture);
    SetTextureFilter(DetailTexture, TEXTURE_FILTER_TRILINEAR);

	GroundTexture = LoadTexture("resources/grass.png");
    GenTextureMipmaps(&GroundTexture);
    SetTextureFilter(GroundTexture, TEXTURE_FILTER_TRILINEAR);

	CrosshairTexture = LoadTexture("resources/circle-02-whole.png");
	GenTextureMipmaps(&CrosshairTexture);
	SetTextureFilter(CrosshairTexture, TEXTURE_FILTER_TRILINEAR);

	LoadStaticTextures();

	TankManager::Init();

	Network.GetEvents().OnJoin.Add([](const ClientPlayerState* playerInfo, void*)
		{
            LocalPlayer = static_cast<ClientLocalPlayerState*>(Network.GetPlayerList().GetPlayer(playerInfo->PlayerID));
			LocalPlayer->World = &World;
			LocalPlayer->OwnerList = &Network.GetPlayerList();
			ResetCurrentInput();
		});

    Network.GetEvents().OnDisconnect.Add([](const bool& timeout, void*)
        {
            LocalPlayer = nullptr;
            ChatWindow::AddSystemChatLine(timeout ? "Disconnected from server due to timeout." : "Disconnected from server.");
        });

	Network.GetEvents().OnSpawn.Add([](const Vector2& spawn, void*) { ProcessPlayerSpawn(spawn); });
	Network.GetEvents().OnChatMessage.Add([](const std::pair<uint64_t, std::string>& message, void*)
		{
			ChatWindow::AddChatLine(Network.GetPlayerList().GetPlayer(message.first), message.second);
		});

    World.WorldStarted.Add([](const auto&, void*) {World.Loading = true; });
    World.WorldFinalized.Add([](const auto&, void*) {World.Loading = false; });

	ChatWindow::OnSendChatMessage.Add([](const std::string& message, void*)
		{
			Network.SentChatMessage(message);
		});
	ChatWindow::AddSystemChatLine("Client Startup");

	ViewCamera.zoom = 1;
}

void GameCleanup()
{
	rlImGuiShutdown();
	Network.Disconnect();

	// unload resources
	UnloadTexture(DetailTexture);
	UnloadTexture(GroundTexture);

	CloseWindow();
}

bool GameUpdate()
{
	PollInputActions();

	if (IsKeyPressed(KEY_PAGE_UP))
		MiniMap::ZoomIn();
    if (IsKeyPressed(KEY_PAGE_DOWN	))
        MiniMap::ZoomOut();
    if (IsKeyPressed(KEY_HOME))
        MiniMap::ResetZoom();


	ViewCamera.offset = Vector2{ (float)GetRenderWidth(), (float)GetRenderHeight() } / 2;

	CurrentZoom += GetMouseWheelMove() * 0.25f;
	if (CurrentZoom < 1.0f)
		CurrentZoom = 1.0f;
	if (CurrentZoom > 96.0f)
		CurrentZoom = 96.0f;

	ViewCamera.zoom = CurrentZoom;

    Network.GetPlayerList().DoForEachPlayer([](ClientPlayerState* player)
        {
            if (!player->IsLocalPlayer)
            {
                player->UpdateInterpolatedTransform(GetFrameTime());
            }
        });

	Network.Update();
	return true;
}

inline Rectangle operator * (const Rectangle& lhs, const float& rhs)
{
	return Rectangle{ lhs.x * rhs, lhs.y * rhs, lhs.width * rhs, lhs.height * rhs };
}

void GameDraw()
{
	ClearBackground(GRAY);
	if (LocalPlayer)
	{
		ViewCamera.target = LocalPlayer->Transform.Position;
	}

	BeginMode2D(ViewCamera);
	Vector2 min = GetScreenToWorld2D(Vector2Zeros, ViewCamera);
	Vector2 max = GetScreenToWorld2D(Vector2{ (float)GetRenderWidth(), (float)GetRenderHeight() }, ViewCamera);

	float groundTextureScale = 48.0f;

	Rectangle destRect = { min.x, min.y, (max.x - min.x), (max.y - min.y) };
	Rectangle sourceRect = destRect * (groundTextureScale);

	DrawTexturePro(GroundTexture, sourceRect, destRect, Vector2Zeros, 0, WHITE);

	Rectangle detailSourceRect = destRect * (groundTextureScale*0.5f);
	//DrawTexturePro(DetailTexture, detailSourceRect, destRect, Vector2Zeros, 0, ColorAlpha(WHITE,0.85f));

	if (World.IsValid())
		World.Draw();

    Network.GetPlayerList().DoForEachPlayer([](ClientPlayerState* player)
        {
            TankManager::DrawTank(player->IsLocalPlayer ? TeamColors::Blue : TeamColors::Red, player->Transform, player->IsLocalPlayer);
        });

	EndMode2D();

	rlImGuiBegin();
	NetConnectionDialog::ShowDialog();
	GameGui::Show();
	rlImGuiEnd();

	if (World.Loading)
	{
		DrawText(TextFormat("Downloading World %d/%d", World.Objects.size(), World.Count), 200, 200, 20, BLUE);
	}
	
	if (Network.IsReady())
	{
		Vector2 center = { GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };

		Vector2 vecToMouse = GetMousePosition() - center;

		float angle = atan2f(vecToMouse.y, vecToMouse.x) * RAD2DEG;

		float crosshairSize = 50.0f;

		Rectangle targetRect = { GetMousePosition().x, GetMousePosition().y, crosshairSize, crosshairSize };
		Rectangle sourceRecct = { 0.0f, 0.0f, static_cast<float>(CrosshairTexture.width), static_cast<float>(CrosshairTexture.height) };

		DrawTexturePro(CrosshairTexture, sourceRecct, targetRect, Vector2{ crosshairSize * 0.5f, crosshairSize * 0.5f }, angle + 90.0f, ColorAlpha(WHITE, 0.5f));
		//DrawRectanglePro(targetRect, Vector2{ 10,25 }, angle, GRAY);
	}
}

void UpdateLocalPlayerState(uint64_t tick = 0)
{
    if (!LocalPlayer)
    {
        return;
    }

    if (tick == 0 && Network.IsReady())
    {
        tick = Network.GetCurrentServerTick();
    }

    auto newPos = UpdatePlayerTransform(LocalPlayer->Transform, CurrentInputState, 1.0f / kDefaultTickRate, LocalPlayer->Rules);

	BoundingCircle pos = { newPos, 10 };

	newPos = World.Collide(LocalPlayer->Transform.Position, newPos, LocalPlayer->CollisionRadius, pos);

    Network.GetPlayerList().DoForEachPlayer([&](ClientPlayerState* other)
    {
        if (other->IsLocalPlayer)
        {
            return;
        }

        PlayerTransform otherTransform = other->GetTransformAtTick(tick);
        Vector2 hitPoint;
        Vector2 hitNormal;
        IntersectCircleCylinder(otherTransform.Position, other->CollisionRadius, newPos, LocalPlayer->Transform.Position, LocalPlayer->CollisionRadius, hitPoint, hitNormal);
    });

	newPos = World.Collide(LocalPlayer->Transform.Position, newPos, LocalPlayer->CollisionRadius, pos);
	LocalPlayer->Transform.Position = newPos;
}

void UpdateRemotePlayersForTick(const uint64_t& tick)
{
    Network.GetPlayerList().DoForEachPlayer([&](ClientPlayerState* player)
        {
            if (!player->IsLocalPlayer)
            {
                player->UpdateForTick(tick);
            }
        });
}

void ProcessNetTick(const uint64_t& tick, void*)
{
	if (!Network.IsReady())
		return;

	C2S_InputState inputPacket;

	inputPacket.shoot = CurrentInputState.Shoot;
    inputPacket.boost = CurrentInputState.Boost;

    inputPacket.forward = CurrentInputState.Foward;
    inputPacket.turn = CurrentInputState.Turn;

    inputPacket.aimDirection = CurrentInputState.TurretAngle;

    inputPacket.clientTick = tick;

    Network.SendPacket(nullptr, 1, inputPacket, false);

	// push the input to history for reconcile
	LocalPlayer->InputHistory[tick] = CurrentInputState;

	UpdateLocalPlayerState(tick);
	ResetCurrentInput();
	UpdateRemotePlayersForTick(tick);
}

void ProcessPlayerSpawn(Vector2 spawnPos)
{
	ViewCamera.target = spawnPos;
    if (LocalPlayer)
    {
        LocalPlayer->Transform.Position = spawnPos;
    }
}

int main()
{
	GameInit();

    RunGameLoop([]()
        {
            if (!GameUpdate() || WindowShouldClose())
                return false;

            GameDraw();
            return true;
        });

	GameCleanup();

	return 0;
}