#!/usr/bin/env bash
# Idempotent setup for the IIoT OEE development stack.
#
# Installs Docker + the dependencies needed to run Docker inside a Cloud Agent
# VM (fuse-overlayfs storage driver, iptables), configures the daemon, and
# pre-builds/pulls the docker-compose images so a later `start` is fast.
#
# Safe to run multiple times.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

log() { echo "[install] $*"; }

# --- 1. System packages -----------------------------------------------------
if ! command -v docker >/dev/null 2>&1; then
    log "Installing Docker and dependencies via apt..."
    export DEBIAN_FRONTEND=noninteractive
    sudo apt-get update -qq
    sudo apt-get install -y -qq \
        -o Dpkg::Options::=--force-confold \
        -o Dpkg::Options::=--force-confdef \
        docker.io docker-compose-v2 fuse-overlayfs iptables uidmap
else
    log "Docker already installed: $(docker --version)"
fi

# --- 2. Daemon configuration -------------------------------------------------
# fuse-overlayfs works inside the nested (overlay-on-overlay) Cloud Agent VM
# where the default overlay2 driver cannot be used.
log "Writing /etc/docker/daemon.json"
sudo mkdir -p /etc/docker
echo '{
  "storage-driver": "fuse-overlayfs"
}' | sudo tee /etc/docker/daemon.json >/dev/null

# --- 3. Temporary daemon to pre-build images ---------------------------------
DOCKERD_STARTED_BY_INSTALL=0
if ! sudo docker info >/dev/null 2>&1; then
    log "Starting a temporary dockerd to pre-build images..."
    sudo bash -c 'nohup dockerd >/var/log/dockerd-install.log 2>&1 &'
    DOCKERD_STARTED_BY_INSTALL=1
    for _ in $(seq 1 30); do
        if sudo docker info >/dev/null 2>&1; then break; fi
        sleep 1
    done
fi

log "Building and pulling compose images..."
cd "$REPO_ROOT"
sudo docker compose --env-file deploy/.env build
sudo docker compose --env-file deploy/.env pull --ignore-buildable || true

if [ "$DOCKERD_STARTED_BY_INSTALL" = "1" ]; then
    log "Stopping the temporary dockerd (start.sh manages it per-boot)."
    sudo pkill -x dockerd || true
fi

log "Install complete."
