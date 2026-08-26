# Local development stack (Docker Compose)

This directory contains a self-contained development environment for the IIoT
OEE Monitoring Dashboard. It runs the entire backend pipeline locally — no
ESP32 hardware and no HiveMQ Cloud account required — using a built-in sensor
simulator:

```
simulator ──MQTT──▶ mosquitto ──▶ node-red (OEE engine) ──▶ influxdb ──▶ grafana
```

## Services

| Service     | Image                   | URL / Port                      | Purpose                                              |
| ----------- | ----------------------- | ------------------------------- | ---------------------------------------------------- |
| `mosquitto` | `eclipse-mosquitto:2.0` | `localhost:1883` (+ `9001` ws)  | Local MQTT broker (replaces HiveMQ Cloud in dev)     |
| `node-red`  | built from `node-red/`  | http://localhost:1880           | OEE calculation engine + Watchdog + dashboard        |
| `influxdb`  | `influxdb:2.7`          | http://localhost:8086           | Time-series storage (org `factory`, bucket `conveyor_oee`) |
| `grafana`   | `grafana/grafana:10.4.2`| http://localhost:3000           | OEE dashboards (auto-provisioned)                    |
| `simulator` | built from `simulator/` | —                               | Publishes fake conveyor counts to the MQTT topics    |

The `node-red` service loads a local-development variant of the production flow
(`../node-red/flows_final_8.3v.json`). It is identical except the MQTT broker
points at the local `mosquitto` service (plain MQTT, no TLS) and InfluxDB points
at the local `influxdb` service. Grafana auto-provisions the InfluxDB datasource
and imports `../grafana/Smart Conveyor Belt - IIoT OEE Dashboard-*.json`.

## Prerequisites

- Docker Engine + the Docker Compose plugin (`docker compose`).

## Usage

From the **repository root**:

```bash
# Build images and start everything
docker compose --env-file deploy/.env up -d --build

# Watch it work
docker compose --env-file deploy/.env logs -f node-red simulator

# Open the dashboards
#   Grafana : http://localhost:3000  (anonymous access enabled; admin/admin)
#   Node-RED: http://localhost:1880

# Tear down (keep data volumes)
docker compose --env-file deploy/.env down

# Tear down and wipe data
docker compose --env-file deploy/.env down -v
```

### Cloud Agent / scripted setup

`deploy/scripts/install.sh` and `deploy/scripts/start.sh` install Docker, build
the images, and start the stack. They are wired into `.cursor/environment.json`
so Cloud Agents bring the whole system up automatically. `start.sh` also
disables `bridge-nf-call-iptables`, which is required for container-to-container
traffic inside the nested Cloud Agent VM (it is a no-op on a normal Docker host).

## Credentials

`deploy/.env` holds **local-development-only** credentials (InfluxDB org/bucket/
token, Grafana admin login). They are throwaway values for local use — never
reuse them, and configure real secrets for any deployment beyond local dev.

## Using real hardware or HiveMQ Cloud

The simulator only exists so the stack is useful without hardware. To drive it
with a real ESP32, flash `../esp32/esp32_8.3v.ino` and either point the board at
this local broker (`<host-ip>:1883`) or keep the production HiveMQ Cloud broker
and adjust the Node-RED MQTT broker config accordingly.
