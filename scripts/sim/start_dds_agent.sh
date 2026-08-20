#!/usr/bin/env bash
set -eo pipefail

# PX4 SITL uses UDP port 8888 for its uXRCE-DDS client by default.
PORT="${PX4_UXRCE_DDS_PORT:-8888}"

if ! command -v MicroXRCEAgent >/dev/null 2>&1; then
    echo "ERROR: MicroXRCEAgent is not installed in this container."
    exit 1
fi

echo "Starting Micro XRCE-DDS Agent on UDP port ${PORT}..."

exec MicroXRCEAgent udp4 -p "${PORT}"
