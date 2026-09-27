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

Vector2 UpdatePlayerTransform(PlayerTransform& transform, const InputState& input, float deltaTime, PlayerMovementRules& rules)
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

    return transform.Position + transform.Velocity * (rules.MaxSpeed * deltaTime) * speedMultiplier;
}

PlayerTransform PlayerState::GetTransformAtTick(uint64_t tick) const
{
    if (TransformHistory.empty())
    {
        return Transform;
    }

    auto it = TransformHistory.find(tick);
    if (it != TransformHistory.end())
    {
        return it->second;
    }

    if (tick < TransformHistory.begin()->first)
    {
        return TransformHistory.begin()->second;
    }

    if (tick > TransformHistory.rbegin()->first)
    {
        const auto& latest = TransformHistory.rbegin()->second;
        uint64_t diffTicks = tick - TransformHistory.rbegin()->first;
        PlayerTransform extrapolated = latest;
        float dt = (1.0f / kDefaultTickRate) * float(diffTicks);
        extrapolated.Position = Vector2Add(latest.Position, Vector2Scale(latest.Velocity, dt));
        return extrapolated;
    }

    auto upper = TransformHistory.upper_bound(tick);
    auto lower = std::prev(upper);
    if (upper->first == lower->first)
    {
        return lower->second;
    }

    float t = float(tick - lower->first) / float(upper->first - lower->first);
    PlayerTransform interp = lower->second;
    interp.Position = Vector2Lerp(lower->second.Position, upper->second.Position, t);
    interp.Rotation[0] = LerpAngleDeg(lower->second.Rotation[0], upper->second.Rotation[0], t);
    interp.Rotation[1] = LerpAngleDeg(lower->second.Rotation[1], upper->second.Rotation[1], t);
    interp.Velocity = Vector2Lerp(lower->second.Velocity, upper->second.Velocity, t);
    return interp;
}