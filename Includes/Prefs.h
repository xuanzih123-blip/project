#pragma once
#import <Foundation/Foundation.h>

// Opaque class name on purpose: every Obj-C class in this process is visible
// to objc_getClassList() from inside the game. "Prefs" would be a label.
@interface Msa7 : NSObject
+ (instancetype)shared;
- (BOOL)boolFor:(NSString *)key;
- (BOOL)boolFor:(NSString *)key defaultValue:(BOOL)fallback;
- (void)setBool:(BOOL)value for:(NSString *)key;
- (float)floatFor:(NSString *)key;
- (float)floatFor:(NSString *)key defaultValue:(float)fallback;
- (void)setFloat:(float)value for:(NSString *)key;
@end

@implementation Msa7

static NSMutableDictionary *g_store;
static NSString *g_path;

// Outside the game container on purpose. standardUserDefaults would write
// "Aimbot" / "Teleport" into the game's own preferences plist.
static NSString *PrefsPath(void) {
    return @"/var/mobile/Library/Preferences/.yue.plist";
}

// Keys are hashed so the file does not spell out what it holds.
static NSString *ObfuscateKey(NSString *key) {
    return [NSString stringWithFormat:@"k_%lu", (unsigned long)[key hash]];
}

+ (instancetype)shared {
    static Msa7 *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
        g_path = PrefsPath();

        NSData *data = [NSData dataWithContentsOfFile:g_path];
        g_store = [NSMutableDictionary dictionary];
        if (data) {
            id plist = [NSPropertyListSerialization propertyListWithData:data
                                                                options:NSPropertyListMutableContainers
                                                                 format:NULL
                                                                  error:NULL];
            if ([plist isKindOfClass:NSDictionary.class]) {
                [g_store addEntriesFromDictionary:plist];
            }
        }
    });
    return instance;
}

- (void)flush {
    NSData *data = [NSPropertyListSerialization dataWithPropertyList:g_store
                                                               format:NSPropertyListBinaryFormat_v1_0
                                                              options:0
                                                                error:NULL];
    if (data) [data writeToFile:g_path atomically:YES];
}

- (BOOL)boolFor:(NSString *)key {
    return [self boolFor:key defaultValue:NO];
}

- (BOOL)boolFor:(NSString *)key defaultValue:(BOOL)fallback {
    id value = g_store[ObfuscateKey(key)];
    if (![value isKindOfClass:NSNumber.class]) return fallback;
    return [value boolValue];
}

- (void)setBool:(BOOL)value for:(NSString *)key {
    g_store[ObfuscateKey(key)] = @(value);
    [self flush];
}

- (float)floatFor:(NSString *)key {
    return [self floatFor:key defaultValue:0.0f];
}

- (float)floatFor:(NSString *)key defaultValue:(float)fallback {
    id value = g_store[ObfuscateKey(key)];
    if (![value isKindOfClass:NSNumber.class]) return fallback;
    return [value floatValue];
}

- (void)setFloat:(float)value for:(NSString *)key {
    g_store[ObfuscateKey(key)] = @(value);
    [self flush];
}

@end
