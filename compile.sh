#!/usr/bin/env bash
set -euo pipefail

CXX=${CXX:-g++}
CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic"

echo "== building wireTest =="
"$CXX" $CXXFLAGS -o wireTest tests/wireTest.cpp
./wireTest

echo
echo "== building cluster_sim =="
"$CXX" $CXXFLAGS -pthread -I src -o cluster_sim src/cluster_sim.cpp
echo "== running cluster_sim =="
./cluster_sim

echo
echo "== building mutex_sim =="
"$CXX" $CXXFLAGS -pthread -I src -o mutex_sim src/mutex_sim.cpp
echo "== running mutex_sim =="
./mutex_sim
