# zeven (DukZeven)

Engine utilities behind small C APIs with a `dk` prefix. Targets: `zeven_core` (all executables) and `zeven_client` (client only, excluded from `all` until Step 3).

| Module | Role | Target |
|---|---|---|
| `dkc` | high-resolution timer | core |
| `dksvar`, `CSystemVariable` | config variable registry (`sv_*`, `cl_*`), `set` command | core |
| `dkt` | TGA texture loading and binding (uses GL/GLU, also in the server) | core |
| `CString`, `CVector`, `CMatrix` | string and math types shared by the whole project | core |
| `dkw`, `dki`, `dkgl`, `dkf`/`CFont`, `dkp`/`CParticle`, `dks` | window, input, GL state, fonts, particles, sound | client |

- **Headers:** `include/` (public), `src/*i.h` (module internals). `include/platform.h` is force-included everywhere by CMake and defines `BV2_PLATFORM_*`, `BV2_POSIX`, `INT4`/`UINT4`.
- **Depends on:** pthreads, OpenGL (and GLU where present). The client modules need FMOD, DirectInput or SDL 1.2 until Step 3.
- **Gotchas:** `systemVariable` is built on first use (`dksvarRegistry()`); the game registers variables from a global constructor. Vendored `glext.h` stays until Step 3. See [../../docs/analysis/01_SYSTEM_OVERVIEW.md](../../docs/analysis/01_SYSTEM_OVERVIEW.md).
