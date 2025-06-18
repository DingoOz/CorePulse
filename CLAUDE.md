# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CorePulse is an open-source mech combat game inspired by classic 1990s titles, built with modern C++20/C++23 and OpenGL 4.3+. The project is in early planning stages with comprehensive technical documentation but no implemented code yet.

## Key Technologies

- **Language:** C++20/C++23 with modern features (concepts, ranges, coroutines)
- **Graphics:** OpenGL 4.3+ with deferred rendering pipeline
- **Asset Format:** glTF 2.0 with custom CorePulse extensions
- **Audio:** SDL2 with custom 3D spatial audio
- **Physics:** Custom rigid body system optimized for mech combat
- **Build System:** CMake 3.20+ with vcpkg for dependencies
- **Architecture:** Entity Component System (ECS) using modern C++ patterns

## Architecture Highlights

### Core Engine Structure
- **Platform Abstraction:** SDL2 fopost-processing
- **Asset Pipeline:** Blender → glTF 2.0 → Custom loader with game-specific extensions
- **Memory Management:** Custom allocators using C++20 concepts
- **Threading:** std::thread and std::async for multi-threaded systems

### Game-Specific Systems
- **Mech Simulation:** Heat management, component-based damage, hardpoint weapons
- **Mission System:** Lua scripting for objectives, dynamic mission loading
- **Combat Mechanics:** Ballistic prediction, electronic warfare, damage modeling
- **Customization:** MechLab interface with tonnage limits and critical slots

### glTF Extensions
CorePulse extends glTF 2.0 with custom extensions:
- `CP_walker_hardpoints`: Weapon mounting points with tonnage/slot constraints
- `CP_damage_zones`: Component-based damage modeling data
- `CP_mission_data`: Mission objectives and spawn points
- `CP_physics_properties`: Rigid body physics parameters

## Development Commands

*Note: This project is in planning phase - actual build commands will be added when implementation begins*

Expected commands based on project plan:
- Build: `cmake --build build --config Release`
- Tests: `ctest --test-dir build`
- Asset validation: `corepulse_validate --recursive assets/`

## Key Design Principles

1. **Authentic Mech Combat:** Realistic heat management, component damage, and tactical gameplay
2. **Modern C++ Standards:** Leverage C++20/23 features for performance and safety
3. **Cross-Platform:** Linux and Windows support from day one
4. **Extensible Architecture:** Plugin-friendly ECS design for modding
5. **Industry Standards:** Use glTF 2.0 for maximum tool compatibility

## Asset Organization

```
assets/
├── models/walkers/[class]/    # Light, medium, heavy mech models
├── models/weapons/[type]/     # Energy, ballistic, missile weapons
├── missions/                  # glTF scenes with mission data
└── textures/                  # PBR texture assets
```

## Code Architecture Notes

- **ECS Implementation:** Modern C++20 with concepts for type safety
- **Shader Management:** OpenGL 4.3+ compute shaders for particle effects
- **Asset Loading:** Memory-mapped files for large .glb assets
- **Animation System:** glTF skeletal animation with custom mech-specific behaviors
- **Physics Integration:** Custom collision detection optimized for large rigid bodies

The project emphasizes creating an authentic giant robot combat experience while maintaining modern development practices and extensive modding support.

If you are blocked from install an apt package, ask the user to install it.