#pragma once

#include <cstdint>

namespace SoundManager
{
    void Init();
    void Cleanup();

    void LoadSFX(uint32_t id, const char* path);
    void PlaySFX(uint32_t id);
}

static constexpr uint32_t BoomSound = 1;
static constexpr uint32_t PewSound = 2;