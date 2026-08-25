#include "log_system.h"
#include "world_data.h"

Logger& GetLogger();

class ClientWorld : public WorldData
{
public:
    uint64_t Count;

    BoundingBox Bounds = { 0 };

    bool Loading = false;
};
extern ClientWorld World;