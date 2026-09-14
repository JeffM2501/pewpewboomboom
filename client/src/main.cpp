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
#include "rlgl.h"

ClientWorld World;

Camera2D ViewCamera = { 0 };
Texture GridTexture = { 0 };
Texture GroundTexture = { 0 };

ClientLocalPlayerState* LocalPlayer = nullptr;

InputState CurrentInputState = { 0 };

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
	GridTexture = LoadTexture("resources/texture_01.png");
	GroundTexture = LoadTexture("resources/pattern_15.png");

	TankManager::Init();

	Network.GetEvents().OnJoin.Add([](const ClientPlayerState* playerInfo, void*)
		{
            LocalPlayer = static_cast<ClientLocalPlayerState*>(Network.GetPlayerList().GetPlayer(playerInfo->PlayerID));
			LocalPlayer->World = &World;
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
	UnloadTexture(GridTexture);
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
	if (CurrentZoom < 0.25f)
		CurrentZoom = 0.25f;
	if (CurrentZoom > 128.0f)
		CurrentZoom = 128.0f;

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

	float groundTextureScale = 32.0f;

	Rectangle destRect = { min.x, min.y, (max.x - min.x), (max.y - min.y) };
	Rectangle sourceRect = destRect * (16.0f);

	DrawTexturePro(GroundTexture, sourceRect, destRect, Vector2Zeros, 0, ColorAlpha(DARKGRAY, 0.5f));

	if (World.IsValid())
		World.Draw();

    Network.GetPlayerList().DoForEachPlayer([](ClientPlayerState* player)
        {
            TankManager::DrawTank(player->IsLocalPlayer ? TeamColors::Green : TeamColors::Red, player->Transform, player->IsLocalPlayer);
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
}

void UpdateLocalPlayerState()
{
    if (!LocalPlayer)
        return;

    auto newPos = UpdatePlayerTransform(LocalPlayer->Transform, CurrentInputState, 1.0f/kDefaultTickRate, LocalPlayer->Rules);

	BoundingCircle pos = { LocalPlayer->Transform.Position, 10 };

	newPos = World.Collide(LocalPlayer->Transform.Position, newPos, 1, pos);
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

	UpdateLocalPlayerState();
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