# Aggressive shrinking: keep only what JNI absolutely needs.
-allowaccessmodification
-repackageclasses ''
-overloadaggressively
-optimizationpasses 5

# Keep the JNI entrypoints reachable from native code.
-keep class com.devin.cs.NativeBridge { *; }
-keep class com.devin.cs.GameActivity { *; }

# Strip logging in release.
-assumenosideeffects class android.util.Log {
    public static *** d(...);
    public static *** v(...);
    public static *** i(...);
}
