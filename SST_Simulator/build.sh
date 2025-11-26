#!/usr/bin/env bash
set -euo pipefail

if [ ! -f torus_gen ]; then
  echo "Compiling torus_generator.cpp -> torus_gen"
  g++ -std=c++17 -O3 torus_generator.cpp -o torus_gen
else
  echo "torus_gen already exists"
fi

echo "Done."
