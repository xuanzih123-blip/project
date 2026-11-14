#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <substrate.h>
#if __has_include(<QuartzCore/CAMetalLayer.h>)
#import <QuartzCore/CAMetalLayer.h>
#endif


#import "Includes/Vector3.h"
#import "Includes/Vector2.h"
#import "Includes/Quaternion.h"
#import "Includes/UnityTypes.h"
#import "Includes/MemoryUtils.h"
#import "Includes/ESP.h"
#import "Includes/Prefs.h"
#import "Includes/Encryption.h"

#define kWidth  [UIScreen mainScreen].bounds.size.width
#define kHeight [UIScreen mainScreen].bounds.size.height


#define COLOR_BG        [UIColor colorWithRed:0.11 green:0.11 blue:0.13 alpha:0.97]
#define COLOR_PANEL     [UIColor colorWithRed:0.14 green:0.14 blue:0.16 alpha:1.0]
#define COLOR_SIDEBAR   [UIColor colorWithRed:0.10 green:0.10 blue:0.12 alpha:1.0]
#define COLOR_ACCENT    [UIColor colorWithRed:1.0  green:0.38 blue:0.10 alpha:1.0]
#define COLOR_ROW       [UIColor colorWithRed:0.18 green:0.18 blue:0.21 alpha:1.0]
#define COLOR_TEXT      [UIColor whiteColor]
#define COLOR_SUBTEXT   [UIColor colorWithWhite:0.55 alpha:1.0]

typedef NS_ENUM(NSInteger, MenuTab) {
    MenuTabAimbot   = 0,
    MenuTabESP      = 1,
    MenuTabMisc     = 2,
    MenuTabSettings = 3,
    MenuTabProfile  = 4,
};


static UIImage *ZSFImage(NSString *name, CGFloat size) {
    if (@available(iOS 13.0, *)) {
        UIImageSymbolConfiguration *cfg = [UIImageSymbolConfiguration
            configurationWithPointSize:size weight:UIImageSymbolWeightMedium];
        return [[UIImage systemImageNamed:name withConfiguration:cfg]
                imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    }

    UIGraphicsBeginImageContextWithOptions(CGSizeMake(size, size), NO, 0);
    CGContextRef ctx = UIGraphicsGetCurrentContext();
    CGContextSetFillColorWithColor(ctx, [UIColor colorWithWhite:0.6 alpha:1.0].CGColor);
    CGContextFillEllipseInRect(ctx, CGRectMake(0, 0, size, size));
    UIImage *img = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return img;
}


@interface ZToggleRow : UIView
@property (nonatomic, strong) UILabel *titleLabel;
@property (nonatomic, strong) UIView  *toggleTrack;
@property (nonatomic, strong) UIView  *toggleThumb;
@property (nonatomic, assign) BOOL     isOn;
@property (nonatomic, copy)   void (^onToggle)(BOOL isOn);
- (instancetype)initWithTitle:(NSString *)title isOn:(BOOL)on;
@end

@implementation ZToggleRow
- (instancetype)initWithTitle:(NSString *)title isOn:(BOOL)on {
    self = [super init];
    if (!self) return nil;
    _isOn = on;
    self.backgroundColor = COLOR_ROW;
    self.layer.cornerRadius = 10;
    self.clipsToBounds = YES;

    _titleLabel = [[UILabel alloc] init];
    _titleLabel.text = title;
    _titleLabel.textColor = COLOR_TEXT;
    _titleLabel.font = [UIFont systemFontOfSize:14 weight:UIFontWeightRegular];
    _titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_titleLabel];

    _toggleTrack = [[UIView alloc] init];
    _toggleTrack.layer.cornerRadius = 13;
    _toggleTrack.clipsToBounds = YES;
    _toggleTrack.translatesAutoresizingMaskIntoConstraints = NO;
    _toggleTrack.backgroundColor = on ? COLOR_ACCENT : [UIColor colorWithWhite:0.3 alpha:1.0];
    [self addSubview:_toggleTrack];

    _toggleThumb = [[UIView alloc] init];
    _toggleThumb.layer.cornerRadius = 11;
    _toggleThumb.backgroundColor = UIColor.whiteColor;
    _toggleThumb.translatesAutoresizingMaskIntoConstraints = NO;
    [_toggleTrack addSubview:_toggleThumb];

    [NSLayoutConstraint activateConstraints:@[
        [_titleLabel.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:14],
        [_titleLabel.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
        [_toggleTrack.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-14],
        [_toggleTrack.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
        [_toggleTrack.widthAnchor constraintEqualToConstant:42],
        [_toggleTrack.heightAnchor constraintEqualToConstant:26],
        [_toggleThumb.centerYAnchor constraintEqualToAnchor:_toggleTrack.centerYAnchor],
        [_toggleThumb.widthAnchor constraintEqualToConstant:22],
        [_toggleThumb.heightAnchor constraintEqualToConstant:22],
    ]];
    [self _applyThumbPosition:NO];

    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
        initWithTarget:self action:@selector(_tapped)];
    [self addGestureRecognizer:tap];
    return self;
}

- (void)_tapped {
    _isOn = !_isOn;
    _toggleTrack.backgroundColor = _isOn ? COLOR_ACCENT : [UIColor colorWithWhite:0.3 alpha:1.0];
    [self _applyThumbPosition:YES];
    if (_onToggle) _onToggle(_isOn);
}

- (void)_applyThumbPosition:(BOOL)animated {
    for (NSLayoutConstraint *c in _toggleTrack.constraints) {
        if ((c.firstItem == _toggleThumb || c.secondItem == _toggleThumb)
            && c.firstAttribute == NSLayoutAttributeLeading) {
            [c setActive:NO];
        }
    }
    [[_toggleThumb.leadingAnchor constraintEqualToAnchor:_toggleTrack.leadingAnchor
                                                constant:_isOn ? 18 : 2] setActive:YES];
    if (animated) {
        [UIView animateWithDuration:0.18 animations:^{ [self->_toggleTrack layoutIfNeeded]; }];
    }
}
@end


@interface ZSliderRow : UIView
@property (nonatomic, strong) UILabel  *titleLabel;
@property (nonatomic, strong) UILabel  *valueLabel;
@property (nonatomic, strong) UISlider *slider;
@property (nonatomic, copy)   void (^onChange)(float value);
- (instancetype)initWithTitle:(NSString *)title min:(float)min max:(float)max value:(float)val;
@end

@implementation ZSliderRow
- (instancetype)initWithTitle:(NSString *)title min:(float)min max:(float)max value:(float)val {
    self = [super init];
    if (!self) return nil;
    self.backgroundColor = COLOR_ROW;
    self.layer.cornerRadius = 10;
    self.clipsToBounds = YES;

    _titleLabel = [[UILabel alloc] init];
    _titleLabel.text = title;
    _titleLabel.textColor = COLOR_TEXT;
    _titleLabel.font = [UIFont systemFontOfSize:14];
    _titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_titleLabel];

    _valueLabel = [[UILabel alloc] init];
    _valueLabel.text = [NSString stringWithFormat:@"%.0f", val];
    _valueLabel.textColor = COLOR_ACCENT;
    _valueLabel.font = [UIFont systemFontOfSize:14 weight:UIFontWeightBold];
    _valueLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_valueLabel];

    _slider = [[UISlider alloc] init];
    _slider.minimumValue = min;
    _slider.maximumValue = max;
    _slider.value = val;
    _slider.minimumTrackTintColor = COLOR_ACCENT;
    _slider.maximumTrackTintColor = [UIColor colorWithWhite:0.3 alpha:1.0];
 
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(22,22), NO, 0);
    CGContextRef ctx = UIGraphicsGetCurrentContext();
    CGContextSetFillColorWithColor(ctx, COLOR_ACCENT.CGColor);
    CGContextFillEllipseInRect(ctx, CGRectMake(0,0,22,22));
    UIImage *thumb = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    [_slider setThumbImage:thumb forState:UIControlStateNormal];
    [_slider setThumbImage:thumb forState:UIControlStateHighlighted];
    _slider.translatesAutoresizingMaskIntoConstraints = NO;
    [_slider addTarget:self action:@selector(_changed) forControlEvents:UIControlEventValueChanged];
    [self addSubview:_slider];

    [NSLayoutConstraint activateConstraints:@[
        [_titleLabel.topAnchor constraintEqualToAnchor:self.topAnchor constant:10],
        [_titleLabel.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:14],
        [_valueLabel.topAnchor constraintEqualToAnchor:self.topAnchor constant:10],
        [_valueLabel.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-14],
        [_slider.topAnchor constraintEqualToAnchor:_titleLabel.bottomAnchor constant:6],
        [_slider.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:14],
        [_slider.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-14],
        [_slider.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-10],
    ]];
    return self;
}
- (void)_changed {
    _valueLabel.text = [NSString stringWithFormat:@"%.0f", _slider.value];
    if (_onChange) _onChange(_slider.value);
}
@end


@interface ZSidebarButton : UIButton
@property (nonatomic, assign) MenuTab tab;
- (instancetype)initWithTab:(MenuTab)tab imageName:(NSString *)name;
- (void)setActive:(BOOL)active;
@end

@implementation ZSidebarButton
- (instancetype)initWithTab:(MenuTab)tab imageName:(NSString *)name {
    self = [super initWithFrame:CGRectMake(0, 0, 46, 46)];
    if (!self) return nil;
    _tab = tab;
    UIImage *img = ZSFImage(name, 18);
    [self setImage:img forState:UIControlStateNormal];
    self.tintColor = [UIColor colorWithWhite:0.45 alpha:1.0];
    self.layer.cornerRadius = 12;
    return self;
}
- (void)setActive:(BOOL)active {
    [UIView animateWithDuration:0.18 animations:^{
        self.tintColor = active ? COLOR_ACCENT : [UIColor colorWithWhite:0.45 alpha:1.0];
        self.backgroundColor = active ? [COLOR_ACCENT colorWithAlphaComponent:0.15] : UIColor.clearColor;
    }];
}
@end


@interface monaMenu : NSObject
@property (nonatomic, strong) UIView       *menuView;
@property (nonatomic, strong) UIView       *sidebarView;
@property (nonatomic, strong) UIView       *panelView;
@property (nonatomic, strong) UIScrollView *scrollView;
@property (nonatomic, strong) UILabel      *tabTitleLabel;
@property (nonatomic, strong) UIImageView  *tabIconView;
@property (nonatomic, assign) MenuTab       currentTab;
@property (nonatomic, strong) NSArray<ZSidebarButton *> *sidebarButtons;
@property (nonatomic, strong) CADisplayLink *displayLink;
@property (nonatomic, assign) CGPoint        lastPoint;
@end

@implementation monaMenu

static monaMenu *extraInfo;
static BOOL MenDeal;
UIWindow *mainWindow;
game_sdk_t *game_sdk = new game_sdk_t();

// The toggles only ever wrote to storage; Vars kept its defaults and every
// persisted feature stayed off. Restore them once, at startup.
static void RestoreVarsFromPrefs(void) {
    Msa7 *p = [Msa7 shared];

    Vars.Aimbot        = [p boolFor:@"Aimbot" defaultValue:NO];
    Vars.AimVisible      = [p boolFor:@"AimVisible" defaultValue:NO];
    Vars.AimWriteField   = [p boolFor:@"AimWriteField" defaultValue:NO];
    Vars.AimSelfTarget   = [p boolFor:@"AimSelfTarget" defaultValue:NO];
    Vars.HumanAim      = [p boolFor:@"HumanAim" defaultValue:YES];
    Vars.ShowFovCircle = [p boolFor:@"ShowFovCircle" defaultValue:YES];
    Vars.VisibleCheck  = [p boolFor:@"VisibleCheck" defaultValue:YES];
    Vars.IgnoreKnocked = [p boolFor:@"IgnoreKnocked" defaultValue:YES];

    Vars.Enable   = [p boolFor:@"Enable" defaultValue:YES];
    Vars.Box      = [p boolFor:@"Box" defaultValue:NO];
    Vars.lines    = [p boolFor:@"Lines" defaultValue:NO];
    Vars.Name     = [p boolFor:@"Name" defaultValue:NO];
    Vars.Health   = [p boolFor:@"Health" defaultValue:NO];
    Vars.Distance = [p boolFor:@"Distance" defaultValue:NO];
    Vars.skeleton = [p boolFor:@"Skeleton" defaultValue:NO];

    Vars.NoRecoil      = [p boolFor:@"NoRecoil" defaultValue:NO];
    Vars.FastFire      = [p boolFor:@"FastFire" defaultValue:NO];
    Vars.FastSwap      = [p boolFor:@"FastSwap" defaultValue:NO];
    Vars.FastReload    = [p boolFor:@"FastReload" defaultValue:NO];
    Vars.NoReload      = [p boolFor:@"NoReload" defaultValue:NO];
    Vars.UnlimitedAmmo = [p boolFor:@"UnlimitedAmmo" defaultValue:NO];
    Vars.Force120FPS   = [p boolFor:@"Force120FPS" defaultValue:NO];

    Vars.AimIntervalMs   = [p floatFor:@"AimIntervalMs" defaultValue:16.0f];
    Vars.AimFov          = [p floatFor:@"AimFov" defaultValue:150.0f];
    Vars.AimRange        = [p floatFor:@"AimRange" defaultValue:200.0f];
    Vars.AimMaxAngleDeg  = [p floatFor:@"AimMaxAngleDeg" defaultValue:0.0f];
    Vars.AimHeightOffset = [p floatFor:@"AimHeightOffset" defaultValue:0.10f];
    Vars.AimReactionMs   = [p floatFor:@"AimReactionMs" defaultValue:210.0f];
    Vars.AimFlickBaseMs  = [p floatFor:@"AimFlickBaseMs" defaultValue:90.0f];
    Vars.AimOvershootPct = [p floatFor:@"AimOvershootPct" defaultValue:8.0f];
    Vars.AimCorrectionMs = [p floatFor:@"AimCorrectionMs" defaultValue:95.0f];
    Vars.AimTremorDeg    = [p floatFor:@"AimTremorDeg" defaultValue:0.30f];
    Vars.AimErrorDeg     = [p floatFor:@"AimErrorDeg" defaultValue:0.35f];
    Vars.AimLeadMs       = [p floatFor:@"AimLeadMs" defaultValue:70.0f];
}

+ (void)load {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        mainWindow = [UIApplication sharedApplication].keyWindow;
        extraInfo = [monaMenu new];
        static bool sdkInitialized = false;
        if (!sdkInitialized) { game_sdk->init(); sdkInitialized = true; }
        RestoreVarsFromPrefs();
        [extraInfo setupDisplayLink];
        [extraInfo initTapGes];
    });
}

- (void)setupDisplayLink {
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(updateMenu)];
    _displayLink.frameInterval = 2;
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}


- (void)setupMenu {
    CGFloat menuW = 560;
    CGFloat menuH = 420;
    CGFloat x = (kWidth  - menuW) * 0.5f;
    CGFloat y = (kHeight - menuH) * 0.5f;

    _menuView = [[UIView alloc] initWithFrame:CGRectMake(x, y, menuW, menuH)];
    _menuView.backgroundColor = UIColor.clearColor;
    _menuView.hidden = YES;
    _menuView.userInteractionEnabled = YES;
    [mainWindow addSubview:_menuView];

    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc]
        initWithTarget:self action:@selector(handlePan:)];
    [_menuView addGestureRecognizer:pan];


    CGFloat sideW = 64;
    _sidebarView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, sideW, menuH)];
    _sidebarView.backgroundColor = COLOR_SIDEBAR;
    _sidebarView.layer.cornerRadius = 16;
    _sidebarView.clipsToBounds = YES;
    [_menuView addSubview:_sidebarView];

    NSArray *icons = @[@"scope", @"eye", @"cube.box", @"gearshape", @"person"];
    NSMutableArray *btns = [NSMutableArray array];
    CGFloat iconY = 16;
    for (int i = 0; i < (int)icons.count; i++) {
        ZSidebarButton *btn = [[ZSidebarButton alloc] initWithTab:(MenuTab)i imageName:icons[i]];
        btn.frame = CGRectMake(9, iconY, 46, 46);
        [btn addTarget:self action:@selector(sidebarTapped:) forControlEvents:UIControlEventTouchUpInside];
        [_sidebarView addSubview:btn];
        [btns addObject:btn];
        iconY += 52;
    }
    _sidebarButtons = [btns copy];
    [_sidebarButtons[0] setActive:YES];


    CGFloat panelW = menuW - sideW - 6;
    _panelView = [[UIView alloc] initWithFrame:CGRectMake(sideW + 6, 0, panelW, menuH)];
    _panelView.backgroundColor = COLOR_PANEL;
    _panelView.layer.cornerRadius = 16;
    _panelView.clipsToBounds = YES;
    [_menuView addSubview:_panelView];

   
    UIView *header = [[UIView alloc] initWithFrame:CGRectMake(0, 0, panelW, 52)];
    header.backgroundColor = UIColor.clearColor;
    [_panelView addSubview:header];

    
    UIView *iconBg = [[UIView alloc] initWithFrame:CGRectMake(14, 12, 28, 28)];
    iconBg.backgroundColor = [COLOR_ACCENT colorWithAlphaComponent:0.2];
    iconBg.layer.cornerRadius = 8;
    [header addSubview:iconBg];

    _tabIconView = [[UIImageView alloc] initWithFrame:CGRectMake(7, 7, 14, 14)];
    _tabIconView.image = ZSFImage(@"cube.box", 14);
    _tabIconView.tintColor = COLOR_ACCENT;
    [iconBg addSubview:_tabIconView];

    _tabTitleLabel = [[UILabel alloc] initWithFrame:CGRectMake(50, 14, 200, 24)];
    _tabTitleLabel.text = @"AIMBOT";
    _tabTitleLabel.textColor = COLOR_TEXT;
    _tabTitleLabel.font = [UIFont systemFontOfSize:15 weight:UIFontWeightSemibold];
    [header addSubview:_tabTitleLabel];

  
    UIButton *closeBtn = [UIButton buttonWithType:UIButtonTypeSystem];
    [closeBtn setImage:ZSFImage(@"xmark", 14) forState:UIControlStateNormal];
    closeBtn.tintColor = [UIColor colorWithWhite:0.5 alpha:1.0];
    closeBtn.frame = CGRectMake(panelW - 44, 12, 28, 28);
    closeBtn.backgroundColor = [UIColor colorWithWhite:0.22 alpha:1.0];
    closeBtn.layer.cornerRadius = 8;
    [closeBtn addTarget:self action:@selector(closeMenu) forControlEvents:UIControlEventTouchUpInside];
    [header addSubview:closeBtn];

   
    UIButton *moonBtn = [UIButton buttonWithType:UIButtonTypeSystem];
    [moonBtn setImage:ZSFImage(@"moon", 14) forState:UIControlStateNormal];
    moonBtn.tintColor = [UIColor colorWithWhite:0.5 alpha:1.0];
    moonBtn.frame = CGRectMake(panelW - 80, 12, 28, 28);
    moonBtn.backgroundColor = [UIColor colorWithWhite:0.22 alpha:1.0];
    moonBtn.layer.cornerRadius = 8;
    [header addSubview:moonBtn];

 
    UIView *sep = [[UIView alloc] initWithFrame:CGRectMake(0, 51, panelW, 0.5)];
    sep.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.08];
    [_panelView addSubview:sep];


    _scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 52, panelW, menuH - 52)];
    _scrollView.showsVerticalScrollIndicator = NO;
    _scrollView.alwaysBounceVertical = YES;
    [_panelView addSubview:_scrollView];

    [self loadTab:MenuTabAimbot];
}


- (void)loadTab:(MenuTab)tab {
    _currentTab = tab;
    for (UIView *v in _scrollView.subviews) [v removeFromSuperview];

    CGFloat panelW = _panelView.bounds.size.width;
    CGFloat rowH = 52, gap = 8, px = 12;
    CGFloat rw = panelW - px * 2;
    CGFloat y  = 10;

    switch (tab) {

        case MenuTabAimbot: {
            _tabTitleLabel.text = @"AIMBOT";
            _tabIconView.image = ZSFImage(@"scope", 14);

            NSArray *rows = @[
                @[@"Aimbot",          @"Aimbot"],
                @[@"Aim Visible",     @"AimVisible"],
                @[@"Write Collider",  @"AimWriteField"],
                @[@"Self Target",     @"AimSelfTarget"],
                @[@"Human Aim",       @"HumanAim"],
                @[@"Show FOV Circle", @"ShowFovCircle"],
                @[@"Visible Check",   @"VisibleCheck"],
                @[@"Ignore Knocked",  @"IgnoreKnocked"],
            ];
            for (NSArray *r in rows) {
                NSString *key = r[1];
                BOOL st = [[Msa7 shared] boolFor:key];
                ZToggleRow *row = [[ZToggleRow alloc] initWithTitle:r[0] isOn:st];
                row.frame = CGRectMake(px, y, rw, rowH);
                row.onToggle = ^(BOOL on) {
                    [[Msa7 shared] setBool:on for:key];
                    if ([key isEqualToString:@"Aimbot"])          Vars.Aimbot = on;
                    if ([key isEqualToString:@"AimVisible"])      Vars.AimVisible = on;
                    if ([key isEqualToString:@"AimWriteField"])   Vars.AimWriteField = on;
                    if ([key isEqualToString:@"AimSelfTarget"])   Vars.AimSelfTarget = on;
                    if ([key isEqualToString:@"HumanAim"])        Vars.HumanAim = on;
                    if ([key isEqualToString:@"ShowFovCircle"])   Vars.ShowFovCircle = on;
                    if ([key isEqualToString:@"VisibleCheck"])    Vars.VisibleCheck = on;
                    if ([key isEqualToString:@"IgnoreKnocked"])   Vars.IgnoreKnocked = on;
                };
                [_scrollView addSubview:row];
                y += rowH + gap;
            }

            ZSliderRow *fov = [[ZSliderRow alloc] initWithTitle:@"FOV (px)" min:20 max:600 value:Vars.AimFov];
            fov.frame = CGRectMake(px, y, rw, 70);
            fov.onChange = ^(float v) {
                Vars.AimFov = v;
                [[Msa7 shared] setFloat:v for:@"AimFov"];
            };
            [_scrollView addSubview:fov];
            y += 70 + gap;

            ZSliderRow *reaction = [[ZSliderRow alloc] initWithTitle:@"Reaction (ms)" min:100 max:400 value:Vars.AimReactionMs];
            reaction.frame = CGRectMake(px, y, rw, 70);
            reaction.onChange = ^(float v) {
                Vars.AimReactionMs = v;
                [[Msa7 shared] setFloat:v for:@"AimReactionMs"];
            };
            [_scrollView addSubview:reaction];
            y += 70 + gap;

            ZSliderRow *flick = [[ZSliderRow alloc] initWithTitle:@"Flick base (ms)" min:40 max:200 value:Vars.AimFlickBaseMs];
            flick.frame = CGRectMake(px, y, rw, 70);
            flick.onChange = ^(float v) {
                Vars.AimFlickBaseMs = v;
                [[Msa7 shared] setFloat:v for:@"AimFlickBaseMs"];
            };
            [_scrollView addSubview:flick];
            y += 70 + gap;

            ZSliderRow *overshoot = [[ZSliderRow alloc] initWithTitle:@"Overshoot (%)" min:0 max:25 value:Vars.AimOvershootPct];
            overshoot.frame = CGRectMake(px, y, rw, 70);
            overshoot.onChange = ^(float v) {
                Vars.AimOvershootPct = v;
                [[Msa7 shared] setFloat:v for:@"AimOvershootPct"];
            };
            [_scrollView addSubview:overshoot];
            y += 70 + gap;

            ZSliderRow *correction = [[ZSliderRow alloc] initWithTitle:@"Correction (ms)" min:40 max:250 value:Vars.AimCorrectionMs];
            correction.frame = CGRectMake(px, y, rw, 70);
            correction.onChange = ^(float v) {
                Vars.AimCorrectionMs = v;
                [[Msa7 shared] setFloat:v for:@"AimCorrectionMs"];
            };
            [_scrollView addSubview:correction];
            y += 70 + gap;

            ZSliderRow *tremor = [[ZSliderRow alloc] initWithTitle:@"Tremor (x100)" min:0 max:150 value:Vars.AimTremorDeg * 100.f];
            tremor.frame = CGRectMake(px, y, rw, 70);
            tremor.onChange = ^(float v) {
                Vars.AimTremorDeg = v / 100.f;
                [[Msa7 shared] setFloat:v / 100.f for:@"AimTremorDeg"];
            };
            [_scrollView addSubview:tremor];
            y += 70 + gap;

            ZSliderRow *errorDeg = [[ZSliderRow alloc] initWithTitle:@"Aim error (x100)" min:0 max:200 value:Vars.AimErrorDeg * 100.f];
            errorDeg.frame = CGRectMake(px, y, rw, 70);
            errorDeg.onChange = ^(float v) {
                Vars.AimErrorDeg = v / 100.f;
                [[Msa7 shared] setFloat:v / 100.f for:@"AimErrorDeg"];
            };
            [_scrollView addSubview:errorDeg];
            y += 70 + gap;

            ZSliderRow *lead = [[ZSliderRow alloc] initWithTitle:@"Lead (ms)" min:0 max:200 value:Vars.AimLeadMs];
            lead.frame = CGRectMake(px, y, rw, 70);
            lead.onChange = ^(float v) {
                Vars.AimLeadMs = v;
                [[Msa7 shared] setFloat:v for:@"AimLeadMs"];
            };
            [_scrollView addSubview:lead];
            y += 70 + gap;

            ZSliderRow *interval = [[ZSliderRow alloc] initWithTitle:@"Interval (ms)" min:8 max:60 value:Vars.AimIntervalMs];
            interval.frame = CGRectMake(px, y, rw, 70);
            interval.onChange = ^(float v) {
                Vars.AimIntervalMs = v;
                [[Msa7 shared] setFloat:v for:@"AimIntervalMs"];
            };
            [_scrollView addSubview:interval];
            y += 70 + gap;

            ZSliderRow *angle = [[ZSliderRow alloc] initWithTitle:@"Max angle (deg, 0=off)" min:0 max:60 value:Vars.AimMaxAngleDeg];
            angle.frame = CGRectMake(px, y, rw, 70);
            angle.onChange = ^(float v) {
                Vars.AimMaxAngleDeg = v;
                [[Msa7 shared] setFloat:v for:@"AimMaxAngleDeg"];
            };
            [_scrollView addSubview:angle];
            y += 70 + gap;

            ZSliderRow *range = [[ZSliderRow alloc] initWithTitle:@"Range (m)" min:20 max:300 value:Vars.AimRange];
            range.frame = CGRectMake(px, y, rw, 70);
            range.onChange = ^(float v) {
                Vars.AimRange = v;
                [[Msa7 shared] setFloat:v for:@"AimRange"];
            };
            [_scrollView addSubview:range];
            y += 70 + gap;

            ZSliderRow *height = [[ZSliderRow alloc] initWithTitle:@"Height x100" min:0 max:30 value:Vars.AimHeightOffset * 100.f];
            height.frame = CGRectMake(px, y, rw, 70);
            height.onChange = ^(float v) {
                Vars.AimHeightOffset = v / 100.f;
                [[Msa7 shared] setFloat:v / 100.f for:@"AimHeightOffset"];
            };
            [_scrollView addSubview:height];
            y += 70 + gap;
            break;
        }

        case MenuTabESP: {
            _tabTitleLabel.text = @"ESP";
            _tabIconView.image = ZSFImage(@"eye", 14);
            NSArray *rows = @[
                @[@"Enable ESP", @"Enable"],
                @[@"Box ESP",    @"Box"],
                @[@"Lines ESP",  @"Lines"],
                @[@"Names",      @"Name"],
                @[@"Health Bar", @"Health"],
                @[@"Distance",   @"Distance"],
                @[@"Skeleton",   @"Skeleton"],
            ];
            for (NSArray *r in rows) {
                BOOL st = [[Msa7 shared] boolFor:r[1]];
                ZToggleRow *row = [[ZToggleRow alloc] initWithTitle:r[0] isOn:st];
                row.frame = CGRectMake(px, y, rw, rowH);
                NSString *key = r[1];
                row.onToggle = ^(BOOL on) {
                    [[Msa7 shared] setBool:on for:key];
                    if ([key isEqualToString:@"Enable"])   Vars.Enable   = on;
                    if ([key isEqualToString:@"Box"])      Vars.Box      = on;
                    if ([key isEqualToString:@"Lines"])    Vars.lines    = on;
                    if ([key isEqualToString:@"Name"])     Vars.Name     = on;
                    if ([key isEqualToString:@"Health"])   Vars.Health   = on;
                    if ([key isEqualToString:@"Distance"]) Vars.Distance = on;
                    if ([key isEqualToString:@"Skeleton"]) Vars.skeleton = on;
                };
                [_scrollView addSubview:row];
                y += rowH + gap;
            }
            break;
        }

        case MenuTabMisc: {
            _tabTitleLabel.text = @"MISC";
            _tabIconView.image = ZSFImage(@"cube.box", 14);

            // Legend for the rows that still need an offset the dump does not have.
            UILabel *legend = [[UILabel alloc] initWithFrame:CGRectMake(px, y, rw, 34)];
            legend.numberOfLines = 0;
            legend.text = @"* needs an offset the dump does not carry yet";
            legend.textColor = COLOR_SUBTEXT;
            legend.font = [UIFont systemFontOfSize:11];
            [_scrollView addSubview:legend];
            y += 38 + gap;

            NSArray *rows = @[
                @[@"No Recoil",        @"NoRecoil"],
                @[@"Fast Fire",        @"FastFire"],
                @[@"Fast Swap Weapon", @"FastSwap"],
                @[@"Fast Reload",      @"FastReload"],
                @[@"No Reload",        @"NoReload"],
                @[@"Unlimited Ammo",   @"UnlimitedAmmo"],
                @[@"Force 120 FPS",    @"Force120FPS"],
                @[@"Fast Parachute *", @"FastParachute"],
                @[@"Teleport (8m) *",  @"Teleport"],
                @[@"Screen Stretch *", @"ScreenStretch"],
                @[@"Spin Player *",    @"SpinPlayer"],
            ];
            for (NSArray *r in rows) {
                BOOL st = [[Msa7 shared] boolFor:r[1]];
                ZToggleRow *row = [[ZToggleRow alloc] initWithTitle:r[0] isOn:st];
                row.frame = CGRectMake(px, y, rw, rowH);
                NSString *key = r[1];
                row.onToggle = ^(BOOL on) {
                    [[Msa7 shared] setBool:on for:key];
                    if ([key isEqualToString:@"NoRecoil"])      Vars.NoRecoil = on;
                    if ([key isEqualToString:@"FastFire"])      Vars.FastFire = on;
                    if ([key isEqualToString:@"FastSwap"])      Vars.FastSwap = on;
                    if ([key isEqualToString:@"FastReload"])    Vars.FastReload = on;
                    if ([key isEqualToString:@"NoReload"])      Vars.NoReload = on;
                    if ([key isEqualToString:@"UnlimitedAmmo"]) Vars.UnlimitedAmmo = on;
                    if ([key isEqualToString:@"Force120FPS"])   Vars.Force120FPS = on;
                };
                [_scrollView addSubview:row];
                y += rowH + gap;
            }
            break;
        }

        case MenuTabSettings: {
            _tabTitleLabel.text = @"SETTINGS";
            _tabIconView.image = ZSFImage(@"gearshape", 14);
            NSArray *rows = @[
                @[@"Anti Screenshot", @"AntiSS"],
                @[@"Stealth Mode",    @"Stealth"],
            ];
            for (NSArray *r in rows) {
                BOOL st = [[Msa7 shared] boolFor:r[1]];
                ZToggleRow *row = [[ZToggleRow alloc] initWithTitle:r[0] isOn:st];
                row.frame = CGRectMake(px, y, rw, rowH);
                NSString *key = r[1];
                row.onToggle = ^(BOOL on) {
                    [[Msa7 shared] setBool:on for:key];
                };
                [_scrollView addSubview:row];
                y += rowH + gap;
            }
            break;
        }

        case MenuTabProfile: {
            _tabTitleLabel.text = @"PROFILE";
            _tabIconView.image = ZSFImage(@"person", 14);
            UILabel *name = [[UILabel alloc] initWithFrame:CGRectMake(px, y, rw, 40)];
            name.text = @"MONA X YUE ENN";
            name.textColor = COLOR_ACCENT;
            name.font = [UIFont systemFontOfSize:16 weight:UIFontWeightBold];
            name.textAlignment = NSTextAlignmentCenter;
            [_scrollView addSubview:name];
            y += 44;
            UILabel *ver = [[UILabel alloc] initWithFrame:CGRectMake(px, y, rw, 24)];
            ver.text = @"PROJECT FREE";
            ver.textColor = COLOR_SUBTEXT;
            ver.font = [UIFont systemFontOfSize:12];
            ver.textAlignment = NSTextAlignmentCenter;
            [_scrollView addSubview:ver];
            y += 28;
            break;
        }
    }

    _scrollView.contentSize = CGSizeMake(_panelView.bounds.size.width, y + 16);
}


- (void)sidebarTapped:(ZSidebarButton *)btn {
    for (ZSidebarButton *b in _sidebarButtons) [b setActive:NO];
    [btn setActive:YES];
    [self loadTab:btn.tab];
}

- (void)handlePan:(UIPanGestureRecognizer *)pan {
    CGPoint t = [pan translationInView:mainWindow];
    if (pan.state == UIGestureRecognizerStateBegan) _lastPoint = _menuView.center;
    _menuView.center = CGPointMake(_lastPoint.x + t.x, _lastPoint.y + t.y);
}

// The renderer layer is a CAMetalLayer on Metal builds and a CAEAGLLayer on
// the older OpenGL builds. CALayer itself has no frame rate property, so the
// tree has to be walked down to the layer that draws the game.
static CALayer *FindRenderLayer(CALayer *layer) {
    if (!layer) return nil;
    if ([layer isKindOfClass:CAMetalLayer.class]) return layer;
    if ([layer respondsToSelector:@selector(preferredFramesPerSecond)]) return layer;
    for (CALayer *sub in layer.sublayers) {
        CALayer *found = FindRenderLayer(sub);
        if (found) return found;
    }
    return nil;
}

- (void)updateMenu {
    _menuView.hidden = !MenDeal;

    // No offset needed: the renderer is told to go faster. The game resets
    // this on its own, so it is pushed every tick.
    if (Vars.Force120FPS && mainWindow) {
        CALayer *render = FindRenderLayer(mainWindow.layer);
        if ([render isKindOfClass:CAMetalLayer.class]) {
            if (@available(iOS 15.0, *)) {
                ((CAMetalLayer *)render).preferredFrameRateRange = CAFrameRateRangeMake(60, 120, 120);
            }
        } else if (render) {
            [render setValue:@120 forKey:@"preferredFramesPerSecond"];
        }
    }

    get_players();
}

- (void)closeMenu { MenDeal = false; }

- (void)initTapGes {
    UITapGestureRecognizer *tap1 = [[UITapGestureRecognizer alloc]
        initWithTarget:self action:@selector(openMenu)];
    tap1.numberOfTapsRequired = 2; tap1.numberOfTouchesRequired = 3;
    [mainWindow addGestureRecognizer:tap1];
    UITapGestureRecognizer *tap2 = [[UITapGestureRecognizer alloc]
        initWithTarget:self action:@selector(closeMenu)];
    tap2.numberOfTapsRequired = 2; tap2.numberOfTouchesRequired = 2;
    [mainWindow addGestureRecognizer:tap2];
}

- (void)openMenu {
    if (!_menuView) [self setupMenu];
    MenDeal = true;
}

@end


void game_sdk_t::init() {
    this->GetHp = (int (*)(void *))getRealOffset(0x4A8478C);
    this->Curent_Match = (void *(*)())getRealOffset(0x4E355B0);
    this->GetLocalPlayer = (void *(*)(void *))getRealOffset(0x28FC854);
    this->GetHeadPositions = (void *(*)(void *))getRealOffset(0x4AA1A28);
    this->get_position = (Vector3(*)(void *))getRealOffset(0x8552BAC);
    this->Component_GetTransform = (void *(*)(void *))getRealOffset(0x854060C);
    this->get_camera = (void *(*)())getRealOffset(0x84E7148);
    this->WorldToScreenPoint = (Vector3(*)(void *, Vector3))getRealOffset(0x84E6AC8);
    this->WorldToScreenPointEx = (Vector3(*)(void *, Vector3, int))getRealOffset(0x84E6AC8);
    this->get_isVisible = (bool (*)(void *))getRealOffset(0x4A20AF4);
    this->get_isLocalTeam = (bool (*)(void *))getRealOffset(0x4A38D90);
    this->get_IsDieing = (bool (*)(void *))getRealOffset(0x4A02EA8);
    this->get_MaxHP = (int (*)(void *))getRealOffset(0x4A8489C);
    this->GetForward = (Vector3(*)(void *))getRealOffset(0x85534CC);
    this->set_aim = (void (*)(void *, Quaternion))getRealOffset(0x4A1C91C);
    this->get_IsSighting = (bool (*)(void *))getRealOffset(0x4A0FF18);
    this->get_IsFiring = (bool (*)(void *))getRealOffset(0x4A05634);
    this->name = (monoString * (*)(void *player))getRealOffset(0x4A16D38);
    this->_GetHeadPositions = (void *(*)(void *))getRealOffset(0x4AA1A28);
    this->_newHipMods = (void *(*)(void *))getRealOffset(0x4AA1BD8);
    this->_GetLeftAnkleTF = (void *(*)(void *))getRealOffset(0x4AA2028);
    this->_GetRightAnkleTF = (void *(*)(void *))getRealOffset(0x4AA2134);
    this->_GetLeftToeTF = (void *(*)(void *))getRealOffset(0x4AA2240);
    this->_GetRightToeTF = (void *(*)(void *))getRealOffset(0x4AA234C);
    this->_getLeftHandTF = (void *(*)(void *))getRealOffset(0x4A1B9B4);
    this->_getRightHandTF = (void *(*)(void *))getRealOffset(0x4A1BAB8);
    this->_getLeftForeArmTF = (void *(*)(void *))getRealOffset(0x4A1BBBC);
    this->_getRightForeArmTF = (void *(*)(void *))getRealOffset(0x4A1BCC0);
}
