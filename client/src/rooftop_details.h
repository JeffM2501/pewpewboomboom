#pragma once

#include "raylib.h"

namespace RooftopDetails
{
    // Texture dimensions: 2048 x 2048 (RGBA transparent)
    // Pure top-down orthographic plan view (zero visible sides, fully rotatable at 0-360 degrees)

    // --- HVAC Equipment ---
    inline constexpr Rectangle HvacQuadFan = { 24.0f, 24.0f, 480.0f, 375.0f };
    inline constexpr Rectangle HvacDoubleFan = { 528.0f, 24.0f, 616.0f, 241.0f };
    inline constexpr Rectangle HvacRtu3Panel = { 1168.0f, 24.0f, 712.0f, 230.0f };
    inline constexpr Rectangle HvacSquareFan = { 24.0f, 423.0f, 264.0f, 265.0f };
    inline constexpr Rectangle HvacCompact2Panel = { 312.0f, 423.0f, 357.0f, 150.0f };

    // --- Skylights ---
    inline constexpr Rectangle SkylightSquareA = { 693.0f, 423.0f, 150.0f, 146.0f };
    inline constexpr Rectangle SkylightSquareB = { 867.0f, 423.0f, 145.0f, 146.0f };
    inline constexpr Rectangle SkylightRect = { 1036.0f, 423.0f, 173.0f, 148.0f };
    inline constexpr Rectangle SkylightSquareC = { 1233.0f, 423.0f, 149.0f, 147.0f };

    // --- Vents & Exhaust Fans ---
    inline constexpr Rectangle VentSquareLouver = { 1406.0f, 423.0f, 156.0f, 148.0f };
    inline constexpr Rectangle VentCircularTurbine = { 1586.0f, 423.0f, 139.0f, 141.0f };
    inline constexpr Rectangle VentCircularGrille = { 1749.0f, 423.0f, 146.0f, 148.0f };
    inline constexpr Rectangle VentCircularCap = { 24.0f, 712.0f, 146.0f, 148.0f };
    inline constexpr Rectangle VentCircularFan = { 194.0f, 712.0f, 145.0f, 147.0f };

    // --- Ductwork ---
    inline constexpr Rectangle DuctElbowA = { 363.0f, 712.0f, 157.0f, 165.0f };
    inline constexpr Rectangle DuctElbowB = { 544.0f, 712.0f, 161.0f, 166.0f };
    inline constexpr Rectangle DuctTJunction = { 729.0f, 712.0f, 192.0f, 163.0f };
    inline constexpr Rectangle DuctCrossA = { 945.0f, 712.0f, 183.0f, 186.0f };
    inline constexpr Rectangle DuctCrossB = { 1152.0f, 712.0f, 186.0f, 186.0f };

    // --- Piping ---
    inline constexpr Rectangle PipeManifold = { 1362.0f, 712.0f, 566.0f, 183.0f };
    inline constexpr Rectangle PipeStraight = { 24.0f, 922.0f, 185.0f, 63.0f };
    inline constexpr Rectangle PipeStraightValve = { 233.0f, 922.0f, 185.0f, 70.0f };

    // --- Roof Access ---
    inline constexpr Rectangle RoofAccessHatch = { 442.0f, 922.0f, 177.0f, 178.0f };

    enum class Details
    {
        MIN = 0,
        HvacQuadFan = 0,
        HvacDoubleFan,
        HvacRtu3Panel,
        HvacSquareFan,
        HvacCompact2Panel,
        SkylightSquareA,
        SkylightSquareB,
        SkylightRect,
        SkylightSquareC,
        VentSquareLouver,
        VentCircularTurbine,
        VentCircularGrille,
        VentCircularCap,
        VentCircularFan,
        DuctElbowA,
        DuctElbowB,
        DuctTJunction,
        DuctCrossA,
        DuctCrossB,
        PipeManifold,
        PipeStraight,
        PipeStraightValve,
        RoofAccessHatch,
        MAX
    };

    inline Rectangle GetDetailSpriteRect(Details detail)
    {
        switch (detail)
        {
        case Details::HvacQuadFan: return HvacQuadFan;
        case Details::HvacDoubleFan: return HvacDoubleFan;
        case Details::HvacRtu3Panel: return HvacRtu3Panel;
        case Details::HvacSquareFan: return HvacSquareFan;
        case Details::HvacCompact2Panel: return HvacCompact2Panel;
        case Details::SkylightSquareA: return SkylightSquareA;
        case Details::SkylightSquareB: return SkylightSquareB;
        case Details::SkylightRect: return SkylightRect;
        case Details::SkylightSquareC: return SkylightSquareC;
        case Details::VentSquareLouver: return VentSquareLouver;
        case Details::VentCircularTurbine: return VentCircularTurbine;
        case Details::VentCircularGrille: return VentCircularGrille;
        case Details::VentCircularCap: return VentCircularCap;
        case Details::VentCircularFan: return VentCircularFan;
        case Details::DuctElbowA: return DuctElbowA;
        case Details::DuctElbowB: return DuctElbowB;
        case Details::DuctTJunction: return DuctTJunction;
        case Details::DuctCrossA: return DuctCrossA;
        case Details::DuctCrossB: return DuctCrossB;
        case Details::PipeManifold: return PipeManifold;
        case Details::PipeStraight: return PipeStraight;
        case Details::PipeStraightValve: return PipeStraightValve;
        case Details::RoofAccessHatch: return RoofAccessHatch;
        }

        return RoofAccessHatch;
    }

    inline Details GetRandomDetail()
    {
        int val = GetRandomValue(int(Details::MIN), int(Details::MAX));
        return Details(val);
    }
}
