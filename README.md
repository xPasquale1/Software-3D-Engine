# Software 3D Engine

This Project is a 2D and 3D software renderer written in C++ using the CPU. 

![alt text](https://github.com/xPasquale1/Software-3D-Engine/blob/main/images/ref.png "G-Buffers")

# Features

Font rendering using .ttf <br>
.obj und .png support <br>
Simple UI Elements like Buttons <br>
3D capabilities <br>
G-Buffers and Wireframe rendering <br>
Perspective corrected texturing <br>
Dynamic sun shadows (wip) <br>
Screen space contact shadows (SSCS) <br>
Screen space reflections (SSR, only mirror like reflections) <br>
World space reflections (WSR, uses SDF as fallback if SSR fails) <br>
Screen space ambient occlusion (SSAO) <br>
Screen space global illumination (SSGI) <br>
Screen Space Radiance Caching (SSRC) (wip) <br>
Signed distance field generation <br>

# Installation

Written with C++ for Windows. Tested only with the GCC compiler.
```
-O3 (recommended)
-lgdi32
-lComdlg32
-ld2d1
```
