// ============================================================================
// GTASA_STRUCTS_210.h
// ----------------------------------------------------------------------------
// Minimal compatible struct header for GTA:SA Android libGTASA.so v2.10
// (64-bit / arm64-v8a build, AML64 path — the #else branch in ES3Shader.h).
//
// IMPORTANT:
// Same disclaimer as GTASA_STRUCTS.h: this is NOT the original/official
// header used internally by the AndroidModLoader team. It's a hand-built,
// minimal-compatible shim containing ONLY the symbols SAShaderL actually
// references, so the 64-bit (AArch64) build target can compile.
//
// Differences vs the 32-bit version (GTASA_STRUCTS.h):
//  - All pointers are 8 bytes (native arm64), so padding/offsets after any
//    pointer field shift accordingly.
//  - Bitfields packed into a 32-bit word keep the same bit layout as 32-bit
//    (RW/GTA:SA didn't widen these on the 64-bit port), but the alignment
//    padding around them can differ due to 8-byte pointer alignment rules.
//
// As with the 32-bit shim: if you obtain the real GTASA_STRUCTS_210.h,
// replace this file entirely. The offsets/padding here are placeholders
// sized to "compile and be roughly position-correct for the fields
// SAShaderL touches" — they have NOT been independently verified against
// the 64-bit v2.10 binary the way your CFont::RenderString / CCamera::Process
// function offsets have been. Re-validate m_pRwCamera / farClip / m_nType /
// m_nModelIndex offsets with radare2/Ghidra on the arm64 .so before trusting
// them for anything beyond "it compiles".
// ============================================================================

#ifndef _GTASA_STRUCTS_210_H
#define _GTASA_STRUCTS_210_H

#include <stdint.h>

// ----------------------------------------------------------------------------
// Basic math types (RenderWare / GTA:SA convention) — identical to 32-bit,
// these are plain floats with no pointer-size dependency.
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
// RwCamera, 64-bit layout.
//
// CONFIRMED via radare2 disassembly (arm64, v2.10) of two independent
// functions, cross-checked against each other:
//
// (1) CCamera::SetRwCamera(RwCamera*):
//       str x1, [x19, 0x930]   ; this->m_pRwCamera = pRwCamera
//       add x1, x1, 0x40       ; pRwCamera + 0x40 -> RwMatrixTag* used by
//                              ; CMatrix::Attach(RwMatrixTag*, bool)
//     => RwCamera+0x40 leads into the camera's frame/matrix block.
//
// (2) CCamera::Process(), right after calling RwCameraSetFarClipPlane:
//       bl   RwCameraSetFarClipPlane(RwCamera*, float)
//       ldr  x8, [x19, 0x930]        ; reload this->m_pRwCamera (CONFIRMS
//                                    ; the 0x930 offset again, independently)
//       ldp  w9, w8, [x8, 0xa8]      ; load TWO adjacent 32-bit floats from
//                                    ; RwCamera+0xa8 and RwCamera+0xac
//       str  w9, [x10]               ; -> CDraw::ms_fNearClipZ
//       str  w8, [x9]                ; -> CDraw::ms_fFarClipZ
//
//     Since these are read immediately after RwCameraSetFarClipPlane/
//     RwCameraSetNearClipPlane calls earlier in the same function, and an
//     `ldp` always loads two CONSECUTIVE 32-bit slots, this gives us exact,
//     directly-observed offsets:
//
//         RwCamera::nearClip  @ offset 0xa8  <-- CONFIRMED
//         RwCamera::farClip   @ offset 0xac  <-- CONFIRMED
//
// This is exactly what TheCamera->m_pRwCamera->farClip in
// SAShaderL/main.cpp (FarClipDist uniform) resolves to.
// ----------------------------------------------------------------------------
struct RwCamera
{
    uint8_t  _pad0[0xa8];     // CONFIRMED size: RwObject header, frame/matrix
                               // block (matrix tag at +0x40), viewport state,
                               // everything up to nearClip — none of these
                               // intermediate fields are touched by SAShaderL
    float    nearClip;        // CONFIRMED offset 0xa8
    float    farClip;         // CONFIRMED offset 0xac
    uint8_t  _pad1[0x50];     // remaining camera fields (placeholder size,
                               // not needed by SAShaderL)
};

// ----------------------------------------------------------------------------
// CCamera — only m_pRwCamera is consumed by SAShaderL.
//
// CONFIRMED via radare2 disassembly (arm64, v2.10) of
// CCamera::SetRwCamera(RwCamera*):
//
//   mov x19, x0            ; x19 = this
//   str x1, [x19, 0x930]   ; this->m_pRwCamera = pRwCamera   <-- CONFIRMED
//   ...
//   ldr x0, [x19, 0x930]   ; reload for return value
//
// So m_pRwCamera sits at offset 0x930 in CCamera on this binary. CCamera is
// a genuinely huge class in GTA:SA (camera modes, shake state, splines,
// cutscene data, etc.) so this large offset is expected and not a sign of
// a wrong layout — it matches what TheCamera->m_pRwCamera->farClip in
// SAShaderL/main.cpp needs to resolve to.
// ----------------------------------------------------------------------------
class CCamera
{
public:
    uint8_t   _pad0[0x930];  // CONFIRMED size: everything before m_pRwCamera
                               // (camera modes, shake, splines, etc. — opaque
                               //  padding, none of these fields are touched
                               //  by SAShaderL)
    RwCamera* m_pRwCamera;     // CONFIRMED offset 0x930 (8-byte pointer)
    uint8_t   _pad1[0x100];   // remainder of CCamera after m_pRwCamera —
                               // size unverified, generous placeholder
};

// ----------------------------------------------------------------------------
// CEntity — only the type & model index fields are needed.
// vtable + m_rwObject are now 8 bytes each (was 4+4 on 32-bit), which shifts
// the bitfield word and everything after it by +8 bytes relative to 32-bit.
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
    void*    vtable;          // 0x00 vtable ptr (8 bytes on arm64)
    void*    m_rwObject;      // 0x08 RpAtomic/RpClump* (8 bytes on arm64)

    // --- bitfield word: contains m_nType among others (same bit layout
    //     as 32-bit; just located 4 bytes later due to pointer widening) ---
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

    uint8_t  _pad0[0x18];      // matrix handle / reference / lod fields...
                                // (wider padding to keep model index roughly
                                //  aligned with the real 64-bit layout)

    int16_t  m_nModelIndex;    // model index (-1 if none)
    int16_t  _pad1;

    uint8_t  _pad2[0x48];      // remainder of CEntity (PADDING ONLY)
};

// ----------------------------------------------------------------------------
// ES2Shader — base class of ES3Shader (see ES3Shader.h in this repo).
// Same role as the 32-bit version; padding widened slightly to account for
// any internal pointer fields being 8 bytes on arm64.
// ----------------------------------------------------------------------------
class ES2Shader
{
public:
    int      nShaderId;     // GL program object ID (glCreateProgram result)
    uint32_t flags;         // shader flags bitmask (FLAG_ALPHA_TEST, etc.)
    uint8_t  _pad0[0x48];   // remaining engine-internal shader bookkeeping
};

#endif // _GTASA_STRUCTS_210_H
