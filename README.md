CubeOpenGL Engine

Overview

This repository contains a small OpenGL-based rendering engine and demo application (CubeOpenGL). The engine provides a lightweight framework for loading meshes, materials and environments and rendering them with modern OpenGL techniques. It includes multiple environmental systems (ocean, volumetric clouds), PBR material support and an application shell to initialize and run the renderer.

Key features

- Modern OpenGL renderer (C++14)
- Mesh loading via Assimp (GLTF/FBX support)
- Material system with PBR and advanced material presets
- Multiple environment systems:
  - Gerstner-style ocean and an FFT/Tessendorf ocean implementation
  - Volumetric clouds and a book-style clouds implementation
- Camera, transforms and scene management utilities
- Shader-based rendering pipeline with support for reflection, Fresnel and simple lighting

Why this repo

This project is intended as a learning / prototype engine to experiment with real-time rendering techniques: PBR, environment simulation (ocean/clouds), mesh loading, and shader pipelines. It is organized for extension and experimentation rather than production use.

Quick start (Windows)

Requirements
- Visual Studio (2019/2022) with C++ toolset
- vcpkg for dependencies OR preinstalled libraries
- Supported external libraries: GLFW, GLEW (or GLAD), GLM, Assimp, stb_image (or similar image loader)

Build steps
1. Install required libraries (recommend using vcpkg):
   - assimp, glm, glew, glfw3, stb
2. Open the solution in Visual Studio and set the include/lib paths (or integrate vcpkg)
3. Build the solution and run the executable

Run
- The application entry point is main.cpp which calls Engine::Init(); the app creates the Window, OpenGL context and starts the rendering loop. Assets (models, textures, shaders) are expected under the repository's assets/shaders/textures folders referenced in code.

Project layout (high level)
- src/
  - main.cpp            -- app entry (Engine::Init / Engine::Release)
  - App.cpp / App.hpp   -- core application, scene setup, asset loading
  - WindowWin.*         -- Windows GLFW/Win32 window and input handling
  - OpenGL.*            -- rendering, shader and frame timing utilities
  - Mesh.*, Material.*  -- mesh loading, material and texture management
  - Ocean*, Clouds*     -- environmental systems (Gerstner, FFT, volumetric clouds)
  - shaders/            -- GLSL shader files used by the renderer
  - textures/           -- example textures used by materials

Assets
- Place models (.glb, .fbx, .obj) and textures in the assets folder (paths used in App.cpp). The engine logs what textures and materials are loaded and will fall back to simple defaults if assets are missing.

Extending the engine
- Add shaders to the shaders/ folder and reference them from material initializers
- Add or modify mesh/material presets in App.cpp
- Implement new environment systems by following existing Ocean/Cloud classes

Contributing
- Fork, change, and open a pull request. Keep changes small and focused. Include build/test instructions for any new dependency.

License
- This project does not include a license file. Add an appropriate open-source license if you plan to publish the code.

Contact
- See code comments and file headers for author/maintainer info if present in source files.
