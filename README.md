# Software 3D Engine

This Project is a 2D and 3D software renderer written in C++ using the CPU. 

![alt text](https://github.com/xPasquale1/Software-3D-Engine/blob/main/images/ref.png "G-Buffers")

# Features

Font rendering using .ttf
.obj und .png support
Simple UI Elements like Buttons
3D capabilities
G-Buffers and Wireframe rendering
Perspective corrected texturing
Dynamic sun shadows (wip)
Screen space contact shadows (SSCS)
Screen space reflections (SSR, only mirror like reflections)
World space reflections (WSR, uses SDF as fallback if SSR fails)
Screen space ambient occlusion (SSAO)
Screen space global illumination (SSGI)
Screen Space Radiance Caching (SSRC) (wip)
Signed distance field generation

# Installation

Written with C++ for Windows. Tested only with the GCC compiler.
```
-O3 (recommended)
-lgdi32
-lComdlg32
-ld2d1
```
