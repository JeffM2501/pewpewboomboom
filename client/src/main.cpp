/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.
-- Game Premake by Jeffery Myers is marked CC0 1.0. To view a copy of this mark, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"
#include "raymath.h"

#include "game.h"    // an external header in this project
#include "lib.h"    // an external header in the static lib project


Matrix CubeTransform = MatrixIdentity();
Mesh CubeMesh = { 0 };
Material CubeMaterial = { 0 };

Camera3D ViewCamera = { 0 };


#include <mutex>
#include <deque>
#include <set>
#include <vector>

std::mutex  KeyLock;

struct KeyEvent
{
    KeyboardKey Key;
    bool Down;
};
std::set<KeyboardKey> DownKeys;
std::deque<KeyEvent> KeyEvents;

void PollRealInput()
{
    std::vector<KeyboardKey> deadKeys;
    for (const KeyboardKey& key : DownKeys)
    {
        if (!IsKeyDown(key))
        {
            std::lock_guard<std::mutex> lock(KeyLock);
            KeyEvents.push_back(KeyEvent{ key, false });
            deadKeys.push_back(key);
        }
    }

    for (auto key : deadKeys)
        DownKeys.erase(key);

    while (int key = GetKeyPressed() != 0)
    {
        std::lock_guard<std::mutex> lock(KeyLock);
        DownKeys.insert((KeyboardKey)key);
        KeyEvents.push_back(KeyEvent{ (KeyboardKey)key, true });
    }
}

KeyEvent PollKeyEvents()
{
    std::lock_guard<std::mutex> lock(KeyLock);
    if (KeyEvents.empty())
        return KeyEvent{ KEY_NULL, false };

    KeyEvent front = KeyEvents.front();
    KeyEvents.pop_front();
    return front;
}

void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(InitialWidth, InitialHeight, "Example");
    SetTargetFPS(144);

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
}

void GameCleanup()
{
    // unload resources
    UnloadMesh(CubeMesh);
    UnloadTexture(CubeMaterial.maps[MATERIAL_MAP_DIFFUSE].texture);

    CloseWindow();
}

bool GameUpdate()
{
    Matrix rotation = MatrixRotateXYZ({ 0.0f, GetFrameTime(), 0.0f });
    CubeTransform = MatrixMultiply(CubeTransform, rotation);
    return true;
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(GRAY);

    BeginMode3D(ViewCamera);
    DrawMesh(CubeMesh, CubeMaterial, CubeTransform);
    EndMode3D();

    DrawText("Hello Cube", 10, 10, 20, GetTextColor());

    EndDrawing();
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