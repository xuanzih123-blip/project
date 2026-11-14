# Building mona

`mona` is a Theos tweak for jailbroken iOS. It builds on macOS only (arm64
iOS target), so the normal path is the GitHub Actions workflow, which runs on a
macOS runner.

![Build Tweak](https://github.com/xuanzih123-blip/project/actions/workflows/objective-c-xcode.yml/badge.svg)

---

## 1. Build on GitHub Actions (the normal path)

The workflow file is `.github/workflows/objective-c-xcode.yml`. It runs on
every push to `main` / `master`, on every `v*` tag, and on demand.

```bash
git add -A
git commit -m "your message"
git push origin main
```

That is the whole build. No `gh` CLI, no runner on the local machine.

Then either:

- **Download from the web**
  <https://github.com/xuanzih123-blip/project/actions> → pick the run →
  **Artifacts → mona** → downloads a zip containing the `.deb`.

- **Download from a shell**

  ```bash
  curl -L -H "Authorization: token $GITHUB_TOKEN" \
    https://api.github.com/repos/xuanzih123-blip/project/actions/artifacts/<artifact_id>/zip \
    -o mona.zip
  unzip mona.zip
  ```

- **Run it by hand** from the Actions tab → **Build Tweak** → **Run workflow**.

The artifact zip contains:

```
com.yue.com_1.0_iphoneos-arm.deb
```

A second artifact, `mona-dylib`, contains the bare dylib and the filter
plist, ready to drop into a device or to inject with Frida.

### What the workflow does

| Step | What happens |
|---|---|
| `actions/checkout@v4` | pulls the source |
| `actions/cache@v4` | caches `${{ github.workspace }}/../theos` so Theos is only cloned once |
| `Install Theos` | clones Theos if it is not there yet, puts `$THEOS` and `$THEOS/bin` on the path, makes sure `ldid` is installed (Theos needs it to sign) |
| `Build Tweak` | `make clean` then `make package` |
| `Upload deb artifact` | attaches `packages/*.deb` as the artifact `mona` |
| `Stage the dylib bundle` | copies `mona.dylib` out of `.theos/obj` and puts `mona.plist` next to it |
| `Sign and inspect the dylib` | ad hoc signs with `ldid`, then prints `lipo`, `file` and the signature load command |
| `Smoke test the dylib` | checks the plist really names the game bundle and that the symbols are in the binary |
| `Upload the dylib bundle` | attaches `build/install` as the artifact `mona-dylib` |

---

## 1b. Building as a bare dylib

**Yes, it works, and the workflow already does it.** The `.deb` is nothing but a
wrapper: Theos builds `mona.dylib` and the deb just packages it together with
the filter plist and the control metadata. The `mona-dylib` artifact is that
same dylib with the plist next to it:

```
build/install/mona.dylib
build/install/mona.plist
```

A dylib on its own needs two things to be usable:

1. **The filter plist, under the same name.** The loader matches `X.dylib`
   against `X.plist` to decide which bundle to inject into. `mona.plist` has
   to stay named `mona.plist`, otherwise it does not load into the game.
2. **A signature.** `ldid -S` signs it ad hoc, which is enough on a jailbroken
   device.

### Installing it by hand

```bash
# rootless (Dopamine and friends)
scp build/install/mona.dylib root@<device>:/var/jb/Library/MobileSubstrate/DynamicLibraries/
scp build/install/mona.plist root@<device>:/var/jb/Library/MobileSubstrate/DynamicLibraries/

# rootful
scp build/install/mona.dylib root@<device>:/var/mobile/Library/MobileSubstrate/DynamicLibraries/
scp build/install/mona.plist root@<device>:/var/mobile/Library/MobileSubstrate/DynamicLibraries/

# then kill the game so the loader picks it up on the next launch
```

### Injecting it without the jailbreak loader

Frida loads the dylib directly and does not need the plist at all:

```bash
frida -U -f com.garena.game.kgvn -l build/install/mona.dylib
```

The tweak's `+load` still fires, so the menu opens the same way.

### What does not change

The dylib and the deb contain the same code. The only difference is who puts
the files where: the deb does it for you (and the package shows up in the
package manager), a bare dylib means copying two files by hand.

---

## 2. Build locally on a Mac

Requirements:

- macOS with the Xcode command line tools (`xcode-select --install`)
- [Theos](https://theos.dev) at `$THEOS`
- `ldid` (`brew install ldid`)

```bash
export THEOS=$HOME/theos
git clone --recursive https://github.com/theos/theos.git "$THEOS"
brew install ldid

cd /path/to/this/project
make clean
make package        # -> packages/com.yue.com_1.0_iphoneos-arm.deb
```

`Make.sh` is the one-liner version:

```bash
./Make.sh            # make package install
```

`install` pushes the package to the device over SSH, which needs
`THEOS_DEVICE_IP` (or `THEOS_DEVICE`) set:

```bash
export THEOS_DEVICE_IP=192.168.1.50
./Make.sh
```

The tweak lands in:

```
/var/jb/Library/MobileSubstrate/DynamicLibraries/mona.dylib   (rootless)
/var/mobile/Library/MobileSubstrate/DynamicLibraries/mona.dylib (rootful)
```

and the filter plist next to it:

```
mona.plist   ->   Filter / Bundles / com.garena.game.kgvn
```

---

## 3. What the Makefile is doing

```make
export THEOS ?= $(THEOS)

ARCHS = arm64
TARGET = iphone:clang

DEBUG = 0
FINALPACKAGE = 1
FOR_RELEASE = 1

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = mona

mona_FILES = Menu.mm

mona_FRAMEWORKS = UIKit Foundation QuartzCore CoreGraphics AudioToolbox

mona_CCFLAGS = -std=c++17 \
    -fno-rtti \
    -fno-exceptions \
    -DNDEBUG \
    -O2 \
    -IIncludes

mona_CFLAGS = \
    -fobjc-arc \
    -Wno-deprecated-declarations \
    -Wno-unused-variable \
    -Wno-unused-value \
    -Wno-incomplete-umbrella \
    -IIncludes

mona_LIBRARIES = substrate

include $(THEOS_MAKE_PATH)/tweak.mk
```

| Line | Why |
|---|---|
| `TWEAK_NAME = mona` | the dylib is named `mona.dylib`. **Theos only reads `mona_*` variables.** A flag block named after anything else is silently ignored |
| `mona_FILES = Menu.mm` | the only translation unit. Everything in `Includes/` is a header and is pulled in by `#import` |
| `mona_CFLAGS` | `-fobjc-arc` turns on ARC for the whole tweak. Without it every `alloc` leaks, and `[[NSString alloc] init...]` returned from a C++ helper hands the caller a pointer ARC already released |
| `mona_LIBRARIES = substrate` | links `-lsubstrate` for `MSHookFunction` and friends |
| `-IIncludes` | the header search path. Without it `#import "Offsets.h"` inside `Includes/ESP.h` does not resolve |
| `FOR_RELEASE = 1` | release build, `NDEBUG`, no debug symbols |

### Header layout

`Menu.mm` is the only `.mm` file. The headers are:

```
Includes/Offsets.h     every offset in the game, one named block
Includes/ESP.h         Vars_t, game_sdk_t, world to screen, the ESP renderer
Includes/Aimbot.h      target selection, aim modes, the field write path
Includes/HumanAim.h    reaction delay, minimum jerk, overshoot, tremor, lead
Includes/GameMods.h    offset backed mods (no recoil, fast fire, no reload)
Includes/Prefs.h       settings storage outside the game's container, class Msa7
Includes/MemoryUtils.h dyld base address, vm patchers
Includes/UnityTypes.h  monoArray, monoList, monoString
Includes/Vector3.h Vector2.h Quaternion.h   Unity math ports
```

Include order matters: `ESP.h` imports `Offsets.h` first, then `Aimbot.h`
(which imports `HumanAim.h`) and `GameMods.h` near the bottom, after `Vars_t`
is defined.

---

## 4. Traps that already bit this build

Keep these in mind before changing the workflow or the Makefile.

**`nic.pl` is not part of a build.** It is Theos's interactive *project
generator*. It asks for a project name and a template, and with no stdin it
exits 1 and kills the job. Do not put it in the workflow.

**Theos clone path and cache path must be the same directory.** The cache
key says `${{ github.workspace }}/../theos`; if the clone goes to `$HOME/theos`
the cache never hits and every run pays for a fresh clone.

**`brew install ldid` fails if ldid is already there.** Use
`brew list ldid >/dev/null 2>&1 || brew install ldid`.

**`preferredFrameRateRange` is iOS 15 and later.** The SDK Theos ships is
older and does not declare it, so `CAMetalLayer.preferredFrameRateRange` does
not compile. The frame rate is raised through KVC on `UIScreen` instead.

**Do not spell out a function that returns a function pointer inline.**

```cpp
static inline void *(*PlayerListFn(void))(void)   // clang parses this wrong
static inline player_list_fn_t PlayerListFn(void) // this is the one that works
```

clang reads the first form as "function returning a pointer to a zero argument
function", and every call site then fails with a parameter count mismatch.

---

## 5. Reading a failed CI run

```bash
gh run list --repo xuanzih123-blip/project
gh run view <run_id> --repo xuanzih123-blip/project --log-failed
```

Or the web: the run page → the failing step → **Run logs**.

The first error is usually the real one; the rest are often cascades from it.
