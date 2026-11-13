#pragma once
#import "ESP.h"
#import "HumanAim.h"
#import <stdlib.h>
#import <float.h>
#import <math.h>

// ---------------------------------------------------------------------------
// Port of the reference aimbot method (external PC build) onto the in-process
// Unity SDK.
//
// Reference pipeline, preserved step for step:
//   trigger held
//     -> camera transform from the local player
//     -> walk the entity list
//     -> filter: dead / knocked / visible / same team
//     -> head bone -> world position
//     -> distance gate
//     -> project to screen, reject off-screen
//     -> pick the entity closest to the crosshair
//     -> rotation to that head point
//     -> write the aim rotation
//     -> sleep(RageAimbotSmooth)
//
// Two primitives do not travel to iOS and are mapped onto what the game
// already exposes:
//   GetAsyncKeyState(VK_LBUTTON) -> game_sdk->get_IsFiring(local)
//   sleep_for(ms)                -> write throttle + rotation slerp
// The blocking sleep is the important one: sleeping the main thread freezes
// the whole game, so the rate limit becomes a timestamp gate and the
// "smoothness" becomes a slerp with a per-write degree clamp.
// ---------------------------------------------------------------------------

namespace aim {

static const float kRad2Deg = 57.29577951308232f;

// FILL: XPose offset on the shadow base. The reference build reads
// shadowBase + XPose and treats the value 8 as "knocked down".
// 0 disables the knocked filter, the dead check still applies.
static constexpr uint64_t kShadowBaseOffset = 0;      // FILL
static constexpr uint64_t kXPoseOffset      = 0;      // FILL

// FILL: aim rotation field on the local player (reference: Offsets::AimRotation).
// 0 = call the game's set_aim() instead of writing the field directly.
static constexpr uint64_t kAimRotationOffset = 0;     // FILL

// Physics layer mask for the line-of-sight ray (same value the project already used).
static constexpr unsigned int kRayLayerMask = 12;

// ---------------------------------------------------------------------------
// small in-process readers
// ---------------------------------------------------------------------------

static inline uint32_t ReadU32(uint64_t base, uint64_t offset) {
    if (!base) return 0;
    return *(uint32_t *)(base + offset);
}

// ---------------------------------------------------------------------------
// cached player list getter, so the dyld scan runs once per process
// ---------------------------------------------------------------------------

static void *(*g_playerListFn)(void *) = nullptr;

static inline void *(*PlayerListFn(void))(void) {
    if (!g_playerListFn) {
        void *addr = (void *)getRealOffset(0x4C869DC);
        if (!addr) return nullptr;
        g_playerListFn = (void *(*)(void *))addr;
    }
    return g_playerListFn;
}

// ---------------------------------------------------------------------------
// state carried between ticks
// ---------------------------------------------------------------------------

static Quaternion g_lastRotation   = Quaternion();
static bool       g_hasLastRotation = false;
static double    g_lastWriteMs     = 0.0;

static inline double NowMs() { return CACurrentMediaTime() * 1000.0; }

// ---------------------------------------------------------------------------
// entity predicates
// ---------------------------------------------------------------------------

static inline bool IsDead(void *entity) {
    return game_sdk->get_IsDieing ? game_sdk->get_IsDieing(entity) : false;
}

// Knocked (alive but on the floor). Needs the shadow-base offset; without it
// the filter is a no-op and knocked players stay targetable.
static inline bool IsKnocked(void *entity) {
    if (kShadowBaseOffset == 0 || kXPoseOffset == 0) return false;
    uint32_t shadowBase = ReadU32((uint64_t)entity, kShadowBaseOffset);
    if (!shadowBase) return false;
    return ReadU32(shadowBase, kXPoseOffset) == 8;
}

static inline bool IsVisible(void *entity) {
    return game_sdk->get_isVisible ? game_sdk->get_isVisible(entity) : true;
}

static inline bool IsSameTeam(void *entity) {
    return game_sdk->get_isLocalTeam ? game_sdk->get_isLocalTeam(entity) : false;
}

// ---------------------------------------------------------------------------
// camera / trigger
// ---------------------------------------------------------------------------

static inline bool CameraPosition(void *local, Vector3 &out) {
    if (!local || !game_sdk->get_position) return false;
    void *camTF = *(void **)((uint64_t)local + 0x390);
    if (!camTF) return false;
    out = game_sdk->get_position(camTF);
    return !(out.x == 0.0f && out.y == 0.0f && out.z == 0.0f);
}

// Reference: GetAsyncKeyState(VK_LBUTTON). The game already tracks the trigger.
static inline bool TriggerHeld(void *local) {
    return game_sdk->get_IsFiring ? game_sdk->get_IsFiring(local) : false;
}

static inline bool Scoped(void *local) {
    return game_sdk->get_IsSighting ? game_sdk->get_IsSighting(local) : false;
}

// ---------------------------------------------------------------------------
// head bone -> world position
// The head offset yields a node, not a transform: node -> transform -> position.
// ---------------------------------------------------------------------------

static inline bool HeadWorldPosition(void *entity, Vector3 &out) {
    if (!entity || !game_sdk->GetHeadPositions) return false;
    if (!game_sdk->Component_GetTransform || !game_sdk->get_position) return false;
    void *node = game_sdk->GetHeadPositions(entity);
    if (!node) return false;
    void *tf = game_sdk->Component_GetTransform(node);
    if (!tf) return false;
    out = game_sdk->get_position(tf);
    return !(out.x == 0.0f && out.y == 0.0f && out.z == 0.0f);
}

// ---------------------------------------------------------------------------
// line of sight: camera -> the exact point the aim goes to
// ---------------------------------------------------------------------------

static inline bool LineOfSight(Vector3 camPos, Vector3 aimPoint) {
    void *hit = nullptr;
    return !Physics_Raycast(camPos, aimPoint, kRayLayerMask, &hit);
}

// ---------------------------------------------------------------------------
// rotation
// ---------------------------------------------------------------------------

static inline Quaternion RotationToLocation(Vector3 target, float heightOffset, Vector3 camPos) {
    Vector3 dir = (target + Vector3(0.0f, heightOffset, 0.0f)) - camPos;
    if (Vector3::Magnitude(dir) < 1e-4f) return Quaternion();
    return Quaternion::LookRotation(dir, Vector3(0, 1, 0));
}

// ---------------------------------------------------------------------------
// target search
// ---------------------------------------------------------------------------

struct Target {
    void   *entity;
    Vector3 headPos;
    float   crosshairDist;
    float   distance;
};

static inline bool FindTarget(void *match, void *local, Target &best) {
    void *(*listFn)(void *) = PlayerListFn();
    if (!listFn) return false;

    monoList<void **> *players = listFn(match);
    if (!players || !players->getItems()) return false;

    Vector3 cameraPosition;
    if (!CameraPosition(local, cameraPosition)) return false;

    // The crosshair centre has to live in the same space the projection returns,
    // otherwise "closest to the crosshair" measures the wrong distance.
    ScreenMetrics screen = GetScreenMetrics();
    float cx = screen.width  * 0.5f;
    float cy = screen.height * 0.5f;

    Vector3 forward;
    bool haveForward = game_sdk->GetForward ? true : false;
    if (haveForward) {
        forward = game_sdk->GetForward(local);
        haveForward = Vector3::Magnitude(forward) > 1e-4f;
    }

    best.entity        = nullptr;
    best.headPos       = Vector3();
    best.crosshairDist = FLT_MAX;
    best.distance      = 0.0f;

    int count = players->getSize();
    if (count > 256) count = 256;                       // bound the walk

    for (int j = 0; j < count; j++) {
        void *entity = players->getItems()[j];
        if (!entity || entity == local) continue;

        if (IsDead(entity)) continue;
        if (Vars.IgnoreKnocked && IsKnocked(entity)) continue;
        if (!IsVisible(entity)) continue;
        if (IsSameTeam(entity)) continue;

        Vector3 headWorldPos;
        if (!HeadWorldPosition(entity, headWorldPos)) continue;

        // distance gate (reference: RageAimbotDistance)
        float playerDistance = Vector3::Distance(cameraPosition, headWorldPos);
        if (playerDistance > Vars.AimRange || playerDistance < 1.0f) continue;

        // optional angular cap
        if (Vars.AimMaxAngleDeg > 0.0f && haveForward) {
            Vector3 to = headWorldPos - cameraPosition;
            float angle = Vector3::Angle(forward, to) * kRad2Deg;
            if (angle > Vars.AimMaxAngleDeg) continue;
        }

        // screen rejection
        SimpleVec2 headScreen = Camera$$WorldToScreen::Regular(headWorldPos);
        if (headScreen.x < 1.0f || headScreen.y < 1.0f) continue;

        // closest to the crosshair
        float dx = headScreen.x - cx;
        float dy = headScreen.y - cy;
        float crosshairDist = sqrtf(dx * dx + dy * dy);

        if (crosshairDist < best.crosshairDist) {
            best.crosshairDist = crosshairDist;
            best.entity        = entity;
            best.headPos       = headWorldPos;
            best.distance      = playerDistance;
        }
    }

    return best.entity != nullptr;
}

// ---------------------------------------------------------------------------
// tick
// ---------------------------------------------------------------------------

static inline void Tick(void *match, void *local) {
    if (!Vars.Aimbot || !local) return;

    bool trigger = TriggerHeld(local);
    bool scoped  = Scoped(local);

    bool armed = (Vars.AimWhen == 0)
               || (Vars.AimWhen == 1 && trigger)
               || (Vars.AimWhen == 2 && scoped)
               || (Vars.AimWhen == 3 && (trigger || scoped));
    if (!armed) {
        g_hasLastRotation = false;
        human::Reset();
        return;
    }

    Target best;
    if (!FindTarget(match, local, best)) {
        g_hasLastRotation = false;
        human::Reset();
        return;
    }

    Vector3 cameraPosition;
    if (!CameraPosition(local, cameraPosition)) return;

    // line of sight against the exact point we aim at
    if (Vars.VisibleCheck && !LineOfSight(cameraPosition, best.headPos)) {
        human::Reset();
        return;
    }

    double now = human::Now();
    double dt  = human::AdvanceClock(now);

    // camera basis, used to turn an angular error into a world offset
    Vector3 forward = game_sdk->GetForward ? game_sdk->GetForward(local) : Vector3(0, 0, 1);
    if (Vector3::Magnitude(forward) < 1e-4f) forward = Vector3(0, 0, 1);
    Vector3 right = Vector3::Normalized(Vector3::Cross(Vector3(0, 1, 0), forward));
    if (Vector3::Magnitude(right) < 1e-4f) right = Vector3(1, 0, 0);
    Vector3 up = Vector3::Cross(forward, right);

    Vector3 aimPoint = best.headPos;

    // lead the target: humans aim where the target will be, not where it is
    if (Vars.AimLeadMs > 0.0f) {
        Vector3 vel = human::TargetVelocity(best.headPos, now, dt);
        aimPoint = aimPoint + vel * (Vars.AimLeadMs / 1000.0f);
    }

    // tremor (band limited) + slow bias (where the human is actually pointing)
    if (Vars.HumanAim) {
        float dxDeg = 0.0f, dyDeg = 0.0f;
        human::TremorOffset(dt, dxDeg, dyDeg);
        human::BiasOffset(now, dxDeg, dyDeg);
        float perMetre = (float)(best.distance * human::kDeg2Rad);
        aimPoint = aimPoint + right * (dxDeg * perMetre) + up * (dyDeg * perMetre);
    }

    Quaternion desired = RotationToLocation(aimPoint, Vars.AimHeightOffset, cameraPosition);

    Quaternion next;
    if (Vars.HumanAim) {
        float angleDeg = Quaternion::Angle(human::g_state.current, desired) * kRad2Deg;
        if (!human::Step(best.entity, desired, angleDeg, now, next)) {
            return;   // still inside the reaction window: hold the aim where it is
        }
    } else {
        next = desired;
    }

    // rate limit (reference: sleep_for(RageAimbotSmooth))
    if (now - g_lastWriteMs < Vars.AimIntervalMs) return;
    g_lastWriteMs = now;

    g_lastRotation    = next;
    g_hasLastRotation = true;
    human::g_state.current = next;

    if (kAimRotationOffset != 0) {
        *(Quaternion *)((uint64_t)local + kAimRotationOffset) = next;
    } else if (game_sdk->set_aim) {
        game_sdk->set_aim(local, next);
    }
}

} // namespace aim
