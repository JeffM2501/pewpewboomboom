#pragma once

#include "player_state.h"

enum class TeamColors
{
	Blue = 0,
	Red = 1,
	Purple = 2,
	Yellow = 3,
	White = 4,
	Black = 5,
};

namespace TankManager
{
	void Init();
	void Cleanup();

	void DrawTank(TeamColors color, const PlayerTransform& transform);
}