// ============================================================================
// GTASA_STRUCTS.h
// ----------------------------------------------------------------------------
// Minimal compatible struct header for GTA:SA Android libGTASA.so v2.10
// (32-bit / armeabi-v7a build, AML32 path).
//
// IMPORTANT:
// This is NOT the original/official GTASA_STRUCTS.h used internally by the
// AndroidModLoader team (that file is not published anywhere on GitHub and
// is privately shared in the AML community). This is a hand-built,
// minimal-compatible shim containing ONLY the symbols actually referenced by
// SAShaderL (ES3Shader.h + main.cpp), so the project can compile.
//
// Field layouts below are based on:
//  - Offsets/RTTI verified via radare2/Ghidra on libGTASA.so v2.10 (32-bit)
//  - Known RenderWare / GTA:SA structure conventions (PC/mobile parity)
//  - Symbol names cross-referenced from other AndroidModLoader mods
//    (GTASA_PedFuncs, GTASA_Timecyc24, SAUtils) that depend on the same
//    headers.
//
// If you already have the real GTASA_STRUCTS.h (e.g. from the AML Discord),
// REPLACE this file with that one — it will have far more complete and
// verified field layouts. Treat this file as a temporary placeholder to get
// the build green, not as ground truth.
// ============================================================================

#ifndef _GTASA_STRUCTS_H
#define _GTASA_STRUCTS_H

#include <stdint.h>

// ----------------------------------------------------------------------------
// Basic math types (RenderWare / GTA:SA convention)
// ----------------------------------------------------------------------------

class CVector
{
public:
    float x, y, z;

    CVector() : x(0.0f), y(0.0f), z(0.0f) {}
    CVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

class CVector2D
{
public:
    float x, y;
};

class CRGBA
{
public:
    uint8_t r, g, b, a;
};

// ----------------------------------------------------------------------------
// RenderWare-side camera (RwCamera). Only the fields used by SAShaderL
// (farClip) are laid out precisely; the rest are padding placeholders.
// Verified against the FarClipDist usage in SAShaderL/main.cpp:
//   TheCamera->m_pRwCamera->farClip
// ----------------------------------------------------------------------------
struct RwCamera
{
    uint8_t  _pad0[0x9C];   // RwObject header + view/projection matrices etc.
    float    nearClip;      // 0x9C (approx, RW3.x layout)
    float    farClip;       // 0xA0 (approx, RW3.x layout)
    uint8_t  _pad1[0x40];   // remaining camera fields (fog, viewport, etc.)
};

// ----------------------------------------------------------------------------
// CCamera — only m_pRwCamera is consumed by SAShaderL.
// Offset of m_pRwCamera inside CCamera must match what's used for
// FarClipDist (TheCamera->m_pRwCamera->farClip). Adjust _pad0 size if your
// own radare2/Ghidra pass on this v2.10 binary finds a different offset.
// ----------------------------------------------------------------------------
class CCamera
{
public:
    uint8_t   _pad0[0x18];   // CEntity-like / matrix-handle fields, camera modes
    RwCamera* m_pRwCamera;    // pointer to active RenderWare camera
    uint8_t   _pad1[0x400];  // remainder of CCamera (very large in GTA:SA)
};

// ----------------------------------------------------------------------------
// CEntity — only the type & model index fields are needed.
// Matches usage in SAShaderL/main.cpp:
//   self->m_nModelIndex
//   self->m_nType
// These offsets/bitfields follow the well-known GTA:SA CEntity layout
// (consistent across PC/mobile ports at this game version).
// ----------------------------------------------------------------------------
enum eEntityType : uint8_t
{
    ENTITY_TYPE_NOTHING  = 0,
    ENTITY_TYPE_BUILDING = 1,
    ENTITY_TYPE_VEHICLE  = 2,
    ENTITY_TYPE_PED      = 3,
    ENTITY_TYPE_OBJECT   = 4,
    ENTITY_TYPE_DUMMY    = 5,
};

class CEntity
{
public:
    void*    vtable;          // 0x00 vtable ptr (polymorphic class)
    void*    m_rwObject;      // 0x04 RpAtomic/RpClump*
    // --- bitfield word: contains m_nType among others ---
    uint32_t m_nType      : 3; // entity type (eEntityType)
    uint32_t m_nStatus     : 5;
    uint32_t m_bUsesCollision : 1;
    uint32_t m_bCollisionProcessed : 1;
    uint32_t m_bIsStatic   : 1;
    uint32_t m_bHasContacted : 1;
    uint32_t m_bPedPhysics : 1;
    uint32_t m_bIsStuck    : 1;
    uint32_t m_bIsInSafePosition : 1;
    uint32_t m_bWasPostponed : 1;
    uint32_t m_bExplosionProof : 1;
    uint32_t m_bIsVisible  : 1;
    uint32_t m_bHasCollided : 1;
    uint32_t m_bRenderDamaged : 1;
    uint32_t m_bBulletProof : 1;
    uint32_t m_bFireProof   : 1;
    uint32_t m_bCollisionProof : 1;
    uint32_t m_bMeleeProof  : 1;
    uint32_t m_bOnlyDamagedByPlayer : 1;
    uint32_t m_bStreamingDontDelete : 1;
    uint32_t m_bZoneCulled  : 1;
    uint32_t m_bZoneCulled2 : 1;
    uint32_t m_bRemoveFromWorld : 1;
    uint32_t m_bHasHitWall  : 1;
    uint32_t m_bImBeingRendered : 1;
    uint32_t m_bDrawLast    : 1;
    uint32_t m_bDistanceFade : 1;

    uint8_t  _pad0[0x14];      // matrix handle / reference / lod fields...

    int16_t  m_nModelIndex;    // model index (-1 if none)
    int16_t  _pad1;

    uint8_t  _pad2[0x40];      // remainder of CEntity (PADDING ONLY)
};

// ----------------------------------------------------------------------------
// ES2Shader — base class of ES3Shader (see ES3Shader.h in this repo).
// This is the engine's "OpenGL ES 2.0 era" shader wrapper used by the
// custom shader pipeline (RQShaderBuildSource / InitES2Shader / etc).
//
// Fields used directly by SAShaderL/main.cpp:
//   self->nShaderId   (InitES2Shader hook: glGetUniformLocation target)
//   shader->flags     (FlagsToShaderName / FlagToName: ShaderFlags uniform)
//
// Layout is a best-effort reconstruction consistent with how the engine
// calls _glGetUniformLocation(self->nShaderId, ...) right after program
// link/compile — nShaderId is the GL program object id. `flags` mirrors the
// RQShaderBuildSource flags bitmask (see FLAG_* defines in ES3Shader.h).
// ----------------------------------------------------------------------------
class ES2Shader
{
public:
    int      nShaderId;     // GL program object ID (glCreateProgram result)
    uint32_t flags;         // shader flags bitmask (FLAG_ALPHA_TEST, etc.)
    uint8_t  _pad0[0x40];   // remaining engine-internal shader bookkeeping
                              // (refcount, source pointers, link state, etc.)
};

#endif // _GTASA_STRUCTS_H
