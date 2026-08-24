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


Camera2D ViewCamera = { 0 };
Texture GridTexture = { 0 };

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
	io.Fonts->AddFontFromFileTTF("resources/fonts/Aileron-SemiBold.otf",defaultConfig.SizePixels, &defaultConfig);
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

	Network.GetEvents().OnSpawn.Add([](const Vector2& spawn, void*) { ProcessPlayerSpawn(spawn); });
	Network.GetEvents().OnChatMessage.Add([](const std::pair<uint64_t, std::string>& message, void*) 
		{
			ChatWindow::AddChatLine(Network.GetPlayerList().GetPlayer(message.first), message.second);
		});

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

	CloseWindow();
}

bool GameUpdate()
{
	ViewCamera.offset = Vector2{ (float)GetRenderWidth(), (float)GetRenderHeight() } / 2;

	Network.Update();
	return true;
}

void GameDraw()
{
	BeginDrawing();
	ClearBackground(GRAY);

	BeginMode2D(ViewCamera);
	Vector2 min = GetScreenToWorld2D(Vector2Zeros, ViewCamera);
	Vector2 max = GetScreenToWorld2D(Vector2{ (float)GetRenderWidth(), (float)GetRenderHeight() }, ViewCamera);

	Rectangle worldRect = { min.x, min.y, max.x - min.x, max.y - min.y };
	DrawTexturePro(GridTexture, worldRect, worldRect, Vector2Zeros, 0, ColorAlpha(WHITE, 0.5f));

	DrawCircleV(ViewCamera.target, 50, GREEN);
	DrawText(TextFormat("x %f y %f", ViewCamera.target.x, ViewCamera.target.y), int(ViewCamera.target.x), int(ViewCamera.target.y), 20, YELLOW);

	EndMode2D();

	rlImGuiBegin();
	NetConnectionDialog::ShowDialog();
	GameGui::Show();
	rlImGuiEnd();

	EndDrawing();
}

void ProcessNetTick(const uint64_t &tick, void*)
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

	while (!WindowShouldClose())
	{
		if (!GameUpdate())
			break;

		GameDraw();
	}
	GameCleanup();

	return 0;
}