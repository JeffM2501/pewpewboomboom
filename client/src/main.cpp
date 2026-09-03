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

	BeginMode2D(ViewCamera);
	Vector2 min = GetScreenToWorld2D(Vector2Zeros, ViewCamera);
	Vector2 max = GetScreenToWorld2D(Vector2{ (float)GetRenderWidth(), (float)GetRenderHeight() }, ViewCamera);

	float groundTextureScale = 32.0f;

	Rectangle destRect = { min.x, min.y, (max.x - min.x), (max.y - min.y) };
	Rectangle sourceRect = destRect * (16.0f);

	DrawTexturePro(GroundTexture, sourceRect, destRect, Vector2Zeros, 0, ColorAlpha(DARKGRAY, 0.5f));

	if (World.IsValid())
		World.Draw();

	DrawCircleV(ViewCamera.target, 2, GREEN);

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

void ProcessNetTick(const uint64_t& tick, void*)
{
	if (!Network.IsReady())
		return;

	// poll input
}

void ProcessPlayerSpawn(Vector2 spawnPos)
{
	ViewCamera.target = spawnPos;
}

int main()
{
	GameInit();

	if (true)
	{
		RunGameLoop([]()
			{
				if (!GameUpdate() || WindowShouldClose())
					return false;

				GameDraw();
				return true;
			});
	}
	else
	{
		while (!WindowShouldClose())
		{
			BeginDrawing();
			if (!GameUpdate())
				break;
			GameDraw();
			EndDrawing();
		}
	}

	GameCleanup();

	return 0;
}