#!/usr/bin/env bash
set -euo pipefail

printf 'OS: '
. /etc/os-release
printf '%s %s\n' "$NAME" "$VERSION_ID"
printf 'Architecture: '; uname -m
printf 'Kernel: '; uname -r

for cmd in git docker; do
  if command -v "$cmd" >/dev/null 2>&1; then
    printf '%-12s OK: ' "$cmd"
    "$cmd" --version | head -n 1
  else
    printf '%-12s MISSING\n' "$cmd"
  fi
done

if docker compose version >/dev/null 2>&1; then
  printf 'docker compose OK: '
  docker compose version
else
  echo 'docker compose MISSING'
fi

if command -v git-lfs >/dev/null 2>&1; then
  printf 'git-lfs      OK: '
  git-lfs version
else
  echo 'git-lfs      MISSING (needed before adding CAD files)'
fi
