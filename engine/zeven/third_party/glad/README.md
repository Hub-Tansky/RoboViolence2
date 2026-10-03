# glad

OpenGL 2.1 compatibility loader plus `GL_EXT_bgra`, generated with glad 2.0.8 (SPDX: WTFPL OR CC0-1.0, plus Apache-2.0 for `KHR/khrplatform.h`):

```
glad --api gl:compatibility=2.1 --extensions GL_EXT_bgra --out-path engine/zeven/third_party/glad c
```

Load with `gladLoadGL(SDL_GL_GetProcAddress)` after the context exists (`dkglCreateContext`). Include through `glheaders.h` only.
