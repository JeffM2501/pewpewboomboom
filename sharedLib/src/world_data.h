#pragma once

#include "protocol.h"
#include <vector>

class WorldData
{
public:
    std::vector<S2C_SetWorldObject> WorldObjects;

    void AddObject(const S2C_SetWorldObject& object);
};