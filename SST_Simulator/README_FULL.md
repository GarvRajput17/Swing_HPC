SST Swing Full Bundle
=====================

Contents:
- torus_generator.cpp                 (your existing torus generator)
- build.sh
- generate_topology.sh
- run_sst.sh
- torus_adj.txt                       (sample 16-node torus)
- README.md (previous)
- notes.txt
- SwingAlgorithm.h                    (algorithm header you provided). See citation in chat.
- src/ (SST component skeleton & CMake files)
  - SwingWorker.h
  - SwingWorker.cc
  - CMakeLists.txt

IMPORTANT:
- The included SST worker component is a **skeleton** that demonstrates how to
  integrate the SwingAlgorithm scheduling logic into an SST component. It is
  intentionally conservative to avoid build fragility across different SST installs.
- You may need to adapt include paths and link targets to match your SST installation.
- If you want, I can turn the skeleton into a fully working component tuned for your SST version
  (I will add serialization details and tested build commands). Just ask.

Reference:
The Swing scheduling logic file included is the header you uploaded: SwingAlgorithm.h.
This contains pi(), rho(), delta(), and the bandwidth/latency variants. fileciteturn1file0

Quick start:
1. Unzip this bundle.
2. Build the torus generator: ./build.sh
3. Generate topology: ./generate_topology.sh 16
4. Inspect src/ for the SwingWorker skeleton. Adapt CMake and build against your SST.
5. I can next produce a fully tested C++ SST element if you want.

