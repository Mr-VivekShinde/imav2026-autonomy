#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
docker compose down --remove-orphans
if command -v xhost >/dev/null 2>&1; then
  xhost -local:root >/dev/null 2>&1 || true
fi
