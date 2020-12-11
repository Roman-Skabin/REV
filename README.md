# REV

### Am working on right now:
* 3D Batch renderer
* Redesigning vec and (probably) mat types because I can't align my vertex struct to 1 byte so I can't "pack" them to a vertex buffer correctly. (surprisingly #pragma pack(1) and __declspec(align(1)) didn't help)
* Scene management

---

### Future improvements:
* [see here](TODO.txt)

---

### Already works:
* Not that much you can use to create a game. Was (and still) working a lot on internal stuff:
  * GPU memory managment
  * A lot of optimizations for working with CPU memory (permanent and per-frame memory, custom allocators, stretchy buffers, etc.)
  * Data layout improvements for working with cache
  * Math
  * Data orientied design (I try at least)
  * Work queue for CPU
  * \+ the biggest problem that has slowed down all the productivity: the engine has been rewritten in C++ beacause DirectX 12 doesn't support C.
