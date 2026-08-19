#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE_WORLD="$ROOT/simulation/worlds/imav_2026.sdf"
RANDOM_WORLD="$ROOT/simulation/worlds/imav_2026_random.sdf"

python3 "$ROOT/scripts/sim/generate_world.py" \
  --input "$BASE_WORLD" \
  --output "$RANDOM_WORLD"

export GZ_SIM_RESOURCE_PATH="${GZ_SIM_RESOURCE_PATH:-}:$ROOT/simulation/models:/opt/PX4-Autopilot/Tools/simulation/gz/worlds:/opt/PX4-Autopilot/Tools/simulation/gz/models"

echo "Starting Gazebo server with world: $RANDOM_WORLD"
exec gz sim -s -r "$RANDOM_WORLD" -v 3
