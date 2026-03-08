# Wave simulation on the GPU

> [!NOTE]
> This is a work in progress.

Simulate and visualize fluid dynamics algorithms on the GPU. The implementation progress and design decisions are documented [here](https://pbhnblog.ballif.eu).

# Features

- rendering as a triangle mesh using OpenGL
- 2D advection equation with a CUDA kernel
- rendering of 3D grids with Marching Cubes
- 3D incompressible two-fluid Navier-Stokes equations using a Volume of Fluid (VoF) method

# Compilation

Uses [CMake](https://cmake.org).

```shell
mkdir build && cd build
cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build .
# Optional: run tests
ctest --test-dir tests
```

# Usage

```
mkdir data
python scripts/make_initial_conditions.py 15
cd build
./src/waves --size 15 -i ../data/still.npy
```

