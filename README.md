# REV

## Important:
* This is **NOT** an open source software. Yes, it's public, but it's just for a resume purposes.

---

## Am working on right now:
* Asset manager
* Textures
* Scene management

---

## Future improvements:
* [see here](TODO.txt)

---

## Already works:

Not that much you can use to create a game. Was (and still) working a lot on internal stuff:
* GPU memory management
* A lot of optimizations for working with CPU memory (permanent and per-frame memory, custom allocators, stretchy buffers, etc.)
* Data layout improvements for working with cache
* Math
* Data orientied design (I try at least)
* Work queue for CPU
* \+ the biggest problem that has slowed down all the productivity: the engine has been rewritten in C++ beacause DirectX 12 doesn't support C.

## But anyway, what we have now:
* CPU work queue
* D3D12 support (actually, the only API we currently support)
* First ideas of Scene Management (I sure it will be being cleaned up and will be being rewritten many times)
* GPU memory management: constant/vertex/index buffer memory
* CPU memory management: "global arenas" (permanent and per-frame), allocator
* 3D Batch Renderer
* A lot of utils: array, list, different strings, async file, OS event, loogger, timers, tuple

---

## How to build:
1. Open console in the build directory
2. Run compiler.bat
3. Run build.bat (see usage in [file](build/build.bat))
