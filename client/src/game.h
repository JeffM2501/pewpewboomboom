#include "log_system.h"
#include "world_data.h"
#include "protocol.h"
#include "client_world.h"

Logger& GetLogger();

extern ClientWorld World;

enum class StaticTextures
{
    Barrel,
    Box,
    Building,
    RoofDetail,
};
Texture GetTexture(StaticTextures texture);