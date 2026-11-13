#pragma once
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <substrate.h>

#import "Offsets.h"
#import "UnityTypes.h"
#import "Vector3.h"
#import "Vector2.h"
#import "Quaternion.h"
#import "MemoryUtils.h"


struct SimpleVec2 {
    float x, y;
    SimpleVec2() : x(0), y(0) {}
    SimpleVec2(float x, float y) : x(x), y(y) {}
};


#define kUnifiedRedColor [UIColor redColor].CGColor


struct Vars_t
{
    bool Enable = true;
    bool lines = false;
    bool Box = false;
    bool Name = false;
    bool Health = false;
    bool Distance = false;
    bool skeleton = false;
    bool NoFog = true;

    // Mods, offset backed (Includes/GameMods.h)
    bool NoRecoil      = false;
    bool FastSwap      = false;
    bool FastReload    = false;
    bool FastFire      = false;
    bool NoReload      = false;
    bool UnlimitedAmmo = false;
    bool Force120FPS   = false;

    // Aimbot
    bool Aimbot = false;
    bool ShowFovCircle = true; 
    bool VisibleCheck = true;  
    bool IgnoreKnocked = true;  
    int AimWhen = 3;       

    // Aim Visible mode (reference build: AimbotAi)
    bool  AimVisible      = false;   // select by pixel FOV, aim at the head collider
    float AimFov          = 150.0f;  // pixel radius around the crosshair
    bool  AimWriteField   = false;   // local weapon traces the enemy head collider
    bool  AimSelfTarget   = false;   // the target is pointed at its own head collider

    // Aimbot - ported from the reference method
    float AimHeightOffset   = 0.10f;   // reference: GetRotationToLocation(..., 0.1f, cam)
    float AimIntervalMs     = 16.0f;   // reference: sleep_for(RageAimbotSmooth)
    float AimRange          = 200.0f;  // reference: RageAimbotDistance
    float AimMaxAngleDeg    = 0.0f;    // 0 = no angular cap

    // Humanisation
    bool  HumanAim          = true;    // minimum-jerk trajectory instead of a snap
    float AimReactionMs     = 210.0f;  // acquisition -> first correction
    float AimFlickBaseMs    = 90.0f;   // base flick duration
    float AimOvershootPct   = 8.0f;    // overshoot, percent of the angle
    float AimCorrectionMs   = 95.0f;   // pull back from the overshoot
    float AimTremorDeg      = 0.30f;   // steady state wobble, degrees
    float AimErrorDeg       = 0.35f;   // slow aim bias, degrees
    float AimLeadMs         = 70.0f;   // lead on a moving target
} Vars;


class game_sdk_t
{
public:
    void init();
    void *(*Curent_Match)();
    void *(*GetLocalPlayer)(void *Game);
    Vector3 (*get_position)(void *player);
    void *(*Component_GetTransform)(void *player);
    void *(*get_camera)();
    Vector3 (*WorldToScreenPoint)(void *, Vector3);
    Vector3 (*WorldToScreenPointEx)(void *, Vector3, int);
    Vector3 (*GetForward)(void *player);
    bool (*get_isLocalTeam)(void *player);
    bool (*get_IsDieing)(void *player);
    bool (*get_isVisible)(void *player);
    bool (*get_IsSighting)(void *player);
    bool (*get_IsFiring)(void *player);
    int (*get_MaxHP)(void *player);
    int (*GetHp)(void *player);
    void (*set_aim)(void *, Quaternion);
    monoString *(*name)(void *player);
    void *(*GetHeadPositions)(void *player);

    void *(*_GetHeadPositions)(void *);
    void *(*_newHipMods)(void *);
    void *(*_GetLeftAnkleTF)(void *);
    void *(*_GetRightAnkleTF)(void *);
    void *(*_GetLeftToeTF)(void *);
    void *(*_GetRightToeTF)(void *);
    void *(*_getLeftHandTF)(void *);
    void *(*_getRightHandTF)(void *);
    void *(*_getLeftForeArmTF)(void *);
    void *(*_getRightForeArmTF)(void *);
};

extern game_sdk_t *game_sdk;


// Screen space used by every projection and by the crosshair centre.
// nativeBounds never rotates with the device, so it is normalised into
// landscape points here. Both sides of the "closest to the crosshair" test
// have to come from this one function, otherwise the test measures the wrong
// distance in portrait.
struct ScreenMetrics { float width; float height; };

inline ScreenMetrics GetScreenMetrics() {
    CGRect b = [UIScreen mainScreen].nativeBounds;
    CGFloat scale = [UIScreen mainScreen].nativeScale;
    float w = (float)(b.size.width  / scale);
    float h = (float)(b.size.height / scale);
    if (w < h) { float t = w; w = h; h = t; }
    return ScreenMetrics{w, h};
}


namespace Camera$$WorldToScreen
{
    inline SimpleVec2 Regular(Vector3 pos)
    {
        auto cam = game_sdk->get_camera();
        if (!cam) return SimpleVec2(0, 0);
        Vector3 worldPoint = game_sdk->WorldToScreenPoint(cam, pos);
        
        if (worldPoint.z < 0.01f) return SimpleVec2(0, 0);

        ScreenMetrics m = GetScreenMetrics();

        float lx = m.width  * worldPoint.x;
        float ly = m.height * (1.0f - worldPoint.y);
        
        return SimpleVec2(lx, ly);
    }
}


inline Vector3 getPosition(void *transform)
{
    if (!transform) return Vector3();
    return game_sdk->get_position(game_sdk->Component_GetTransform(transform));
}

inline Vector3 GetBonePosition(void *player, void *(*transformGetter)(void *)) {
    if (!player || !transformGetter) return Vector3();
    void *transform = transformGetter(player);
    if (!transform) return Vector3();
    void *tf = game_sdk->Component_GetTransform(transform);
    return tf ? game_sdk->get_position(tf) : Vector3();
}


@interface ESPRenderer : NSObject
@property (nonatomic, strong) CAShapeLayer *espLayer;
@property (nonatomic, strong) UIView *containerView;
@property (nonatomic, strong) NSMutableArray<CATextLayer *> *textPool;
@property (nonatomic, strong) NSMutableArray<CALayer *> *healthPool;
@property (nonatomic, assign) int textUsedCount;
@property (nonatomic, assign) int healthUsedCount;

+ (instancetype)sharedInstance;
- (void)renderOnView:(UIView *)view;
- (void)clearDrawings;
- (void)drawBoxFrom:(SimpleVec2)min to:(SimpleVec2)max path:(UIBezierPath *)path;
- (void)drawLineFrom:(SimpleVec2)from to:(SimpleVec2)to path:(UIBezierPath *)path;
- (void)drawTextAt:(SimpleVec2)position text:(NSString*)text;
- (void)drawHealthBarAt:(SimpleVec2)min to:(SimpleVec2)max multiplier:(float)multiplier;
@end

@implementation ESPRenderer

+ (instancetype)sharedInstance {
    static ESPRenderer *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _espLayer = [CAShapeLayer layer];
        _espLayer.name = @"ESP_Layer_Optimized";
        _espLayer.fillColor = [UIColor clearColor].CGColor;
        _espLayer.strokeColor = kUnifiedRedColor;
        _espLayer.lineWidth = 1.2f;
        _textPool = [NSMutableArray new];
        _healthPool = [NSMutableArray new];
    }
    return self;
}

- (void)renderOnView:(UIView *)view {
    if (!view) return;
    if (!_containerView || _containerView != view) {
        _containerView = view;
        [_containerView.layer addSublayer:_espLayer];
    }
    _espLayer.frame = view.bounds;
    _textUsedCount = 0;
    _healthUsedCount = 0;
}

- (void)clearDrawings {
    _espLayer.path = nil;
    for (CATextLayer *layer in _textPool) layer.hidden = YES;
    for (CALayer *layer in _healthPool) layer.hidden = YES;
}

- (void)drawBoxFrom:(SimpleVec2)min to:(SimpleVec2)max path:(UIBezierPath *)path {
    if (!path) return;
    float w = max.x - min.x;
    float h = max.y - min.y;
    float cw = w * 0.25f; 
    float ch = h * 0.25f;
    
    [path moveToPoint:CGPointMake(min.x, min.y + ch)];
    [path addLineToPoint:CGPointMake(min.x, min.y)];
    [path addLineToPoint:CGPointMake(min.x + cw, min.y)];
    
    [path moveToPoint:CGPointMake(max.x - cw, min.y)];
    [path addLineToPoint:CGPointMake(max.x, min.y)];
    [path addLineToPoint:CGPointMake(max.x, min.y + ch)];

    [path moveToPoint:CGPointMake(min.x, max.y - ch)];
    [path addLineToPoint:CGPointMake(min.x, max.y)];
    [path addLineToPoint:CGPointMake(min.x + cw, max.y)];
   
    [path moveToPoint:CGPointMake(max.x - cw, max.y)];
    [path addLineToPoint:CGPointMake(max.x, max.y)];
    [path addLineToPoint:CGPointMake(max.x, max.y - ch)];
}

- (void)drawLineFrom:(SimpleVec2)from to:(SimpleVec2)to path:(UIBezierPath *)path {
    if (!path) return;
    [path moveToPoint:CGPointMake(from.x, from.y)];
    [path addLineToPoint:CGPointMake(to.x, to.y)];
}

- (void)drawTextAt:(SimpleVec2)position text:(NSString*)text {
    if (!text || !_containerView) return;
    CATextLayer *textLayer;
    if (_textUsedCount < _textPool.count) {
        textLayer = _textPool[_textUsedCount];
    } else {
        textLayer = [CATextLayer layer];
        textLayer.fontSize = 10.0f;
        textLayer.foregroundColor = [UIColor whiteColor].CGColor;
        textLayer.backgroundColor = [UIColor colorWithWhite:0 alpha:0.45].CGColor;
        textLayer.cornerRadius = 2.0f;
        textLayer.alignmentMode = kCAAlignmentCenter;
        textLayer.contentsScale = [UIScreen mainScreen].scale;
        [_containerView.layer addSublayer:textLayer];
        [_textPool addObject:textLayer];
    }
    textLayer.string = text;
    textLayer.hidden = NO;
    CGSize textSize = [text sizeWithAttributes:@{NSFontAttributeName: [UIFont systemFontOfSize:9.0f]}];
    textLayer.frame = CGRectMake(position.x - textSize.width/2, position.y, textSize.width + 4, textSize.height + 2);
    _textUsedCount++;
}

- (void)drawHealthBarAt:(SimpleVec2)min to:(SimpleVec2)max multiplier:(float)multiplier {
    if (!_containerView) return;
    CALayer *bgLayer;
    CALayer *fgLayer;
    int index = _healthUsedCount * 2;
    
    if (index + 1 < _healthPool.count) {
        bgLayer = _healthPool[index];
        fgLayer = _healthPool[index + 1];
    } else {
        bgLayer = [CALayer layer];
        bgLayer.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.6].CGColor;
        [_containerView.layer addSublayer:bgLayer];
        [_healthPool addObject:bgLayer];
        
        fgLayer = [CALayer layer];
        [_containerView.layer addSublayer:fgLayer];
        [_healthPool addObject:fgLayer];
    }
    
  
    UIColor *hpColor;
    if (multiplier > 0.6f)      hpColor = [UIColor colorWithRed:0.2 green:1.0 blue:0.2 alpha:1.0]; // أخضر
    else if (multiplier > 0.3f) hpColor = [UIColor colorWithRed:1.0 green:0.85 blue:0.0 alpha:1.0]; // أصفر
    else                        hpColor = [UIColor colorWithRed:1.0 green:0.1 blue:0.1 alpha:1.0]; // أحمر
    
    fgLayer.backgroundColor = hpColor.CGColor;
    
    float height = max.y - min.y;
    float barW = 3.0f; 
    bgLayer.frame = CGRectMake(min.x, min.y, barW, height);
    fgLayer.frame = CGRectMake(min.x, max.y - (height * multiplier), barW, height * multiplier);
    bgLayer.hidden = NO;
    fgLayer.hidden = NO;
    _healthUsedCount++;
}

@end


// Head collider getter (reference build: entity + 0x4a4 holds the same pointer).
// Cached so the dyld scan does not run every frame.
static void *(*g_headColliderFn)(void *) = nullptr;

static void *Player_GetHeadCollider(void *p) {
    if (!p) return nullptr;
    if (!g_headColliderFn) {
        void *addr = (void *)getRealOffset(0x4A1A9D4);
        if (!addr) return nullptr;
        g_headColliderFn = (void *(*)(void *))addr;
    }
    return g_headColliderFn(p);
}
static Vector3 Transform_INTERNAL_GetPosition(void *tf) {
    Vector3 o = Vector3();
    ((void(*)(void*,Vector3*))getRealOffset(0x8552C10))(tf,&o);
    return o;
}
// Layer mask 12 is the value this project already used. Verify the export at
// 0x5580870 before trusting the result: if it is the stock Unity
// Physics.Raycast(Vector3, Vector3, float maxDistance, int layerMask), then the
// third argument arrives as a denormal float and the fourth as a pointer-sized
// int, and the result is meaningless either way.
static bool Physics_Raycast(Vector3 a, Vector3 b, unsigned int l, void *c) {
    void *fn = (void*)getRealOffset(0x5580870);
    if (!fn) return true;    // unresolved: treat as blocked, never aim at what cannot be checked
    return ((bool(*)(Vector3,Vector3,unsigned int,void*))fn)(a,b,l,c);
}
static void *getItransform(void *itf)
    { return ((void*(*)(void*))getRealOffset(0x5C52CFC))(itf); }

// The head offset yields a node, not a transform: node -> transform -> position.
// Same shape as the reference build (Bones::Head -> GetNodePosition).
inline Vector3 GetHeadPosition(void *p) {
    if (!p || !game_sdk->GetHeadPositions) return Vector3();
    void *node = game_sdk->GetHeadPositions(p);
    if (!node) return Vector3();
    void *tf = game_sdk->Component_GetTransform(node);
    return tf ? game_sdk->get_position(tf) : Vector3();
}
inline Vector3 GetHipPosition(void *p) {
    if (!p) return Vector3();
    void *HipITF = *(void**)((uint64_t)p + 0x648);
    if (!HipITF) return Vector3();
    void *HipTF = getItransform(HipITF);
    return HipTF ? Transform_INTERNAL_GetPosition(HipTF) : Vector3();
}
inline Vector3 CameraMain(void *p) {
    if (!p) return Vector3();
    void *camTF = *(void**)((uint64_t)p + 0x390);
    return camTF ? game_sdk->get_position(camTF) : Vector3();
}
inline bool IsGod(void *p) { return *(bool*)((uint64_t)p + 0xF4C); }

// The aimbot lives in its own module now. See Includes/Aimbot.h.
// It replaces GetHitboxPosition / ff_isVisible / GetClosestEnemy / ProcessAimbot.
#import "Aimbot.h"

// Offset backed modifications (no recoil, fast fire, no reload, ...).
#import "GameMods.h"

@interface FOVCircleManager : NSObject
@property (nonatomic, strong) CAShapeLayer *fovLayer;
+ (instancetype)shared;
- (void)updateOnView:(UIView*)view;
@end

@implementation FOVCircleManager
+ (instancetype)shared {
    static FOVCircleManager *i; static dispatch_once_t t;
    dispatch_once(&t,^{ i=[self new]; });
    return i;
}
- (instancetype)init {
    self=[super init];
    _fovLayer=[CAShapeLayer layer];
    _fovLayer.fillColor=UIColor.clearColor.CGColor;
    _fovLayer.strokeColor=[[UIColor whiteColor] colorWithAlphaComponent:0.85].CGColor;
    _fovLayer.lineWidth=1.0f;
    return self;
}
- (void)updateOnView:(UIView*)view {
    if (!view) return;
    if (_fovLayer.superlayer != view.layer) [view.layer addSublayer:_fovLayer];
    _fovLayer.frame = view.bounds;

    if (!Vars.Aimbot || !Vars.ShowFovCircle) {
        _fovLayer.path = nil;
        return;
    }

    ScreenMetrics screen = GetScreenMetrics();
    float cx = screen.width  * 0.5f;
    float cy = screen.height * 0.5f;

    // Aim Visible mode uses a pixel radius around the crosshair, same as the
    // reference build draws it: red, 64 segments, thickness 2.
    if (Vars.AimVisible) {
        float r = Vars.AimFov;
        if (r < 1.0f) { _fovLayer.path = nil; return; }
        _fovLayer.strokeColor = [[UIColor colorWithRed:1.0 green:0.0 blue:0.0 alpha:0.8] CGColor];
        _fovLayer.lineWidth = 2.0f;
        _fovLayer.path = [UIBezierPath bezierPathWithOvalInRect:
            CGRectMake(cx - r, cy - r, r * 2, r * 2)].CGPath;
        return;
    }

    _fovLayer.strokeColor = [[UIColor whiteColor] colorWithAlphaComponent:0.85].CGColor;
    _fovLayer.lineWidth = 1.0f;

    if (Vars.AimMaxAngleDeg <= 0.0f) {
        _fovLayer.path = nil;
        return;
    }

    // The circle is drawn from the real angular cap: project a ray rotated by
    // half the cap and read the radius back in screen points.
    void *match = game_sdk->Curent_Match();
    void *local = match ? game_sdk->GetLocalPlayer(match) : nullptr;
    if (!local) { _fovLayer.path = nil; return; }

    Vector3 camPos;
    if (!aim::CameraPosition(local, camPos)) { _fovLayer.path = nil; return; }

    Vector3 fwd = game_sdk->GetForward ? game_sdk->GetForward(local) : Vector3();
    if (Vector3::Magnitude(fwd) < 1e-4f) { _fovLayer.path = nil; return; }

    float half = Vars.AimMaxAngleDeg * 0.5f * 0.017453292519943295f;
    Vector3 right = Vector3::Normalized(Vector3::Cross(fwd, Vector3(0, 1, 0)));
    if (Vector3::Magnitude(right) < 1e-4f) { _fovLayer.path = nil; return; }

    Vector3 edge = Vector3::Normalized(fwd * cosf(half) + right * sinf(half));

    SimpleVec2 c = Camera$$WorldToScreen::Regular(camPos + fwd * 100.0f);
    SimpleVec2 e = Camera$$WorldToScreen::Regular(camPos + edge * 100.0f);
    float rx = fabsf(e.x - c.x);
    if (rx < 1.0f) { _fovLayer.path = nil; return; }
    float ry = rx * (view.bounds.size.height / view.bounds.size.width);

    _fovLayer.path = [UIBezierPath bezierPathWithOvalInRect:
        CGRectMake(c.x - rx, c.y - ry, rx * 2, ry * 2)].CGPath;
}
@end


inline void get_players()
{
 
    if (!game_sdk) return;

    
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!Vars.Enable) {
            [[ESPRenderer sharedInstance] clearDrawings];
            return;
        }

        
        if (Vars.NoFog) {
            void *noFogAddr = (void*)getRealOffset(0x850363C);
            if (noFogAddr) ((void (*)(bool))noFogAddr)(false);
        }

        void *current_Match = game_sdk->Curent_Match();
        if (!current_Match) {
            [[ESPRenderer sharedInstance] clearDrawings];
            return;
        }


        void *local_player = game_sdk->GetLocalPlayer(current_Match);
        if (!local_player) return;

        // Aimbot runs on its own module and does its own entity walk.
        aim::Tick(current_Match, local_player);

        // Offset backed mods: no recoil, fast fire, fast swap, no reload.
        mods::Apply(local_player);

        void *playersListAddr = (void*)getRealOffset(0x4C869DC);
        if (!playersListAddr) return;
        
        monoList<void **> *players = ((monoList<void **>* (*)(void*))playersListAddr)(current_Match);
        if (!players || !players->getItems()) return;

        UIWindow *keyWindow = [UIApplication sharedApplication].keyWindow;
        if (!keyWindow) return;

        ESPRenderer *renderer = [ESPRenderer sharedInstance];
        [renderer renderOnView:keyWindow];
        [renderer clearDrawings];
        

        [[FOVCircleManager shared] updateOnView:keyWindow];

        UIBezierPath *combinedPath = [UIBezierPath bezierPath];
        Vector3 localPos = getPosition(local_player);
        int drawnCount = 0;
        int totalEnemies = 0;

     
        static CGFloat sW = 0, sH = 0;
        if (sW == 0) {
            CGRect b = [UIScreen mainScreen].nativeBounds;
            CGFloat scale = [UIScreen mainScreen].nativeScale;
            sW = b.size.width / scale;
            sH = b.size.height / scale;
            if (sW < sH) { CGFloat t = sW; sW = sH; sH = t; }
        }

        SimpleVec2 lineStart(sW / 2.0f, 0);  

        for (int u = 0; u < players->getSize(); u++) {
            void *enemy = players->getItems()[u];
            if (!enemy || enemy == local_player) continue;
            
         
            if (!game_sdk->Component_GetTransform(enemy)) continue;
            if (game_sdk->get_IsDieing(enemy) || game_sdk->get_isLocalTeam(enemy)) continue;

            totalEnemies++;
            if (drawnCount >= 30) continue;

            Vector3 pos = getPosition(enemy);
            float distance = Vector3::Distance(pos, localPos);
            if (distance > 150.0f) continue;

            SimpleVec2 bot_pos = Camera$$WorldToScreen::Regular(pos);
            if (bot_pos.x == 0 && bot_pos.y == 0) continue;

            SimpleVec2 top_pos = Camera$$WorldToScreen::Regular(pos + Vector3(0, 1.8f, 0));
            float height = fabsf(bot_pos.y - top_pos.y);
            float width = height / 2.0f;

            if (Vars.Box) {
                [renderer drawBoxFrom:SimpleVec2(bot_pos.x - width/2, top_pos.y) to:SimpleVec2(bot_pos.x + width/2, bot_pos.y) path:combinedPath];
            }

            if (Vars.lines) {
                [renderer drawLineFrom:lineStart to:SimpleVec2(bot_pos.x, top_pos.y) path:combinedPath];
            }

            if (Vars.skeleton) {
                Vector3 bones[] = {
                    GetBonePosition(enemy, game_sdk->_GetHeadPositions),
                    GetBonePosition(enemy, game_sdk->_newHipMods),
                    GetBonePosition(enemy, game_sdk->_getLeftHandTF),
                    GetBonePosition(enemy, game_sdk->_getRightHandTF),
                    GetBonePosition(enemy, game_sdk->_GetLeftAnkleTF),
                    GetBonePosition(enemy, game_sdk->_GetRightAnkleTF)
                };
                SimpleVec2 sBones[6];
                for(int i=0; i<6; i++) sBones[i] = Camera$$WorldToScreen::Regular(bones[i]);
                
                if (sBones[0].x != 0) {
                    [renderer drawLineFrom:sBones[0] to:sBones[1] path:combinedPath]; 
                    [renderer drawLineFrom:sBones[1] to:sBones[4] path:combinedPath]; 
                    [renderer drawLineFrom:sBones[1] to:sBones[5] path:combinedPath]; 
                    [renderer drawLineFrom:sBones[0] to:sBones[2] path:combinedPath]; 
                    [renderer drawLineFrom:sBones[0] to:sBones[3] path:combinedPath]; 
                }
            }

            if (Vars.Health) {
                int hpVal = game_sdk->GetHp(enemy);
                int maxHpVal = game_sdk->get_MaxHP(enemy);
                if (maxHpVal > 0) {
                    float hp = (float)hpVal / (float)maxHpVal;
                    if (hp > 1.0f) hp = 1.0f;
                    if (hp < 0.0f) hp = 0.0f;
                    [renderer drawHealthBarAt:SimpleVec2(bot_pos.x - width/2 - 4, top_pos.y) to:SimpleVec2(bot_pos.x - width/2 - 4, bot_pos.y) multiplier:hp];
                }
            }

            if (Vars.Name) {
                monoString *pname = game_sdk->name(enemy);
                if (pname) {
                    NSString *nsName = pname->toNSString();
                    if (nsName) [renderer drawTextAt:SimpleVec2(bot_pos.x, top_pos.y - 14) text:nsName];
                }
            }

            if (Vars.Distance) {
                [renderer drawTextAt:SimpleVec2(bot_pos.x + width/2 + 5, top_pos.y) text:[NSString stringWithFormat:@"%.0fm", distance]];
            }

            drawnCount++;
        }

        [renderer drawTextAt:SimpleVec2(sW / 2, 38) text:[NSString stringWithFormat:@"[%d]", totalEnemies]];

        renderer.espLayer.path = combinedPath.CGPath;
    });
}
