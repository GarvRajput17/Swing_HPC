#!/bin/bash

echo "Building Swing Allreduce Simulator..."
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

echo ""
echo "Build complete!"
echo ""
echo "Running simulations..."
echo ""

# Run various configurations
echo "=== 8x8 2D Torus (64 nodes) ==="
./swing_sim 8 8

echo ""
echo "=== 4x4x4 3D Torus (64 nodes) ==="
./swing_sim 4 4 4

echo ""
echo "=== 16x4 Rectangular Torus (64 nodes) ==="
./swing_sim 16 4

echo ""
echo "All simulations complete!"
