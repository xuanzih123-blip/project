#pragma once
#import <stdint.h>

// ---------------------------------------------------------------------------
// One named block for every offset in the game. Nothing in the project is
// allowed to carry a bare hex literal any more.
//
// Base binary: UnityFramework (arm64, iOS).
// ---------------------------------------------------------------------------

namespace off {

// --- match / session ---
constexpr uint64_t InitBase            = 0xA342EFC;
constexpr uint64_t StaticClass         = 0x5C;
constexpr uint64_t CurrentMatch        = 0x50;
constexpr uint64_t CurrentMatchGame    = 0x4;
constexpr uint64_t MatchStatus         = 0x8C;
constexpr uint64_t LocalPlayer         = 0x94;
constexpr uint64_t DictionaryEntities  = 0x68;
constexpr uint64_t GameTimer           = 0x10;
constexpr uint64_t FixedDeltaTime      = 0x24;

// --- player ---
constexpr uint64_t Player_IsDead       = 0x50;
constexpr uint64_t Player_Name         = 0x31C;
constexpr uint64_t Player_Data         = 0x48;
constexpr uint64_t IsClientBot         = 0x324;
constexpr uint64_t TeamModeID          = 0x294;
constexpr uint64_t Player_ShadowBase   = 0x1A60;
constexpr uint64_t ShadowState         = 0x1A60;
constexpr uint64_t XPose               = 0x78;
constexpr uint64_t PlayerID            = 0x268;
constexpr uint64_t m_PlayerID          = 0x268;

constexpr uint64_t Player_Profile      = 0x18CC;
constexpr uint64_t Player_LevelPtr     = 0x18CC;
constexpr uint64_t Profile_Level       = 0x14;
constexpr uint64_t Profile_Rank        = 0x58;
constexpr uint64_t Profile_RankPoint   = 0x5C;
constexpr uint64_t BaseProfileInfo     = 0x18CC;
constexpr uint64_t LevelUp             = 0x18CC;

constexpr uint64_t Player_IsFemale     = 0x7D8;
constexpr uint64_t Player_Data_PoolObj = 0x10;
constexpr uint64_t Player_Health       = 0x10;

// --- avatar ---
constexpr uint64_t AvatarManager       = 0x504;
constexpr uint64_t Avatar              = 0xA8;
constexpr uint64_t Avatar_IsVisible    = 0x95;
constexpr uint64_t Avatar_Data         = 0x14;
constexpr uint64_t Avatar_Data_IsTeam  = 0x59;

// --- camera ---
constexpr uint64_t MainCameraTransform = 0x28C;
constexpr uint64_t FollowCamera        = 0x494;
constexpr uint64_t Camera_IntPtr       = 0x8;
constexpr uint64_t Camera              = 0x18;
constexpr uint64_t FollowCamera_m_RightOffset = 0x64;
constexpr uint64_t FollowCamera_m_UpOffset    = 0x68;
constexpr uint64_t FollowCamera_m_VisionSpeed = 0x48;
constexpr uint64_t CurrentObserver     = 0xB4;
constexpr uint64_t ObserverPlayer      = 0x28;

// --- aim ---
// The field the weapon traces against. The reference build calls it guna;
// this dump calls the same offset AimbotVisible.
constexpr uint64_t LockedAimingCollider = 0x54;
constexpr uint64_t Collider             = 0x4E8;
constexpr uint64_t AimbotVisible        = 0x4E8;
constexpr uint64_t AimRotation          = 0x440;

// --- bones ---
constexpr uint64_t Head          = 0x49C;
constexpr uint64_t Root          = 0x4B0;
constexpr uint64_t RootBone      = 0x4B0;
constexpr uint64_t Root_Pes      = 0x4B0;
constexpr uint64_t Hip           = 0x4A0;
constexpr uint64_t Spine         = 0x4A4;
constexpr uint64_t Neck          = 0x4A4;
constexpr uint64_t Groin         = 0x4AC;
constexpr uint64_t LeftAnkle     = 0x4B8;
constexpr uint64_t RightAnkle    = 0x4BC;
constexpr uint64_t LeftFoot      = 0x4C0;
constexpr uint64_t RightFoot     = 0x4C4;
constexpr uint64_t LeftToe       = 0x4C0;
constexpr uint64_t RightToe      = 0x4C4;
constexpr uint64_t LeftShoulder  = 0x4D0;
constexpr uint64_t RightShoulder = 0x4D4;
constexpr uint64_t LeftHand      = 0x4C8;
constexpr uint64_t RightHand     = 0x498;
constexpr uint64_t LeftElbow     = 0x4E4;
constexpr uint64_t RightElbow    = 0x4E0;
constexpr uint64_t LeftWrist     = 0x4DC;
constexpr uint64_t RightWrist    = 0x4D8;
constexpr uint64_t LeftWristJoint = 0x4DC;

// --- head collider ---
constexpr uint64_t HeadCollider    = 0x360;
constexpr uint64_t ReplaceCollider = 0x54;

// --- weapon ---
constexpr uint64_t Weapon          = 0x434;
constexpr uint64_t WeaponData      = 0x58;
constexpr uint64_t Weapon_ItemId   = 0x494;
constexpr uint64_t WeaponSkinMap   = 0x18D8;
constexpr uint64_t WeaponRecoil    = 0xC;
constexpr uint64_t UnkPlayerWeaponInfoClass = 0x4A8;
constexpr uint64_t IsCombineWeapon = 0xD8;
constexpr uint64_t WeaponOnHand    = 0x54;
constexpr uint64_t CombineWeaponOnHand = 0x58;
constexpr uint64_t Weapon_Info     = 0x64;
constexpr uint64_t WeaponID        = 0x14;
constexpr uint64_t Weapon_FireDelay = 0x10;
constexpr uint64_t Weapon_IsSighting = 0x5CC;

constexpr uint64_t DobleGunOffs1 = 0x4B4;
constexpr uint64_t DobleGunOffs2 = 0x4D0;

// --- aim chain: local player -> firing flag -> weapon info -> gun tip -> hit ---
constexpr uint64_t sAim1                   = 0x58C;
constexpr uint64_t sAim2                   = 0x9D0;
constexpr uint64_t sAim3                   = 0x38;
constexpr uint64_t sAim4                   = 0x2C;
constexpr uint64_t LocalPlayerIsFiring     = 0x58C;
constexpr uint64_t LastAimingInfoFromWeapon = 0x9D0;
constexpr uint64_t WeaponInfo              = 0x9D0;
constexpr uint64_t GunTipPosition          = 0x38;
constexpr uint64_t BulletHit               = 0x2C;
constexpr uint64_t StartPosition           = 0x38;
constexpr uint64_t RayDir                  = 0x2C;
constexpr uint64_t WeaponInfoOnWeapon      = 0x64;
constexpr uint64_t ViewMatrix              = 0xE8;
constexpr uint64_t isFiring                = 0x58C;

// --- weapon timing ---
constexpr uint64_t Weapon_FireComponent             = 0x58;
constexpr uint64_t Weapon_ChangeWeaponTime          = 0x44;
constexpr uint64_t Weapon_ChangeWeaponAnimSpeed     = 0x48;
constexpr uint64_t Weapon_LastFireTickCount         = 0x444;
constexpr uint64_t Weapon_LastFiringTime            = 0x43C;
constexpr uint64_t Weapon_LastPullBoltTime          = 0x440;
constexpr uint64_t Weapon_NotReadyTimeFromPreWeapon = 0x44;
constexpr uint64_t Weapon_LastStartFireReqTickCount = 0x448;
constexpr uint64_t Weapon_LastSwapSuccessTime       = 0x3E8;
constexpr uint64_t WeaponFireComponent_FireInterval = 0x120;

// --- attributes ---
constexpr uint64_t LocalPlayerAttributes = 0x500;
constexpr uint64_t PlayerAttributes      = 0x500;
constexpr uint64_t NoReload              = 0xC1;
constexpr uint64_t UnlimitedAmmo         = 0xE0;
constexpr uint64_t RapidfirePtr          = 0x1C8;
constexpr uint64_t FireInterval          = 0x1C8;
constexpr uint64_t FireIntervalScaleTwo  = 0x1C8;
constexpr uint64_t FallingSpeedUpScale   = 0x1D4;
constexpr uint64_t RunSpeedUpScale       = 0x1D8;
constexpr uint64_t WalkDownSpeedScale    = 0x1DC;

constexpr uint64_t Player_SecondaryWeapon   = 0x3F8;
constexpr uint64_t Player_WeaponFireComponent = 0x8FC;

// --- world / loot ---
constexpr uint64_t LevelObjectManager    = 0x60;
constexpr uint64_t LevelObjectManagerAlt = 0x64;
constexpr uint64_t LootTransformComponent = 0x3C;
constexpr uint64_t LootPickupList        = 0xDC;
constexpr uint64_t LootPickupListAlt     = 0xC4;

constexpr uint64_t InSnowSlideWayDashing = 0x15E8;
constexpr uint64_t isBotOffs             = 0x1B0;

// --- silent fire ---
constexpr uint64_t SilentFiring = 0x6C0;
constexpr uint64_t SilentHit    = 0x7D0;
constexpr uint64_t Silents      = 0x38;
constexpr uint64_t Silentr      = 0x2C;

} // namespace off
