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
#include "net_connection.h" 
#include "connection_dialog.h" 

Matrix CubeTransform = MatrixIdentity();
Mesh CubeMesh = { 0 };
Material CubeMaterial = { 0 };

Camera3D ViewCamera = { 0 };


#include <deque>
std::deque<std::string> LogLines;

static void GameLogger(std::string_view message, LogLevel level)
{
	LogLines.push_back(std::string(message));

	while (LogLines.size() > 10)
		LogLines.pop_front();
}

Logger GlobalLogger(GameLogger);
Logger& GetLogger()
{
	return GlobalLogger;
}


void ProcessNetTick(const uint64_t& tick, void* sender);

void GameInit()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 800, "Example");
	SetTargetFPS(144);

	rlImGuiSetup(true);

	NetConnection::Init();

	NetConnection::GetEvents().OnTick.Add(ProcessNetTick);

	ViewCamera.fovy = 45.0f;
	ViewCamera.position = { 3.0f, 3.0f, 3.0f };
	ViewCamera.up = { 0.0f, 1.0f, 0.0f };
	ViewCamera.target = { 0.0f, 0.0f, 0.0f };

	// load resources

	CubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
	CubeMaterial = LoadMaterialDefault();
	CubeMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("resources/texture_01.png");

	GenTextureMipmaps(&CubeMaterial.maps[MATERIAL_MAP_DIFFUSE].texture);
	SetTextureFilter(CubeMaterial.maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_TRILINEAR);

	NetConnection::GetEvents().OnSpawn.Add([](const Vector2& spawn, void*) { CubeTransform = MatrixTranslate(spawn.x, 0, spawn.y); });
}

void GameCleanup()
{
	rlImGuiShutdown();
	NetConnection::Shutdown();
	// unload resources
	UnloadMesh(CubeMesh);
	UnloadTexture(CubeMaterial.maps[MATERIAL_MAP_DIFFUSE].texture);

	CloseWindow();
}

bool GameUpdate()
{
	NetConnection::Update();
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
		UpdateCamera(&ViewCamera, CAMERA_THIRD_PERSON);
	return true;
}

void GameDraw()
{
	BeginDrawing();
	ClearBackground(GRAY);

	BeginMode3D(ViewCamera);
	DrawGrid(100, 1);
	DrawMesh(CubeMesh, CubeMaterial, CubeTransform);
	EndMode3D();

	if (NetConnection::GetState() != ConnectionState::Connected)
	{
		DrawText("Disconnected", 10, 10, 20, RED);
	}
	else
	{
		char statusText[128];
		snprintf(statusText, sizeof(statusText), "Connected | Name %s", NetConnection::GetPlayerName().Data());
		DrawText(statusText, 10, 10, 20, DARKGRAY);
	}

	rlImGuiBegin();
	NetConnectionDialog::ShowDialog();
	rlImGuiEnd();

	// playerlist
	int x = GetScreenWidth() - 300;
	DrawRectangle(x, 0, 300, 300, ColorAlpha(BLACK, 0.5f));
	int y = 5;
	NetConnection::GetPlayerList().DoForEachPlayer([&y, &x](PlayerState* player)
		{
			DrawText(player->Name.Data(), x+5, y, 20, WHITE);
			y += 20;
		});

	// log
	y = GetScreenHeight() - 200;
	DrawRectangle(5, y - 5, 600, 210, ColorAlpha(BLACK, 0.25f));
	for (auto& line : LogLines)
	{
		DrawText(line.c_str(), 10, y, 10, WHITE);
		y += 10;
	}

	EndDrawing();
}

void ProcessNetTick(const uint64_t &tick, void*)
{
	if (!NetConnection::IsReady())
		return;

	// poll input
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