#!/usr/bin/env bash
set -euo pipefail

echo "Starting Gazebo GUI and attaching to the running server."
exec gz sim -g
