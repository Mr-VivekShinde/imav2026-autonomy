#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORLD_NAME="${IMAV_WORLD:-imav2026_indoor}"
MODEL_FILE="$ROOT/simulation/models/x500_flow/model.sdf"
MODEL_NAME="${IMAV_VEHICLE_NAME:-x500}"

export GZ_SIM_RESOURCE_PATH="${GZ_SIM_RESOURCE_PATH:-}:$ROOT/simulation/models:/opt/PX4-Autopilot/Tools/simulation/gz/models"

echo "Spawning temporary reference vehicle '$MODEL_NAME' in world '$WORLD_NAME'."
gz service -s "/world/${WORLD_NAME}/create" \
  --reqtype gz.msgs.EntityFactory \
  --reptype gz.msgs.Boolean \
  --timeout 5000 \
  --req "sdf_filename: '${MODEL_FILE}', name: '${MODEL_NAME}', pose: {position: {x: 1.5, y: 1.0, z: 0.3}, orientation: {x: 0, y: 0, z: 0.7071068, w: 0.7071068}}"
