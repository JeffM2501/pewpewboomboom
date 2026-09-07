#include "external/fix_win32_compatibility.h"

#include "player_state.h"

#include "raymath.h"

float ClampInput(float input)
{
    if (input > 1)
        return 1;
    if (input < -1)
        return -1;

    return input;
}

bool UpdatePlayerTransform(PlayerTransform& transform, const InputState& input, float deltaTime, PlayerMovementRules& rules)
{
    float turn = ClampInput(input.Turn);
    float foward = ClampInput(input.Foward);

    transform.Rotation[0] += turn * (rules.TurnSpeed * deltaTime);
    transform.Rotation[1] = input.TurretAngle;

    Vector2 forwardDir = Vector2Rotate(Vector2{ 1, 0 }, transform.Rotation[0] * DEG2RAD);

    transform.Velocity = forwardDir * foward;

    float speedMultiplier = 1.0f;
    if (input.Boost)
        speedMultiplier = rules.BoostMultiplier;

    transform.Position += transform.Velocity * (rules.MaxSpeed * deltaTime) * speedMultiplier;
    return true;
}