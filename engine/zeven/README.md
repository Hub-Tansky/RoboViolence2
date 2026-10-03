# zeven (DukZeven)

Engine utilities behind small C APIs with a `dk` prefix.

| Module | Role | Target |
|---|---|---|
| `dkc` | fixed-step timer (`std::chrono`, catch-up capped at 5 steps) | `zeven_core` |
| `dksvar`, `CSystemVariable` | config variable registry (`sv_*`, `cl_*`), `set` command | `zeven_core` |
| `CString`, `CVector`, `CMatrix` | string and math types shared by the whole project | `zeven_core` |
| `dkt_console.cpp` | no-op `dkt` for the headless server | `zeven_console` |
| `dkw` | SDL3 window, GL 2.1 compatibility context host, event pump | `zeven_client` |
| `dki` | keyboard (`DIK_*` IDs, `dikeys.h`), mouse and gamepad state | `zeven_client` |
| `dkgl`, `dkt`, `dkf`/`CFont`, `dkp`/`CParticle` | GL state, textures, fonts, particles | `zeven_client` |
| `dks` | miniaudio effects and music | `zeven_client` |

- **Headers:** `include/` (public), `src/*i.h` (module internals). `platform.h` is force-included by CMake. `glheaders.h` is the only place that includes OpenGL (glad from `third_party/glad`, then GLU).
- **Depends on:** pthreads, glad; the client adds SDL3, miniaudio and (for OGG) stb_vorbis. GLU is the system library for now.
- **Gotchas:** `systemVariable` is built on first use (`dksvarRegistry()`) because the game registers variables from a global constructor. `dkwGetResolution()` is the drawable size in pixels (HiDPI-aware): use it for `glViewport`. Key binds in `bv2.cfg` are the old Windows keyboard scancodes (`dikeys.h`); `dkw` translates SDL scancodes. See [../../docs/analysis/01_SYSTEM_OVERVIEW.md](../../docs/analysis/01_SYSTEM_OVERVIEW.md) and [ADR 0006](../../docs/decisions/0006-sdl3-miniaudio-glad-platform-layer.md).
