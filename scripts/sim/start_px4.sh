#!/usr/bin/env bash
set -euo pipefail

PX4_DIR="${PX4_DIR:-/opt/PX4-Autopilot}"
VEHICLE_NAME="${IMAV_VEHICLE_NAME:-x500}"
WORLD_NAME="${IMAV_WORLD:-imav2026_indoor}"

if [[ ! -x "$PX4_DIR/build/px4_sitl_default/bin/px4" ]]; then
  echo "PX4 SITL binary is missing. Rebuild the Docker image." >&2
  exit 1
fi

cd "$PX4_DIR"
export PX4_GZ_STANDALONE=1
export PX4_SYS_AUTOSTART=4001
export PX4_GZ_MODEL_NAME="$VEHICLE_NAME"
export PX4_GZ_WORLD="$WORLD_NAME"

# PX4 1.17 can attach to an already-spawned Gazebo model by exact model name.
exec "$PX4_DIR/build/px4_sitl_default/bin/px4"
