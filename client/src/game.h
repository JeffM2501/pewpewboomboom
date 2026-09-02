#include "log_system.h"
#include "world_data.h"

Logger& GetLogger();

class WorldData
{
public:
	std::vector<S2C_SetWorldObject> WorldObjects;

	void AddObject(const S2C_SetWorldObject& object)
	{
		WorldObjects.push_back(object);
	}
};

class ClientWorld : public WorldData
{
public:
	uint64_t Count;

	BoundingBox Bounds = { 0 };

	bool Loading = false;
};
extern ClientWorld World;