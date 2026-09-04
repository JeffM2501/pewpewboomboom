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

#include "world_data.h"
#include "rlgl.h"

ClientWorld World;


Camera2D ViewCamera = { 0 };
Texture GridTexture = { 0 };
Texture GroundTexture = { 0 };

Texture PlayerTexture = { 0 };
Vector2 PlayerTextureOrigin = { 0, 0 };
Texture PlayerTurretTexture = { 0 };
Vector2 PlayerTurretTextureOrigin = { 0, 0 };

ClientPlayerState* LocalPlayer = nullptr;


float CurrentAngle = 0;
float CurrentForward = 0;
float CurrentTurn = 0;
bool CurrentShoot = false;
bool CurrentBoost = false;

void ResetCurrentInput()
{
    CurrentForward = 0;
    CurrentTurn = 0;
    CurrentShoot = false;
    CurrentBoost = false;
}

void PollInputActions()
{
	if (IsKeyDown(KEY_W))
		CurrentForward += 1;

    if (IsKeyDown(KEY_S))
		CurrentForward -= 1;

	if (IsKeyDown(KEY_A))
		CurrentTurn -= 1;
	if (IsKeyDown(KEY_D))
		CurrentTurn += 1;

	if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		CurrentShoot = true;
	}	

	if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
	{
		CurrentBoost = true;
	}

    Vector2 mousePos = GetMousePosition() - Vector2{ (float)GetRenderWidth(), (float)GetRenderHeight() } / 2;

	CurrentAngle = atan2f(mousePos.y, mousePos.x);
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
	InitWindow(1280, 800, "Example");
	SetTargetFPS(144);

	rlImGuiSetLoadFontsCallback(SetupRlImGuiFonts);
	rlImGuiSetup(true);
	GameGui::InstallStyle();

	Network.GetEvents().OnTick.Add(ProcessNetTick);

	// load resources
	GridTexture = LoadTexture("resources/texture_01.png");
	GroundTexture = LoadTexture("resources/pattern_15.png");


    Image hullImage = LoadImage("resources/tanks/hull05_blue2.png");
	ImageRotateCW(&hullImage);
    PlayerTexture = LoadTextureFromImage(hullImage);
	GenTextureMipmaps(&PlayerTexture);
    SetTextureFilter(PlayerTexture, TEXTURE_FILTER_TRILINEAR);
    UnloadImage(hullImage);

    PlayerTextureOrigin = { PlayerTexture.width / 2.0f, PlayerTexture.height / 2.0f };

    Image turretImage = LoadImage("resources/tanks/turret07_blue.png");
	ImageRotateCW(&turretImage);
    PlayerTurretTexture = LoadTextureFromImage(turretImage);
	GenTextureMipmaps(&PlayerTurretTexture);
	SetTextureFilter(PlayerTurretTexture, TEXTURE_FILTER_TRILINEAR);
    UnloadImage(turretImage);

    PlayerTurretTextureOrigin = { PlayerTurretTexture.width / 4.0f, PlayerTurretTexture.height / 2.0f };

	Network.GetEvents().OnJoin.Add([](const ClientPlayerState* playerInfo, void*)
		{
            LocalPlayer = Network.GetPlayerList().GetPlayer(playerInfo->PlayerID);
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

	ViewCamera.offset = Vector2{ (float)GetRenderWidth(), (float)GetRenderHeight() } / 2;
	ViewCamera.zoom = 32;
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
            if (player)
            {
				float playerScale = 1.0f / 128.0f;

                Color playerColor = player->IsLocalPlayer ? WHITE : RED;
                Rectangle playerRect = { player->Transform.Position.x, player->Transform.Position.y, PlayerTexture.width * playerScale, PlayerTexture.height * playerScale };

                Rectangle srcRect = { 0, 0, (float)PlayerTexture.width, (float)PlayerTexture.height };
				DrawTexturePro(PlayerTexture, srcRect, playerRect, PlayerTextureOrigin * playerScale, player->Transform.Rotation[0], playerColor);

				playerScale *= 0.75f;
                Rectangle turretRect = { player->Transform.Position.x, player->Transform.Position.y, PlayerTurretTexture.width * playerScale, PlayerTurretTexture.height * playerScale };

                srcRect = { 0, 0, (float)PlayerTurretTexture.width, (float)PlayerTurretTexture.height };
                DrawTexturePro(PlayerTurretTexture, srcRect, turretRect, PlayerTurretTextureOrigin * playerScale, player->Transform.Rotation[1], playerColor);
            }
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

    if (CurrentTurn != 0)
		CurrentTurn = CurrentTurn / fabsf(CurrentTurn);

    LocalPlayer->Transform.Rotation[0] += CurrentTurn * (90.0f/kDefaultTickRate);

    Vector2 forwardDir = Vector2Rotate(Vector2{ 1, 0 }, LocalPlayer->Transform.Rotation[0] * DEG2RAD);
	
	if (CurrentForward != 0)
		CurrentForward = CurrentForward / fabsf(CurrentForward);

	LocalPlayer->Transform.Velocity = forwardDir * CurrentForward;

    LocalPlayer->Transform.Position += LocalPlayer->Transform.Velocity * (10.0f / kDefaultTickRate);
}

void ProcessNetTick(const uint64_t& tick, void*)
{
	if (!Network.IsReady())
		return;

	C2S_InputState inputPacket;

    inputPacket.action = Action::None;
    if (CurrentShoot)
        inputPacket.action = Action(uint8_t(inputPacket.action) | uint8_t(Action::Shoot));

    if (CurrentBoost)
        inputPacket.action = Action(uint8_t(inputPacket.action) | uint8_t(Action::Boost));

    if (CurrentForward > 0)
        inputPacket.movement |= Movement::Up;
    else if (CurrentForward < 0)
        inputPacket.movement |= Movement::Down;
	if (CurrentTurn > 0)
        inputPacket.movement |= Movement::Right;
    else if (CurrentTurn < 0)
        inputPacket.movement |= Movement::Left;

    inputPacket.aimDirection = CurrentAngle;	

    inputPacket.clientTick = tick;

    Network.SendPacket(nullptr, 1, inputPacket, false);

	UpdateLocalPlayerState();
	ResetCurrentInput();
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