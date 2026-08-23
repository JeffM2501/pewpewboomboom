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
#include "client_network_manager.h" 
#include "connection_dialog.h" 

#include "game_gui.h"
#include "guiControls/chat_window.h"

Matrix CubeTransform = MatrixIdentity();
Mesh CubeMesh = { 0 };
Material CubeMaterial = { 0 };

Camera3D ViewCamera = { 0 };

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

void GameInit()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 800, "Example");
	SetTargetFPS(144);

	rlImGuiSetup(true);

	Network.GetEvents().OnTick.Add(ProcessNetTick);

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

	Network.GetEvents().OnSpawn.Add([](const Vector2& spawn, void*) { CubeTransform = MatrixTranslate(spawn.x, 0, spawn.y); });

	ChatWindow::AddChatLine(nullptr, "Client Startup");
}

void GameCleanup()
{
	rlImGuiShutdown();
	Network.Disconnect();
	// unload resources
	UnloadMesh(CubeMesh);
	UnloadTexture(CubeMaterial.maps[MATERIAL_MAP_DIFFUSE].texture);

	CloseWindow();
}

bool GameUpdate()
{
	Network.Update();
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