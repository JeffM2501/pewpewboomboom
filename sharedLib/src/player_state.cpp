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

    float turnRate = turn * rules.TurnSpeed;
    transform.Rotation[0] += turnRate * deltaTime;

    float prevTurret = transform.Rotation[1];
    transform.Rotation[1] = input.TurretAngle;

    transform.AngularVelocity[0] = turnRate;
    if (deltaTime > 0.0f)
    {
        transform.AngularVelocity[1] = AngleDiffDeg(prevTurret, input.TurretAngle) / deltaTime;
    }
    else
    {
        transform.AngularVelocity[1] = 0.0f;
    }

    Vector2 forwardDir = Vector2Rotate(Vector2{ 1, 0 }, transform.Rotation[0] * DEG2RAD);

    float speedMultiplier = 1.0f;
    if (input.Boost)
    {
        speedMultiplier = rules.BoostMultiplier;
    }

    transform.Velocity = forwardDir * (foward * rules.MaxSpeed * speedMultiplier);

    return transform.Position + transform.Velocity * deltaTime;
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
        float dt = (1.0f / kDefaultTickRate) * float(diffTicks);
        return PredictTransform(latest, dt);
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
    interp.AngularVelocity[0] = Lerp(lower->second.AngularVelocity[0], upper->second.AngularVelocity[0], t);
    interp.AngularVelocity[1] = Lerp(lower->second.AngularVelocity[1], upper->second.AngularVelocity[1], t);
    return interp;
}

PlayerTransform PredictTransform(const PlayerTransform& transform, float deltaTime)
{
    PlayerTransform predicted = transform;
    predicted.Position = Vector2Add(transform.Position, Vector2Scale(transform.Velocity, deltaTime));
    predicted.Rotation[0] = fmodf(transform.Rotation[0] + transform.AngularVelocity[0] * deltaTime, 360.0f);
    if (predicted.Rotation[0] < 0.0f)
    {
        predicted.Rotation[0] += 360.0f;
    }
    predicted.Rotation[1] = fmodf(transform.Rotation[1] + transform.AngularVelocity[1] * deltaTime, 360.0f);
    if (predicted.Rotation[1] < 0.0f)
    {
        predicted.Rotation[1] += 360.0f;
    }
    return predicted;
}

