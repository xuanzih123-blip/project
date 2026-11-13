#pragma once
#import "ESP.h"

// ---------------------------------------------------------------------------
// Offset backed game modifications.
//
// Every one of these writes into a field of a live game object. The game
// rewrites those fields every frame, so the cheat has to rewrite them every
// frame too, or the effect switches itself off. Every pointer chase is
// guarded: a bad read is a crash, and a crash on the wrong build is worse
// than the feature being off.
//
// Object layout used here:
//   local player            +0x500  attributes
//   local player            +0x434  weapon
//   weapon                  +0x58   weapon data / fire component
//   weapon data             +0x0C   recoil
//   weapon data             +0x10   fire delay
//   weapon data             +0x44   change weapon time / not ready from pre weapon
//   weapon data             +0x120  fire interval
//   attributes              +0xC1   no reload flag
//   attributes              +0xE0   unlimited ammo flag
// ---------------------------------------------------------------------------

namespace mods {

static inline void *Chase(void *base, uint64_t offset) {
    if (!base) return nullptr;
    return *(void **)((uint64_t)base + offset);
}

static inline bool WriteU8(void *base, uint64_t offset, uint8_t value) {
    if (!base) return false;
    *(uint8_t *)((uint64_t)base + offset) = value;
    return true;
}

static inline bool WriteF32(void *base, uint64_t offset, float value) {
    if (!base) return false;
    *(float *)((uint64_t)base + offset) = value;
    return true;
}

static inline void *LocalAttributes(void *local) {
    return Chase(local, off::LocalPlayerAttributes);   // 0x500
}

static inline void *LocalWeaponData(void *local) {
    void *weapon = Chase(local, off::Weapon);           // 0x434
    if (!weapon) return nullptr;
    return Chase(weapon, off::WeaponData);              // 0x58
}

static inline void Apply(void *local) {
    if (!local) return;

    bool wantWeapon = Vars.NoRecoil || Vars.FastSwap || Vars.FastReload || Vars.FastFire;
    bool wantAttrs  = Vars.NoReload || Vars.UnlimitedAmmo;

    void *weaponData = wantWeapon ? LocalWeaponData(local) : nullptr;
    void *attrs      = wantAttrs  ? LocalAttributes(local)  : nullptr;

    if (weaponData) {
        if (Vars.NoRecoil)  WriteF32(weaponData, off::WeaponRecoil, 0.0f);   // 0x0C
        if (Vars.FastFire)  WriteF32(weaponData, off::Weapon_FireDelay, 0.0f); // 0x10
        if (Vars.FastSwap)  WriteF32(weaponData, off::Weapon_ChangeWeaponTime, 0.0f);        // 0x44
        if (Vars.FastReload) WriteF32(weaponData, off::Weapon_NotReadyTimeFromPreWeapon, 0.0f); // 0x44
    }

    if (attrs) {
        if (Vars.NoReload)      WriteU8(attrs, off::NoReload, 1);       // 0xC1
        if (Vars.UnlimitedAmmo) WriteU8(attrs, off::UnlimitedAmmo, 1);  // 0xE0
    }
}

} // namespace mods
