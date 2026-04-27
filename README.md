# Strike Native (cs-android-native)

> **Status:** v0 / scaffold. Builds a runnable APK with a custom OpenGL ES 3 engine, touch FPS controls, AI bots, CS-style round flow (freezetime, randomized spawns, CT/T sides, bomb plant/defuse, halftime, MR12), and a layered native hardening pipeline (visibility hidden, full strip, hidden JNI symbols, anti-debug, anti-tamper text-section CRC, optional OLLVM/Hikari obfuscation passes).

This is an **offline single-player** Android FPS written in Android NDK / C++17 with a hand-rolled engine. No third-party game engine, no Unity, no Unreal — every triangle is yours.

## What's in v0

- **Custom 3D engine** (`app/src/main/jni/engine/`)
  - GLES3 renderer + tiny self-contained math library (no GLM)
  - Asset loader (AAssetManager-backed)
  - Shader / mesh / camera / two-thumb touch input
  - Frame loop driven by `GLSurfaceView`
- **Game** (`app/src/main/jni/game/`)
  - Boxy "Strike-Lite" map with two bombsites and waypoint nav
  - Player physics (axis-separated AABB sliding) + raycast hit detection
  - AI bots with goal-driven FSM (HoldSite / Push / Plant / Defuse / Engage / Reposition)
  - CS-style match: freezetime → live → post-round → halftime, MR12 (configurable)
  - Bomb mechanics: plant in site (3.2s), defuse (10s), 40s fuse
  - Main menu with mode select (Casual / Competitive / Practice)
  - HUD: score strip, phase bar, health/ammo bars, crosshair, virtual joystick + fire button
- **Hardening** (`app/src/main/jni/hardening/`)
  - Compile-time XOR string obfuscation (`OBF("...")`)
  - Multi-layer anti-debug (ptrace self-attach, TracerPid poller, /proc/self/maps watchlist for frida/gdbserver/lldb-server/xposed, timing canaries, jittered polling thread)
  - Anti-tamper: CRC32 of own `.text` segment captured at startup and re-verified periodically
  - JNI surface kept tiny on purpose (only the `Java_com_devin_cs_NativeBridge_*` entry points are exported)
- **Build hardening** (`app/build.gradle`, `CMakeLists.txt`)
  - `-fvisibility=hidden`, `-fvisibility-inlines-hidden`
  - `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections`
  - `-fno-rtti -fno-exceptions` (no class metadata in binary)
  - `-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`, full RELRO + BIND_NOW, noexecstack
  - `-Wl,--exclude-libs,ALL`, `-Wl,--build-id=none`, `--strip-all` + post-build strip pass
  - R8 with `minifyEnabled true`, `shrinkResources true`, `proguard-android-optimize`, `-allowaccessmodification`, `-repackageclasses ''`
  - ProGuard strips `android.util.Log.{d,v,i}` calls in release
- **Optional OLLVM/Hikari pipeline** (`obfuscation/`)
  - `cmake -DUSE_OLLVM=ON` enables `-mllvm -fla -bcf -sub -sobf -split` if you build the NDK clang against an OLLVM toolchain. See [`obfuscation/README.md`](obfuscation/README.md) for the build recipe (OLLVM is *not* required for an obfuscated/stripped baseline; the build above already produces a hardened .so).

## Honest limitations

This is a **starting foundation**, not a finished product.

- No textures, no models — everything is colored boxes. A model loader (glTF / OBJ) is the obvious next step.
- No audio.
- AI is goal-driven but uses a coarse waypoint graph, not full navmesh A*.
- No client-side prediction / lag comp (single-player only by design — LAN multiplayer was deferred).
- Touch controls work but lack customization (sensitivity slider, button repositioning).
- **"Unbreakable" is not a thing.** The hardening here raises reverse-engineering cost substantially (especially with OLLVM enabled), but every shipped Android binary can eventually be reversed by a determined attacker. Don't ship a license/DRM scheme that assumes the binary is opaque.

## Build

```bash
# Prereqs: JDK 17, Android SDK + NDK (r26+), CMake 3.22+
echo "sdk.dir=/path/to/android-sdk" > local.properties
./gradlew :app:assembleDebug      # debug APK -> app/build/outputs/apk/debug/
./gradlew :app:assembleRelease    # hardened release APK
```

Verify hardening on the resulting library:

```bash
unzip -p app/build/outputs/apk/release/app-release-unsigned.apk \
    lib/arm64-v8a/libcsnative.so > /tmp/cs.so
$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-nm \
    -D --defined-only /tmp/cs.so
# -> only 9 exported symbols: JNI_OnLoad + Java_com_devin_cs_NativeBridge_*
$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf -d /tmp/cs.so
# -> BIND_NOW, no build-id, etc.
```

## Layout

```
app/src/main/
├── AndroidManifest.xml
├── java/com/devin/cs/         # tiny Java host (Activity + JNI bridge)
├── jni/
│   ├── jni_bridge.cpp         # only TU with exported JNI symbols
│   ├── engine/                # custom GLES3 engine
│   ├── game/                  # match logic, AI, bomb, HUD, menu
│   └── hardening/             # antidebug, antitamper, OBF()
├── assets/shaders/            # GLSL ES 3.00
└── res/                       # icons + strings
```

## License

MIT
