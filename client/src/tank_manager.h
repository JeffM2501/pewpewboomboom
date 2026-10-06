#pragma once

#include "player_state.h"

namespace TankManager
{
	void Init();
	void Cleanup();

	void DrawTank(TeamColors color, const PlayerTransform& transform, bool showArrow = false);
}