#include "mini_map.h"

#include "imgui.h"
#include "rlImGui.h"
#include "raylib.h"
#include "game.h"
#include "rlgl.h"
#include "raymath.h"

#include "client_network_manager.h"

namespace MiniMap
{
	static ImVec2 WindowSize(250, 250);

	static RenderTexture2D MiniMapTexture = { 0 };

	void UpdateMiniMapTexture()
	{
		if (MiniMapTexture.texture.width != int(WindowSize.x))
		{
			if (MiniMapTexture.id != 0)
			{
				UnloadRenderTexture(MiniMapTexture);
				MiniMapTexture.id = 0;
			}
		}

		if (MiniMapTexture.id == 0)
		{
			MiniMapTexture = LoadRenderTexture(int(WindowSize.x), int(WindowSize.y));
		}
		BeginTextureMode(MiniMapTexture);
		ClearBackground(BLANK);
		Camera2D camera = { 0 };
		camera.offset = { WindowSize.x * 0.5f, WindowSize.y * 0.5f };
		camera.target = { 0,0 };

		camera.zoom = 1;
		if (World.IsValid())
		{
			camera.zoom = WindowSize.x / World.WallSize.x;
		}

		BeginMode2D(camera);
		if (World.IsValid())
		{
			World.Draw();
		}

		Network.GetPlayerList().DoForEachPlayer([](ClientPlayerState* player)
			{
				DrawCircleV(player->Transform.Position, 5, player->IsLocalPlayer ? GREEN : RED);
			});

		EndMode2D();
		EndTextureMode();
	}

	void Show()
	{
		UpdateMiniMapTexture();

		rlPushMatrix();
		float padding = 10;

		rlTranslatef(GetScreenWidth()-WindowSize.x- (padding*2), GetScreenHeight()-WindowSize.y- (padding*2), 0);

		Rectangle bounds{ padding, padding, WindowSize.x, WindowSize.y };
		DrawRectangleRec(Rectangle{ 0, 0, WindowSize.x + (padding * 2), WindowSize.y + (padding * 2) }, ColorAlpha(DARKBLUE, 0.75f));

		DrawRectangleRec(bounds, ColorAlpha(DARKGRAY, 0.75f));
		DrawTexturePro(MiniMapTexture.texture, Rectangle{ 0, 0, float(MiniMapTexture.texture.width), float(-MiniMapTexture.texture.height) }, bounds, Vector2Zeros, 0, WHITE);
		rlPopMatrix();
	}
}