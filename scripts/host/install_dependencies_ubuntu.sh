#!/usr/bin/env bash
set -euo pipefail

# Installs ONLY host-side collaboration/runtime tools.
# ROS 2, Gazebo, PX4 and MAVSDK stay inside the project Docker image.

if [[ $EUID -eq 0 ]]; then
  echo "Run this script as your normal user, not as root." >&2
  exit 1
fi

. /etc/os-release
if [[ "${ID:-}" != "ubuntu" ]]; then
  echo "This installer is intended for Ubuntu. Detected: ${ID:-unknown}" >&2
  exit 1
fi

case "${VERSION_ID:-}" in
  24.04|22.04) ;;
  *)
    echo "Recommended host is Ubuntu 24.04 LTS; 22.04 is also acceptable. Detected ${VERSION_ID:-unknown}." >&2
    exit 1
    ;;
esac

sudo apt-get update
sudo apt-get install -y ca-certificates curl git git-lfs x11-xserver-utils

git lfs install

# Remove packages that conflict with Docker's official packages if they exist.
conflicts=(docker.io docker-compose docker-compose-v2 docker-doc docker-buildx podman-docker containerd runc)
installed=()
for pkg in "${conflicts[@]}"; do
  if dpkg -s "$pkg" >/dev/null 2>&1; then
    installed+=("$pkg")
  fi
done
if ((${#installed[@]})); then
  sudo apt-get remove -y "${installed[@]}"
fi

sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

sudo tee /etc/apt/sources.list.d/docker.sources >/dev/null <<DOCKER_REPO
Types: deb
URIs: https://download.docker.com/linux/ubuntu
Suites: ${UBUNTU_CODENAME:-$VERSION_CODENAME}
Components: stable
Architectures: $(dpkg --print-architecture)
Signed-By: /etc/apt/keyrings/docker.asc
DOCKER_REPO

sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
sudo usermod -aG docker "$USER"

echo
echo "Host dependencies installed. IMPORTANT: log out and log back in so Docker group membership takes effect."
echo "Then run: docker run --rm hello-world"
