#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "/opt/ros/${ROS_DISTRO:-jazzy}/setup.bash"
cd "$ROOT"

# The workspace is intentionally almost empty in Phase 0. Packages are added only after
# they are ported and tested against ROS 2 Jazzy.
colcon build --symlink-install
