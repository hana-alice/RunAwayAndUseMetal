# RunAwayAndUseMetal
https://vulkan-tutorial.com/Introduction

```
git submodule update --init --recursive
cd vcpkg
.\bootstrap-vcpkg.bat
```
using Visual Studio or Clion open this folder direclty, IDEs take care of the next step.
For Clion, you may need to choose cmake preset manually at first launch.


### What is going on:
1. Seperate rendering thread for Editor
2. Contact shadow
3. SSAO - GTAO
4. Soft shadow

TODO:
 - [ ] InputSystem
 - [ ] std::container -> pmr
 - [ ] ECS
 - [ ] Mesh Shader
 - [ ] RayTracing pass
 - [ ] Multi Device Queue
 - [ ] too long to write down

