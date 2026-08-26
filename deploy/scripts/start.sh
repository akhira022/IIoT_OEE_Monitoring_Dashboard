#!/usr/bin/env bash
# Per-boot startup for the IIoT OEE development stack.
#
# Starts the Docker daemon (if needed), applies the networking tweak required
# for container-to-container traffic inside the Cloud Agent VM, then brings the
# docker-compose stack up and waits for it to become healthy.
#
# Safe to run multiple times.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"

log() { echo "[start] $*"; }

# --- 1. Docker daemon --------------------------------------------------------
if ! sudo docker info >/dev/null 2>&1; then
    log "Starting dockerd..."
    sudo bash -c 'nohup dockerd >/var/log/dockerd.log 2>&1 &'
    for _ in $(seq 1 30); do
        if sudo docker info >/dev/null 2>&1; then break; fi
        sleep 1
    done
fi
sudo docker info >/dev/null 2>&1 || { log "dockerd failed to start"; tail -n 40 /var/log/dockerd.log || true; exit 1; }

# --- 2. Bridge networking ----------------------------------------------------
# In this nested VM, br_netfilter routes same-bridge container traffic through
# the iptables FORWARD chain, where Docker's nftables rules drop it. Turning
# bridge-nf off lets containers on the same compose network talk to each other.
# Published host ports (routed, DNAT'd) are unaffected.
if [ -e /proc/sys/net/bridge/bridge-nf-call-iptables ]; then
    log "Disabling bridge netfilter (container-to-container connectivity)."
    sudo sysctl -q net.bridge.bridge-nf-call-iptables=0 || true
    sudo sysctl -q net.bridge.bridge-nf-call-ip6tables=0 || true
fi

# --- 3. Bring up the stack ---------------------------------------------------
log "Starting docker-compose stack..."
sudo docker compose --env-file deploy/.env up -d

log "Waiting for InfluxDB to report healthy..."
for _ in $(seq 1 30); do
    status="$(sudo docker inspect -f '{{.State.Health.Status}}' oee-influxdb 2>/dev/null || echo starting)"
    if [ "$status" = "healthy" ]; then break; fi
    sleep 2
done

sudo docker compose --env-file deploy/.env ps
log "Stack is up. Grafana: http://localhost:3000  Node-RED: http://localhost:1880"
