# Architecture

## Layers

```
   ┌────────────────────────────────────────────────┐
   │  android.app.Activity (GameActivity.java)      │
   │  GLSurfaceView + touch dispatch                │
   └───────────────┬────────────────────────────────┘
                   │  JNI (NativeBridge.java)
                   ▼
   ┌────────────────────────────────────────────────┐
   │  jni_bridge.cpp  (only file with EXPORT decls) │
   └───────────────┬────────────────────────────────┘
                   │
                   ▼
   ┌────────────────────────────────────────────────┐
   │   eng::App  ──────────────────────────────┐    │
   │   ├── Camera                              │    │
   │   ├── TouchInput                          │    │
   │   ├── Shader (lit, hud)                   │    │
   │   └── Mesh (unit_box, unit_quad)          │    │
   │                                           ▼    │
   │   cs::game::Game                                │
   │   ├── cs::game::Menu                            │
   │   └── cs::game::Match                           │
   │       ├── Map (walls, spawns, sites, nav)       │
   │       ├── std::vector<Player>  (idx 0 = local)  │
   │       ├── std::vector<BotBrain>                 │
   │       └── Bomb                                  │
   └────────────────────────────────────────────────┘

   ┌────────────────────────────────────────────────┐
   │   cs::hard::AntiDebug  (background thread)     │
   │   cs::hard::AntiTamper (background thread)     │
   └────────────────────────────────────────────────┘
```

## Round flow

```
   Menu  ── tap "START" ──►  Match.start(rounds)
                                 │
                                 ▼
                            ┌─────────┐  5s
                            │ Freeze  │◄────┐
                            └────┬────┘     │
                                 ▼          │
                            ┌─────────┐  ≤115s, ends on:
                            │  Live   │     • bomb explode  (T win)
                            └────┬────┘     • bomb defuse   (CT win)
                                 │          • side eliminated
                                 ▼          • timer expires (CT win, no plant)
                            ┌──────────┐ 5s
                            │ PostRound│
                            └────┬─────┘
                                 ▼
                  round == max/2  ?  ──►  Halftime ──► Freeze
                  match point     ?  ──►  MatchOver
                  else                 ──►  Freeze
```

## Threads

| Thread          | Owner                      | Notes |
| --------------- | -------------------------- | ----- |
| Main / UI       | Android                    | dispatches touch events into `TouchInput::on_touch` |
| GL render       | `GLSurfaceView`            | calls `App::on_draw_frame()` ~60Hz |
| `csn-aux`       | `AntiDebug`                | jittered 250-750ms TracerPid + maps + timing checks |
| `csn-vfy`       | `AntiTamper`               | 2s interval CRC32 of own .text |

## Where to add things next

- **Models**: drop a glTF loader into `engine/`, replace `Mesh::make_box` calls in `Match::render` with skinned meshes.
- **Audio**: OpenSL ES or AAudio under `engine/audio.{h,cpp}`. Hook footstep / weapon fire / bomb beep to existing events (apply_damage, fire, plant, fuse < 10s).
- **Better AI**: replace `nav_nodes()` waypoints with a navmesh; A* over edge graph.
- **Multiplayer**: add `net/` module with UDP sockets, snapshot interpolation, host migration; flip `Player::is_local` checks into "is locally controlled this peer".
- **OLLVM**: see [`obfuscation/README.md`](../obfuscation/README.md).
