#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "/opt/ros/${ROS_DISTRO:-jazzy}/setup.bash"

if [[ -f "$ROOT/install/setup.bash" ]]; then
  source "$ROOT/install/setup.bash"
fi

exec ros2 run ros_gz_bridge parameter_bridge --ros-args \
  -p config_file:="$ROOT/simulation/bridge/gz_bridge.yaml"
