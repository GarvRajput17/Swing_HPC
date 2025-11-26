#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <dim1> <dim2> [dim3]"
  exit 1
fi

if [ ! -x ./torus_gen ]; then
  echo "torus_gen not found. Run ./build.sh"
  exit 1
fi

./torus_gen "$@"
