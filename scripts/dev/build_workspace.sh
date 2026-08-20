#!/usr/bin/env bash

# Stop on normal command errors and pipeline failures.
# We intentionally do NOT enable "set -u" before sourcing ROS,
# because ROS setup scripts may reference optional unset variables.
set -eo pipefail

# Find the repository root from this script's location.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Load the ROS 2 environment.
source "/opt/ros/${ROS_DISTRO:-jazzy}/setup.bash"

# Enter the repository root.
cd "$ROOT"

# Build all ROS 2 packages under src/.
colcon build --symlink-install
