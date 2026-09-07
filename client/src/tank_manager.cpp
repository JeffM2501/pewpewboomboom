#include "tank_manager.h"
#include "raylib.h"
#include "raymath.h"
#include <map>

namespace TankManager
{
	struct TankInfo
	{
		Texture2D BodyTexture;
		Texture2D TurretTexture;

		float BodyScale = 1.0f/128.0f;
		float TurretScale = 1.0f / 128.0f;;
		Vector2 BodyOrigin;
		Vector2 TurretOrigin;
	};

	std::map<TeamColors, TankInfo> TankResources;

    Texture FowardArrow = { 0 };

	Texture2D LoadTextureFromFile(std::string_view path)
	{
		std::string resourcePath = "resources/tanks/" + std::string(path);
		Image image = LoadImage(resourcePath.c_str());
		ImageRotateCW(&image);
		Texture2D texture = LoadTextureFromImage(image); 
		UnloadImage(image);

		GenTextureMipmaps(&texture);
		SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);

		return texture;
	}

	void LoadTankInfo(std::string_view bodyTexturePath, std::string_view turretTexturePath, TeamColors color)
	{
		TankInfo info;
		info.BodyTexture = LoadTextureFromFile(bodyTexturePath);
		info.TurretTexture = LoadTextureFromFile(turretTexturePath);

		info.BodyOrigin = { (float)info.BodyTexture.width / 2.0f, (float)info.BodyTexture.height / 2.0f };

		float turretHeightOffset = (float)info.TurretTexture.height / 2; // we offset by only the height, since some turrets are long
		info.TurretOrigin = { (float)turretHeightOffset, (float)turretHeightOffset };

		info.BodyOrigin *= info.BodyScale;
		info.TurretOrigin *= info.TurretScale;

		TankResources[color] = info;
	}
	void Init()
	{
		LoadTankInfo("hull10_blue2.png", "turret10_blue.png", TeamColors::Blue);
		LoadTankInfo("hull07_red.png", "turret02_red2.png", TeamColors::Red);
		LoadTankInfo("hull10_purple2.png", "turret10_purple.png", TeamColors::Purple);
		LoadTankInfo("hull10_yellow2.png", "turret10_yellow.png", TeamColors::Yellow);
		LoadTankInfo("hull10_white2.png", "turret10_white.png", TeamColors::White);
		LoadTankInfo("hull10_black2.png", "turret10_black.png", TeamColors::Black);
		LoadTankInfo("hull05_green.png", "turret07_green.png", TeamColors::Green);
		FowardArrow = LoadTextureFromFile("arrow_decorative_n.png");
	}
	void Cleanup()
	{
		for (auto& [color, info] : TankResources)
		{
			UnloadTexture(info.BodyTexture);
			UnloadTexture(info.TurretTexture);
		}

		TankResources.clear();
	}

	void DrawTank(TeamColors color, const PlayerTransform& transform, bool showArrow)
	{
		TankInfo& tankInfo = TankResources[color];
		Rectangle playerRect = { transform.Position.x, transform.Position.y, tankInfo.BodyTexture.width * tankInfo.BodyScale, tankInfo.BodyTexture.height * tankInfo.BodyScale };

		if (showArrow)
		{
            Rectangle forwardRect = { transform.Position.x, transform.Position.y, 2.0f, 2.0f };
            Vector2 forwardOrigin = { -playerRect.width/1.5f, forwardRect.height / 2 };
            DrawTexturePro(FowardArrow, Rectangle{ 0,0,float(FowardArrow.width), float(FowardArrow.height) }, forwardRect, forwardOrigin, transform.Rotation[0], ColorAlpha(WHITE, 0.25f));
		}

		Rectangle srcRect = { 0, 0, (float)tankInfo.BodyTexture.width, (float)tankInfo.BodyTexture.height };
		DrawTexturePro(tankInfo.BodyTexture, srcRect, playerRect, tankInfo.BodyOrigin, transform.Rotation[0], WHITE);

		Rectangle turretRect = { transform.Position.x, transform.Position.y, tankInfo.TurretTexture.width * tankInfo.TurretScale, tankInfo.TurretTexture.height * tankInfo.TurretScale };

		srcRect = { 0, 0, (float)tankInfo.TurretTexture.width, (float)tankInfo.TurretTexture.height };
		DrawTexturePro(tankInfo.TurretTexture, srcRect, turretRect, tankInfo.TurretOrigin, transform.Rotation[1], WHITE);
	}
}