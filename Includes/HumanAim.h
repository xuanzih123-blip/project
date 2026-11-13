#pragma once
#import "ESP.h"
#import <stdlib.h>
#import <math.h>

// ---------------------------------------------------------------------------
// Humanisation layer.
//
// A bot that locks the crosshair exactly on the head pixel is more obvious
// than one that flicks wildly. What a server can measure without hooking the
// client is the shape of the aim curve and the timing around it:
//
//   reaction delay   a human needs 150-280 ms from acquisition to the first
//                    correction. A bot that reacts in one frame is a bot.
//   velocity profile human point-to-point movement follows a minimum-jerk
//                    bell: slow start, peak in the middle, settle at the end.
//                    Constant velocity (a slerp with a fixed factor) is not
//                    something a hand produces.
//   overshoot        fast flicks land 4-12% past the target and come back.
//                    A bot that arrives perfectly every time is a bot.
//   tremor           a hand at rest wobbles at 1-15 Hz with a correlation
//                    time of ~100 ms. White noise per frame reads as static,
//                    not as a hand.
//   lead             humans lead a moving target by 50-120 ms.
//
// The trajectory is built here, the target selection stays in Aimbot.h.
// ---------------------------------------------------------------------------

namespace human {

static const float kRad2Deg = 57.29577951308232f;
static const float kDeg2Rad = 0.017453292519943295f;
static const float kTau     = 6.28318530717958648f;

enum Phase {
    PhaseIdle    = 0,
    PhaseAcquire = 1,   // waiting out the reaction time
    PhaseFlick   = 2,   // accelerating toward the target
    PhaseCorrect = 3,   // pulling back from the overshoot
    PhaseHold    = 4,   // tracking, with tremor
};

struct State {
    int       phase;
    void     *target;
    double    phaseStart;
    double    reactionUntil;
    double    flickDuration;
    double    correctDuration;
    float     overshoot;
    Quaternion current;      // last rotation written to the game
    Quaternion from;         // rotation the current flick started from
    Quaternion to;           // where the aim wants to be right now
    Quaternion overshot;     // rotation at the peak of the overshoot

    // tremor oscillators: phase, frequency, amplitude
    float     oscPhase[4];
    float     oscFreq[4];
    float     oscAmp[4];

    // slow aim bias, re-rolled every 150-400 ms
    float     biasX;
    float     biasY;
    double    biasUntil;

    // target velocity for the lead
    Vector3   lastTarget;
    double    lastTargetAt;
    Vector3   targetVel;

    double    lastTick;
    bool      seeded;
};

static State g_state;

// Drift, slow body, fast, and the physiological tremor band.
static const float kOscFreq[4] = { 0.45f, 2.20f, 7.50f, 13.00f };
static const float kOscAmp[4]  = { 1.00f, 0.55f, 0.20f, 0.08f };

static inline float Rand01(void) {
    return (float)arc4random_uniform(1000000) / 1000000.0f;
}

static inline double Now(void) {
    return CACurrentMediaTime() * 1000.0;
}

static inline void Reset(void) {
    g_state.phase     = PhaseIdle;
    g_state.target    = nullptr;
    g_state.lastTick  = 0.0;
    g_state.biasUntil = 0.0;
}

// Minimum jerk: the velocity profile of a human point-to-point movement.
static inline float MinJerk(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * t * (10.0f + t * (-15.0f + 6.0f * t));
}

static inline void SeedOscillators(void) {
    for (int i = 0; i < 4; i++) {
        g_state.oscPhase[i] = Rand01() * kTau;
        g_state.oscFreq[i]  = kOscFreq[i] * (0.85f + 0.30f * Rand01());
        g_state.oscAmp[i]   = kOscAmp[i];
    }
    g_state.seeded = true;
}

// Frame delta, clamped so a stall does not teleport the tremor phase.
static inline double AdvanceClock(double now) {
    double dt = (g_state.lastTick > 0.0) ? (now - g_state.lastTick) / 1000.0 : (1.0 / 60.0);
    if (dt < 0.0 || dt > 0.25) dt = 1.0 / 60.0;
    g_state.lastTick = now;
    return dt;
}

// Band-limited wobble in the camera plane, in degrees.
static inline void TremorOffset(double dt, float &dxDeg, float &dyDeg) {
    if (!g_state.seeded) SeedOscillators();
    for (int i = 0; i < 4; i++) {
        g_state.oscPhase[i] += (float)(kTau * g_state.oscFreq[i] * dt);
        if (g_state.oscPhase[i] > kTau) g_state.oscPhase[i] -= kTau;
    }
    float amp = Vars.AimTremorDeg;
    dxDeg = 0.0f;
    dyDeg = 0.0f;
    for (int i = 0; i < 4; i++) {
        float a = amp * g_state.oscAmp[i] * sinf(g_state.oscPhase[i]);
        if (i & 1) dyDeg += a; else dxDeg += a;
    }
}

// Where the human is actually pointing: a slowly re-rolled offset around the
// aim point, so the error distribution is a disc instead of a perfect pixel.
static inline void BiasOffset(double now, float &dxDeg, float &dyDeg) {
    if (now >= g_state.biasUntil) {
        float r = Vars.AimErrorDeg;
        g_state.biasX = (Rand01() * 2.0f - 1.0f) * r;
        g_state.biasY = (Rand01() * 2.0f - 1.0f) * r;
        g_state.biasUntil = now + 150.0 + 250.0 * Rand01();
    }
    dxDeg += g_state.biasX;
    dyDeg += g_state.biasY;
}

// Smoothed target velocity, used for the lead.
static inline Vector3 TargetVelocity(Vector3 pos, double now, double dt) {
    Vector3 vel = g_state.targetVel;
    if (g_state.lastTargetAt > 0.0 && dt > 0.0001) {
        Vector3 instant = (pos - g_state.lastTarget) / (float)dt;
        vel = g_state.targetVel + (instant - g_state.targetVel) * 0.35f;   // light EMA
    }
    g_state.lastTarget    = pos;
    g_state.lastTargetAt  = now;
    g_state.targetVel     = vel;
    return vel;
}

// Flick duration grows with the angle, then gets a personal jitter.
static inline double FlickDuration(float angleDeg) {
    if (angleDeg < 1.0f) angleDeg = 1.0f;
    double base = (double)Vars.AimFlickBaseMs / 1000.0 + 0.022 * pow((double)angleDeg, 0.55);
    double t = base * (0.85 + 0.30 * Rand01());
    if (t < 0.05) t = 0.05;
    if (t > 0.55) t = 0.55;
    return t;
}

// Build the rotation for this tick.
// Returns false while the reaction window is still open, which means: do not
// move the aim yet.
static inline bool Step(void *target, Quaternion desired,
                        float angleDeg, double now, Quaternion &out) {
    State &s = g_state;
    if (!s.seeded) SeedOscillators();

    if (s.target != target) {
        s.target = target;
        s.phase = PhaseAcquire;
        s.reactionUntil = now + (Vars.AimReactionMs / 1000.0) * (0.75 + 0.50 * Rand01());
        s.from = s.current;   // the aim has not moved yet, the flick starts from here
    }

    s.to = desired;

    if (s.phase == PhaseAcquire) {
        if (now < s.reactionUntil) return false;
        s.phase         = PhaseFlick;
        s.phaseStart    = now;
        s.flickDuration = FlickDuration(angleDeg);
        s.overshoot     = 1.0f + (Vars.AimOvershootPct / 100.0f) * (0.5f + Rand01());
        s.from          = s.current;
        // falls through on purpose: the flick starts on this very tick
    }

    if (s.phase == PhaseFlick) {
        float tau = (float)((now - s.phaseStart) / s.flickDuration);
        float p   = MinJerk(tau) * s.overshoot;
        out = Quaternion::SlerpUnclamped(s.from, s.to, p);
        if (tau >= 1.0f) {
            s.overshot        = out;
            s.phase           = PhaseCorrect;
            s.phaseStart      = now;
            s.correctDuration = (Vars.AimCorrectionMs / 1000.0) * (0.70 + 0.60 * Rand01());
        }
        return true;
    }

    if (s.phase == PhaseCorrect) {
        float tau = (float)((now - s.phaseStart) / s.correctDuration);
        out = Quaternion::Slerp(s.overshot, s.to, MinJerk(tau));
        if (tau >= 1.0f) s.phase = PhaseHold;
        return true;
    }

    // PhaseHold, and the default when nothing was acquired
    out = s.to;
    return true;
}

} // namespace human
