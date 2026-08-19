#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

# Allow Gazebo GUI from the local Docker container. Revoke later with: xhost -local:root
if command -v xhost >/dev/null 2>&1; then
  xhost +local:root >/dev/null
fi

docker compose up -d sim
docker compose exec sim bash
