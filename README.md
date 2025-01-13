# SimuSama - Material Point Method (MPM) Simulation for Blender

SimuSama is a high-performance physics simulation addon for Blender that implements the Material Point Method (MPM) for fluid and soft body simulation. The core simulation engine is written in C++ for performance, with Python bindings to integrate seamlessly into Blender's interface.

## Overview

The addon combines several technologies:

- C++ core simulation engine using MPM
- pybind11 for Python/C++ bindings
- Blender Python API for UI and visualization
- GPU-accelerated rendering for real-time particle visualization

## How It Works

### 1. Core Simulation (C++)

The MPM simulation works by combining Lagrangian particles with an Eulerian grid:

1. **Particle to Grid (P2G)**: Particle quantities are transferred to a background grid using weighted interpolation
2. **Grid Operations**:
   - Incompressibility solver using Jacobi iteration
   - Force application (gravity, etc.)
   - Pressure projection
3. **Grid to Particle (G2P)**: Grid velocities are transferred back to particles
4. **Particle Update**: Particle positions are updated using FLIP/PIC velocity blending

Key components:

- `MPMSimulation` class handles the core simulation logic
- Particles store position, velocity, mass, and emitter information
- Grid cells store mass and velocity for computation

### 2. Python Bindings

The C++ simulation is exposed to Python using pybind11.

### 3. Blender Integration

The addon is implemented as a modal operator in Blender:

1. **Initialization**:
   - Creates simulation instance
   - Sets up GPU shader for visualization
   - Initializes particles

2. **Modal Execution**:
   - Runs simulation steps on timer events
   - Updates particle visualization
   - Handles user interaction

3. **Visualization**:
   - Uses Blender's GPU module for real-time particle rendering
   - Custom shaders for particle appearance
   - Dynamic updates in the 3D viewport

## Building

1. **Prerequisites**:
   - Visual Studio 2022
   - Python 3.11
   - Eigen (included in extern/)
   - pybind11 (included in extern/)

2. **Compilation**:

    ```batch
       compile.bat
       dll2pyd.bat
    ```

3. **Installation**:
   - Copy the `pysimusama` folder to Blender's addon directory
   - Enable the addon in Blender preferences

## Usage

1. Open Blender's 3D viewport
2. Find the "MPM Simulation" panel in the sidebar (N-panel)
3. Click "Run MPM Simulation" to start
4. Use ESC to stop the simulation

## Technical Details

### Simulation Parameters

- Grid size: 100³
- Time step: Based on Blender's FPS settings
- Default particle mass: 0.1
- Gravity: -9.81 m/s²
- Rest density: 1000.0 kg/m³
- Dynamic viscosity: 0.001 Pa·s

### FLIP/PIC Blending

The simulation uses a hybrid FLIP/PIC method with a 95% FLIP ratio for stable yet energetic behavior.

### Boundary Conditions

Simple bounce conditions are implemented with a 50% velocity damping on collision.

## Future Improvements

- Multi-material support
- Adaptive time-stepping
- Surface tension
- Two-way coupling with Blender objects
- Particle emission from mesh surfaces
- Save/load simulation cache

## License

- Addon side is licensed by GPLv3
- Engine (C++ side) is non-commercial

## Credits

J. Fran Matheu, 2024
