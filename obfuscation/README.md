# OLLVM / Hikari obfuscation pipeline (optional)

The default build already produces a hardened, stripped `libcsnative.so` with
hidden visibility, full RELRO, JNI-only export table, anti-debug, anti-tamper,
and compile-time XOR string obfuscation (`OBF("...")`). That is enough for most
threat models.

If you want **stronger** native-code obfuscation (control-flow flattening,
bogus control flow, instruction substitution, etc.), wire in a custom Clang
built from an OLLVM/Hikari fork and rebuild with:

```bash
./gradlew :app:assembleRelease \
  -Pandroid.native.cmakeArguments=-DUSE_OLLVM=ON
```

When `USE_OLLVM=ON`, `app/src/main/jni/CMakeLists.txt` appends:

```
-mllvm -fla              # control-flow flattening
-mllvm -bcf              # bogus control flow
-mllvm -bcf_loop=2
-mllvm -bcf_prob=40
-mllvm -sub              # instruction substitution
-mllvm -sub_loop=2
-mllvm -sobf             # string obfuscation
-mllvm -split            # basic-block splitting
-mllvm -split_num=3
```

## Setting up an OLLVM toolchain (once)

The Android NDK ships its own Clang. To use OLLVM you replace it with a
custom build that includes the obfuscation passes.

1. Pick a maintained OLLVM fork. Recent options:
   - https://github.com/HikariObfuscator/Hikari (LLVM 8-based)
   - https://github.com/heroims/obfuscator/tree/llvm-12.0.1 (LLVM 12-based, supports newer Android NDK targets)

2. Build it against the same LLVM version your target NDK uses (`$NDK/toolchains/llvm/prebuilt/linux-x86_64/lib64/clang/<ver>` shows the version).

3. Replace the NDK's `clang`, `clang++`, and `lld` in
   `$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/` with symlinks (or
   wrappers) to your OLLVM build, keeping the existing target-prefixed wrappers
   (`aarch64-none-linux-android24-clang` etc.).

4. Sanity check: the wrapper should accept `-mllvm -fla` without errors.

5. Run the build with `-DUSE_OLLVM=ON` as above.

## Manual fallback (no OLLVM)

If your toolchain doesn't support OLLVM you still get:
- Compile-time XOR string obfuscation via the `OBF()` macro.
- Hidden visibility, dead-code stripping, full strip, BIND_NOW.
- Anti-debug + anti-tamper.
- R8 / ProGuard with aggressive shrinking, repackaging, and access modification.

That's the production baseline this repo ships.
