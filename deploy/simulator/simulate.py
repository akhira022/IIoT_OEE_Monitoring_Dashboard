#!/usr/bin/env python3
"""Conveyor sensor simulator for the local development stack.

Stands in for the ESP32 + IR sensors described in the project README. It
publishes *cumulative* item counts to the same MQTT topics the firmware uses,
which is exactly what the Node-RED OEE engine expects:

    factory/conveyor/total  - total items entering the conveyor
    factory/conveyor/good   - items that passed QC
    factory/conveyor/waste  - items rejected as defective

good + waste always equals total, and roughly 8-12% of items are treated as
waste so the OEE / Quality figures look realistic on the dashboard.
"""

import os
import random
import time

import paho.mqtt.client as mqtt

MQTT_HOST = os.getenv("MQTT_HOST", "mosquitto")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
INTERVAL = float(os.getenv("PUBLISH_INTERVAL_SEC", "2"))

TOPIC_TOTAL = "factory/conveyor/total"
TOPIC_GOOD = "factory/conveyor/good"
TOPIC_WASTE = "factory/conveyor/waste"


def main() -> None:
    client = mqtt.Client(client_id="conveyor-simulator")
    client.on_connect = lambda c, u, f, rc: print(
        f"[simulator] connected to {MQTT_HOST}:{MQTT_PORT} (rc={rc})",
        flush=True,
    )

    while True:
        try:
            client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
            break
        except Exception as exc:  # broker may still be starting up
            print(f"[simulator] waiting for broker: {exc}", flush=True)
            time.sleep(2)

    client.loop_start()

    total = 0
    good = 0
    waste = 0

    print(
        f"[simulator] publishing every {INTERVAL}s to "
        f"{TOPIC_TOTAL} / {TOPIC_GOOD} / {TOPIC_WASTE}",
        flush=True,
    )

    while True:
        new_items = random.randint(1, 3)
        for _ in range(new_items):
            total += 1
            if random.random() < 0.10:  # ~10% defective
                waste += 1
            else:
                good += 1

        client.publish(TOPIC_TOTAL, total, qos=0)
        client.publish(TOPIC_GOOD, good, qos=0)
        client.publish(TOPIC_WASTE, waste, qos=0)

        print(
            f"[simulator] total={total} good={good} waste={waste}",
            flush=True,
        )
        time.sleep(INTERVAL)


if __name__ == "__main__":
    main()
