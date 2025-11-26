#!/usr/bin/env bash
set -euo pipefail

TOPFILE="torus_adj.txt"
OUT="run.log"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --top) TOPFILE="$2"; shift 2;;
    --outfile) OUT="$2"; shift 2;;
    *) echo "Unknown arg"; exit 1;;
  esac
done

echo "Running SST with topology $TOPFILE (using sst_input_template.py if available)"
sst sst_input_template.py --topology "$TOPFILE" > "$OUT" 2>&1 &
