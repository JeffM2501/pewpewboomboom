#include "client_world.h"
#include "raymath.h"
#include "rlgl.h"
#include "game.h"

// ClientWorldWalls
ClientWorldWalls::ClientWorldWalls(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Collider.Size.x = message.scale;
    Collider.Size.y = message.scale;

    Bounds.Radius = message.scale;
}

void ClientWorldWalls::Draw()
{
    Rectangle rect{
        -(Collider.Size.x)-10,
        -(Collider.Size.y)-10,
        Collider.Size.x * 2.0f + 20,
        Collider.Size.y * 2.0f + 20
    };
    DrawRectangleLinesEx(rect, 10, Tint);
}

static float GetBuildingBorderSize(float size)
{
    return std::min(3.0f, size * 0.25f);
}

// ClientWorldBuilding
ClientWorldBuilding::ClientWorldBuilding(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Collider.Size.x = message.scale;
    Collider.Size.y = message.scale;

    Collider.Bounds.Center.x = message.position[0];
    Collider.Bounds.Center.y = message.position[1];
    Collider.Bounds.Radius = Vector2Length(Vector2(message.scale, message.scale));

    Collider.Rotation = message.rotation;

    float detailScale = 0.030f;
    float scale = message.scale * detailScale;

    int min = std::max(1, int(message.scale * 0.1f));
    int max = std::max(5, int(message.scale * 1.0f));

    int count = GetRandomValue(min, max);
    for (int i = 0; i < count; i++)
    {
        auto detail = RooftopDetails::GetRandomDetail();
        Rectangle sourceRect = RooftopDetails::GetDetailSpriteRect(detail);

        DetailInfo info;
        Vector2 offset = { GetRandomValue(0, 1000) / 1000.0f, GetRandomValue(0, 1000) / 1000.0f };
        Vector2 size = { sourceRect.width * detailScale, sourceRect.height * detailScale };

        float border = GetBuildingBorderSize(message.scale);

        Vector2 testMin = { -message.scale + border , -message.scale + border };
        Vector2 testMax = { (message.scale) - (border + size.x), (message.scale) - (border + size.y) };

        Rectangle testRect = { testMin.x, testMin.y,  testMax.x-testMin.x, testMax.y-testMin.y};

        if (size.x >= testRect.width || size.y >= testRect.height)
            continue;

        info.DestinationRect = Rectangle{ testRect.x + (testRect.width * offset.x), testRect.y + (testRect.height * offset.y), size.x, size.y};

        info.Detail = detail;

        bool valid = true;

        float extends = message.scale * 2 - border;

        Vector2 end = { info.DestinationRect.x + info.DestinationRect.width, info.DestinationRect.y + info.DestinationRect.height };

        if (end.x >= extends || end.y >= extends)
            valid = false;

        for (const auto& other : Details)
        {
            if (CheckCollisionRecs(info.DestinationRect, other.DestinationRect))
                valid = false;
        }

        if (valid)
            Details.push_back(info);
    }
}

void UpperLeft(Vector2 size, float borderSize, float quarterSize)
{
    rlColor4f(1, 1, 1, 1);
    rlTexCoord2f(0, 0);
    rlVertex2f(-size.x, -size.y);

    rlTexCoord2f(0, borderSize);
    rlVertex2f(-size.x, -size.y + quarterSize);

    rlTexCoord2f(borderSize, borderSize);
    rlVertex2f(-size.x + quarterSize, -size.y + quarterSize);

    rlTexCoord2f(borderSize, 0);
    rlVertex2f(-size.x + quarterSize, -size.y);
}

void UpperRight(Vector2 size, float borderSize, float quarterSize)
{
    rlColor4f(1, 1, 1, 1);
    rlTexCoord2f(1.0f-borderSize, 0);
    rlVertex2f(size.x - quarterSize, -size.y);

    rlTexCoord2f(1.0f-borderSize, borderSize);
    rlVertex2f(size.x - quarterSize, -size.y + quarterSize);

    rlTexCoord2f(1.0f, borderSize);
    rlVertex2f(size.x, -size.y + quarterSize);

    rlTexCoord2f(1.0f, 0);
    rlVertex2f(size.x, -size.y);
}

void LowerLeft(Vector2 size, float borderSize, float quarterSize)
{
    rlColor4f(1, 1, 1, 1);
    rlTexCoord2f(0, 1.0f - borderSize);
    rlVertex2f(-size.x, size.y-quarterSize);

    rlTexCoord2f(0, 1.0f);
    rlVertex2f(-size.x, size.y);

    rlTexCoord2f(borderSize, 1.0f);
    rlVertex2f(-size.x + quarterSize, size.y);

    rlTexCoord2f(borderSize, 1.0f - borderSize);
    rlVertex2f(-size.x + quarterSize, size.y - quarterSize);
}

void LowerRight(Vector2 size, float borderSize, float quarterSize)
{
    rlColor4f(1, 1, 1, 1);
    rlTexCoord2f(1.0f - borderSize, 1.0f-borderSize);
    rlVertex2f(size.x - quarterSize, size.y - quarterSize);

    rlTexCoord2f(1.0f - borderSize, 1.0f);
    rlVertex2f(size.x - quarterSize, size.y);

    rlTexCoord2f(1.0f, 1.0f);
    rlVertex2f(size.x, size.y);

    rlTexCoord2f(1.0f, 1.0f - borderSize);
    rlVertex2f(size.x, size.y - quarterSize);
}

void Center(Vector2 size, float borderSize, float quarterSize)
{
    rlColor4f(1, 1, 1, 1);
    rlTexCoord2f(borderSize, borderSize);
    rlVertex2f(-size.x + quarterSize, -size.y + quarterSize);

    rlTexCoord2f(borderSize, 1.0f-borderSize);
    rlVertex2f(-size.x + quarterSize, size.y - quarterSize);

    rlTexCoord2f(1.0f - borderSize, 1.0f - borderSize);
    rlVertex2f(size.x - quarterSize, size.y - quarterSize);

    rlTexCoord2f(1.0f - borderSize, borderSize);
    rlVertex2f(size.x - quarterSize, -size.y + quarterSize);
}

void Top(Vector2 size, float borderSize, float quarterSize)
{
    rlTexCoord2f(borderSize, 0);
    rlVertex2f(-size.x + quarterSize, -size.y);

    rlTexCoord2f(borderSize, borderSize);
    rlVertex2f(-size.x + quarterSize, -size.y + quarterSize);

    rlTexCoord2f(1.0f - borderSize, borderSize);
    rlVertex2f(size.x - quarterSize, -size.y + quarterSize);

    rlTexCoord2f(1.0f - borderSize, 0);
    rlVertex2f(size.x - quarterSize, -size.y);
}

void Bottom(Vector2 size, float borderSize, float quarterSize)
{
    rlTexCoord2f(borderSize, 1.0f-borderSize);
    rlVertex2f(-size.x + quarterSize, size.y - quarterSize);

    rlTexCoord2f(borderSize, 1.0f);
    rlVertex2f(-size.x + quarterSize, size.y);

    rlTexCoord2f(1.0f - borderSize, 1.0f);
    rlVertex2f(size.x - quarterSize, size.y);

    rlTexCoord2f(1.0f - borderSize, 1.0f - borderSize);
    rlVertex2f(size.x - quarterSize, size.y - quarterSize);
}

void Left(Vector2 size, float borderSize, float quarterSize)
{
    rlTexCoord2f(0, borderSize);
    rlVertex2f(-size.x, -size.y + quarterSize);

    rlTexCoord2f(0, 1.0f- borderSize);
    rlVertex2f(-size.x, size.y - quarterSize);

    rlTexCoord2f(borderSize, 1.0f - borderSize);
    rlVertex2f(-size.x + quarterSize, size.y - quarterSize);

    rlTexCoord2f(borderSize, borderSize);
    rlVertex2f(-size.x + quarterSize, -size.y + quarterSize);
}

void Right(Vector2 size, float borderSize, float quarterSize)
{
    rlTexCoord2f(1.0f - borderSize, borderSize);
    rlVertex2f(size.x - quarterSize, -size.y+quarterSize);

    rlTexCoord2f(1.0f - borderSize, 1.0f - borderSize);
    rlVertex2f(size.x - quarterSize, size.y - quarterSize);

    rlTexCoord2f(1.0f, 1.0f - borderSize);
    rlVertex2f(size.x, size.y - quarterSize);

    rlTexCoord2f(1.0f, borderSize);
    rlVertex2f(size.x, -size.y + quarterSize);
}

void ClientWorldBuilding::Draw()
{
    auto building = GetTexture(StaticTextures::Building); 
    Rectangle rect = { 0,0, float(building.width), float(building.height) };
    Rectangle dest{
      -Collider.Size.x,
      -Collider.Size.y,
      Collider.Size.x * 2.0f,
      Collider.Size.y * 2.0f
    };

    Rectangle shadow = dest;
    shadow.x -= 1;
    shadow.y -= 1;
    shadow.width += 2;
    shadow.height += 2;

    DrawRectangleRec(shadow, ColorAlpha(BLACK, 0.25f));

    float quarterSize = GetBuildingBorderSize(Collider.Size.x);
    float borderSize = 150.0f / building.width;
    rlBegin(RL_QUADS);
    rlSetTexture(building.id);
    rlColor4ub(Tint.r, Tint.g, Tint.b, Tint.a);
 
    UpperLeft(Collider.Size, borderSize, quarterSize);
    UpperRight(Collider.Size, borderSize, quarterSize);
    LowerLeft(Collider.Size, borderSize, quarterSize);
    LowerRight(Collider.Size, borderSize, quarterSize);
    Center(Collider.Size, borderSize, quarterSize);

    Top(Collider.Size, borderSize, quarterSize);
    Bottom(Collider.Size, borderSize, quarterSize);
    Left(Collider.Size, borderSize, quarterSize);
    Right(Collider.Size, borderSize, quarterSize);

    rlEnd();
 
    rlSetTexture(0);

    auto detailTexture = GetTexture(StaticTextures::RoofDetail);
    for (const auto& detail : Details)
    {
        DrawTexturePro(detailTexture, RooftopDetails::GetDetailSpriteRect(detail.Detail), detail.DestinationRect, Vector2Zeros, 0, WHITE);
    }
}

void ClientWorldBuilding::DrawMiniMap()
{
    Rectangle dest{
       -Collider.Size.x,
       -Collider.Size.y,
       Collider.Size.x * 2.0f,
       Collider.Size.y * 2.0f
    };
    DrawRectangleRec(dest, GRAY);
    DrawRectangleLinesEx(dest, 4, ColorAlpha(MAROON, 0.5f));
}

// ClientWorldBox
ClientWorldBox::ClientWorldBox(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Collider.Size.x = message.scale;
    Collider.Size.y = message.scale;

    Collider.Bounds.Center.x = message.position[0];
    Collider.Bounds.Center.y = message.position[1];
    Collider.Bounds.Radius = Vector2Length(Vector2(message.scale, message.scale));

    Collider.Rotation = message.rotation;
}

void ClientWorldBox::Draw()
{
    auto box = GetTexture(StaticTextures::Box);
    Rectangle rect = { 0.0f, 0.0f, static_cast<float>(box.width), static_cast<float>(box.height) };
    Rectangle dest{
     -Collider.Size.x,
     -Collider.Size.y,
     Collider.Size.x * 2.0f,
     Collider.Size.y * 2.0f
    };

    Rectangle shadow = dest;
    float offset = 0.25f;

    shadow.x -= offset;
    shadow.y -= offset;
    shadow.width += offset*2;
    shadow.height += offset*2;
    DrawRectangleRec(shadow, ColorAlpha(DARKGREEN, 0.95f));

    DrawTexturePro(box, rect, dest, Vector2Zeros, 0, Tint);
}

void ClientWorldBox::DrawMiniMap()
{
    Rectangle dest{
       -Collider.Size.x,
       -Collider.Size.y,
       Collider.Size.x * 2.0f,
       Collider.Size.y * 2.0f
    };
    DrawRectangleRec(dest, BROWN);
}

// ClientWorldBarrel
ClientWorldBarrel::ClientWorldBarrel(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Collider.Bounds.Center.x = message.position[0];
    Collider.Bounds.Center.y = message.position[1];
    Collider.Bounds.Radius = message.scale;

    DrawSize = message.scale * 2;
}

void ClientWorldBarrel::Draw()
{
    auto barrel = GetTexture(StaticTextures::Barrel);
    Rectangle rect = { 0.0f, 0.0f, static_cast<float>(barrel.width), static_cast<float>(barrel.height) };
    Rectangle dest = { -Collider.Bounds.Radius, -Collider.Bounds.Radius, DrawSize, DrawSize };

    float offset = (sinf(float(GetTime() * 4.0f)) + 1) + 0.25f;
    DrawCircleGradient(Vector2Zeros, Collider.Bounds.Radius + offset, ColorAlpha(MAROON, 0.75f), ColorAlpha(RED, 0.125f));

    DrawTexturePro(barrel, rect, dest, Vector2Zeros, 0, Tint);
}

void ClientWorldBarrel::DrawMiniMap()
{
    DrawCircleV(Vector2Zeros, Collider.Bounds.Radius, RED);
}

// ClientWorld
void ClientWorld::Init(const S2C_SetWorldInfo& message)
{
    Name = message.name;
    Count = message.objectCount;
    Objects.clear();

    WorldStarted.Invoke(*this);
}

void ClientWorld::Receive(const S2C_SetWorldObject& message)
{
    switch (message.objType)
    {
    case S2C_SetWorldObject::ObjectType::Walls:
        Walls = std::make_unique<ClientWorldWalls>(message);
		WallSize.x = message.scale;
        WallSize.y = message.scale;
        break;
    case S2C_SetWorldObject::ObjectType::Building:
        Objects.push_back(std::make_unique<ClientWorldBuilding>(message));
        break;
    case S2C_SetWorldObject::ObjectType::Box:
        Objects.push_back(std::make_unique<ClientWorldBox>(message));
        break;
    case S2C_SetWorldObject::ObjectType::Barrel:
        Objects.push_back(std::make_unique<ClientWorldBarrel>(message));
        break;
    default:
        break;
    }

    if (Objects.size() + (Walls ? 1 : 0) == Count)
    {
        WorldFinalized.Invoke(*this);
    }
}

void ClientWorld::Draw()
{
    if (Walls)
    {
        Walls->Draw();
    }

    for (auto& object : Objects)
	{
		rlPushMatrix();
		rlTranslatef(object->GetBoundingCircle().Center.x, object->GetBoundingCircle().Center.y, 0);
		rlRotatef(object->GetRotation(), 0, 0, 1);

		object->Draw();
		rlPopMatrix();
	}
}

void ClientWorld::DrawMiniMap()
{
    if (Walls)
    {
        Walls->DrawMiniMap();
    }

    for (auto& object : Objects)
    {
        rlPushMatrix();
        rlTranslatef(object->GetBoundingCircle().Center.x, object->GetBoundingCircle().Center.y, 0);
        rlRotatef(object->GetRotation(), 0, 0, 1);

        object->DrawMiniMap();
        rlPopMatrix();
    }
}

void ClientWorld::DoForEachObject(BoundingCircle& area, std::function<void(WorldObject& object)> func)
{
    for (auto& object : Objects)
    {
        if (CheckCollisionCircles(area.Center, area.Radius, object->GetBoundingCircle().Center, object->GetBoundingCircle().Radius))
            func(*object.get());
    }

    if (CheckCollisionCircles(area.Center, area.Radius, Walls->GetBoundingCircle().Center, Walls->GetBoundingCircle().Radius))
        func(*Walls.get());
}