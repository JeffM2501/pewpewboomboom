#include "SoundManager.h"

#include "raylib.h"

#include <unordered_map>
#include <vector>

constexpr size_t MaxInstances = 16;

namespace SoundManager
{
    struct SoundInstance
    {
        uint32_t ID;
        Sound MasterSFX;
        std::vector<Sound> Instances;
        int CurrentId = -1;
    };

    std::unordered_map<uint32_t, SoundInstance> Sounds;

    void Init()
    {
        InitAudioDevice();
    }

    void Cleanup()
    {
        for (auto& [key, sound] : Sounds)
        {
            for (auto& alias : sound.Instances)
                UnloadSoundAlias(alias);

            UnloadSound(sound.MasterSFX);
        }

        Sounds.clear();
    }

    void LoadSFX(uint32_t id, const char* path)
    {
        SoundInstance& sound = Sounds[id];
        sound.ID = id;
        sound.MasterSFX = LoadSound(path);
        sound.Instances.resize(MaxInstances);
        for (auto& inst : sound.Instances)
        {
            inst = LoadSoundAlias(sound.MasterSFX);
        }
    }

    void PlaySFX(uint32_t id)
    {
        auto inst = Sounds.find(id);
        if (inst == Sounds.end())
            return;

        auto& sound = inst->second;
        if (sound.CurrentId == -1)
            PlaySound(sound.MasterSFX);
        else
            PlaySound(sound.Instances[sound.CurrentId]);

        sound.CurrentId++;
        if (sound.CurrentId >= sound.Instances.size())
            sound.CurrentId = -1;
    }
}