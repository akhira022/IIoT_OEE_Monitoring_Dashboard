# IIoT OEE Monitoring Dashboard v 8.3

ระบบตรวจสอบประสิทธิภาพสายพานลำเลียงอัจฉริยะแบบ Real-time ด้วยสถาปัตยกรรม Industrial IoT (IIoT) รองรับการคำนวณดัชนีประสิทธิผลโดยรวมของเครื่องจักร (OEE) บนระบบหลังบ้าน และแสดงผลแบบอัตโนมัติผ่าน Dashboard มาตรฐานอุตสาหกรรม

> เอกสารครบสำหรับโปรเจค ปวส. (แยกบทที่ 1–5 + ภาคผนวก): [`docs/README.md`](docs/README.md)  
> เอกสารรวมเล่มฉบับเดียว: [`docs/เอกสารโปรเจค-ปวส.md`](docs/เอกสารโปรเจค-ปวส.md)  
> รันระบบหลังบ้านทั้งหมดบนเครื่องด้วย Docker (ไม่ต้องใช้ฮาร์ดแวร์): [`deploy/README.md`](deploy/README.md)

---

## เทคโนโลยีที่เลือกใช้งาน (Tech Stack)

* **Hardware:** ESP32 Microcontroller + IR Sensor x 3 (Total, Good, Waste)
* **Protocol:** MQTT (ผ่านทาง HiveMQ Cloud Secure Port 8883)
* **Backend Logic:** Node-RED (ประมวลผลสมการ OEE และระบบ Watchdog จับเวลาคนหาย 1 นาที)
* **Database:** InfluxDB v2 (จัดเก็บข้อมูลอนุกรมเวลา Time-Series ขาเข้าและผลลัพธ์)
* **Visualization:** Grafana Dashboard (แสดงผลกราฟเปอร์เซ็นต์และเกจวัดสถานะ)

---

## สถาปัตยกรรมข้อมูล (Data Flow & Logic)

ระบบทำการดักจับสตรีมข้อมูลชิ้นงานผ่านโปรโตคอล MQTT จากเซนเซอร์ 3 จุด และนำมาประมวลผลที่ระบบหลังบ้าน (Node-RED) เพื่อหาค่าประสิทธิภาพตามหลักการทางวิศวกรรม:

$$OEE = Availability \times Performance \times Quality$$

**ฟีเจอร์เพิ่มเติมฝั่ง Logic:**

* **Watchdog / Timeout:** หากไม่มีชิ้นงานไหลผ่านเซนเซอร์ตัวใดตัวหนึ่งเกินเวลา 1 นาที Node-RED จะทำการ Override ค่าเพื่อเปลี่ยนสถานะเครื่องจักรเป็น **IDLE (หยุดพัก)** อัตโนมัติ

### MQTT Topic Structure

* `factory/conveyor/total` : ยอดนับชิ้นงานรวมจากต้นสายพาน (Total Count)
* `factory/conveyor/good` : ยอดนับชิ้นงานที่ผ่านการตรวจสอบคุณภาพ (Good Count)
* `factory/conveyor/waste` : ยอดนับชิ้นงานเสียที่ถูกคัดออก (Waste Count)

---

## การต่อใช้งานขาพินฮาร์ดแวร์ (Pin Assignment)

อ้างอิงจาก firmware ปัจจุบัน `esp32/esp32_8.3v.ino`

| อุปกรณ์ฮาร์ดแวร์ | ขาพิน ESP32 | หน้าที่การทำงาน |
| --- | --- | --- |
| **IR Sensor 1 (Total)** | `GPIO 2` | นับจำนวนชิ้นงานทั้งหมดที่เข้าสู่สายพาน |
| **IR Sensor 2 (Good)** | `GPIO 4` | นับจำนวนชิ้นงานดีที่ผ่านเกณฑ์ QC |
| **IR Sensor 3 (Waste)** | `GPIO 5` | นับจำนวนชิ้นงานเสียที่ถูกปัดออก |

*(หมายเหตุ: สามารถปรับเปลี่ยนขา GPIO ของเซนเซอร์ได้ตามความเหมาะสมของโค้ดฝั่ง Arduino)*

---

## โครงสร้างไฟล์ในโปรเจกต์

* `esp32/esp32_8.3v.ino` — firmware หลัก (IR 3 ตัว + MQTT TLS)
* `esp32/esp32_3.2v.ino` — เวอร์ชันเก่า (มีปุ่ม Start/Stop/Reset + รีเลย์มอเตอร์)
* `node-red/flows_final_8.3v.json` — Node-RED flow คำนวณ OEE และ Watchdog
* `grafana/Smart Conveyor Belt - IIoT OEE Dashboard-1785916197192.json` — Grafana dashboard template
* `docs/README.md` — สารบัญเอกสารรายงานแบบแยกบท (ปวส.)
* `docs/เอกสารโปรเจค-ปวส.md` — เอกสารโปรเจคฉบับรวมเล่ม

---

## วิธีนำโปรเจกต์ไปติดตั้งเพื่อใช้งาน (Installation)

1. **ฝั่งฮาร์ดแวร์ (ESP32):** เปิด `esp32/esp32_8.3v.ino` ด้วย Arduino IDE แล้วแทนที่ค่า `YOUR_WIFI_SSID`, `YOUR_WIFI_PASSWORD`, `YOUR_CLUSTER_ID.s1.eu.hivemq.cloud`, `YOUR_MQTT_USERNAME`, และ `YOUR_MQTT_PASSWORD` จากนั้นอัปโหลดลงบอร์ด
2. **ฝั่งหลังบ้าน (Node-RED):** เปิด Node-RED ผ่านบราวเซอร์ที่พอร์ต `1880` กดเมนูขวาบน เลือก **Import** แล้วนำเข้าไฟล์ `node-red/flows_final_8.3v.json` จากนั้นตั้งค่า MQTT broker hostname และ credentials ของ HiveMQ Cloud ให้ตรงกับ ESP32
3. **ฝั่งฐานข้อมูล (InfluxDB):** ตรวจสอบการรันเซิร์ฟเวอร์ที่พอร์ต `8086` และสร้าง Bucket ชื่อ `conveyor_oee`
4. **ฝั่งแสดงผล (Grafana):** เปิดบราวเซอร์ที่พอร์ต `3000` นำเข้าไฟล์ `grafana/Smart Conveyor Belt - IIoT OEE Dashboard-1785916197192.json` เพื่อเริ่มมอนิเตอร์สถานะเครื่องจักรได้ทันที

---

# IIoT OEE Monitoring Dashboard v 8.3 (English)

A real-time smart conveyor belt efficiency monitoring system powered by Industrial IoT (IIoT) architecture. Features backend calculation of Overall Equipment Effectiveness (OEE) metrics and automated visualization via an industry-standard dashboard.

> Full project documentation (Thai, vocational diploma chapters 1–5): [`docs/README.md`](docs/README.md)

---

## Tech Stack

* **Hardware:** ESP32 Microcontroller + IR Sensors x 3 (Total, Good, Waste)
* **Protocol:** MQTT (via HiveMQ Cloud Secure Port 8883)
* **Backend Logic:** Node-RED (OEE computation engine & 1-minute timeout Watchdog system)
* **Database:** InfluxDB v2 (Time-series database for raw sensor streams and computed metrics)
* **Visualization:** Grafana Dashboard (Real-time gauges, status indicators, and historical trends)

---

## Data Architecture & Logic

The system ingests real-time item counts via MQTT across 3 sensor points and processes them on the backend (Node-RED) to compute performance based on standard industrial engineering principles:

$$OEE = Availability \times Performance \times Quality$$

**Key Logic Feature:**

* **Watchdog / Timeout Mechanism:** If no items pass any sensor for over 1 minute, Node-RED automatically overrides the machine status to **IDLE (Standby)** to maintain data accuracy.

### MQTT Topic Structure

* `factory/conveyor/total` : Total item count at the entry point (Total Count)
* `factory/conveyor/good` : Passed quality check item count (Good Count)
* `factory/conveyor/waste` : Defective or rejected item count (Waste Count)

---

## Hardware Pin Assignment

Based on the current firmware in `esp32/esp32_8.3v.ino`

| Hardware Component | ESP32 Pin | Function / Description |
| --- | --- | --- |
| **IR Sensor 1 (Total)** | `GPIO 2` | Counts total incoming items on the conveyor |
| **IR Sensor 2 (Good)** | `GPIO 4` | Counts items meeting quality criteria (Good) |
| **IR Sensor 3 (Waste)** | `GPIO 5` | Counts defective/rejected items (Waste) |

*(Note: Pin assignments can be customized in the Arduino code to fit your specific hardware configuration.)*

---

## Project Files

* `esp32/esp32_8.3v.ino` — primary firmware (3 IR sensors + MQTT TLS)
* `esp32/esp32_3.2v.ino` — legacy firmware (Start/Stop/Reset buttons + motor relay)
* `node-red/flows_final_8.3v.json` — Node-RED OEE calculation and Watchdog flow
* `grafana/Smart Conveyor Belt - IIoT OEE Dashboard-1785916197192.json` — Grafana dashboard template
* `docs/README.md` — chapter index for vocational diploma report
* `docs/เอกสารโปรเจค-ปวส.md` — single-file full project documentation

---

## Installation & Setup Guide

1. **Hardware Setup (ESP32):** Open `esp32/esp32_8.3v.ino` in Arduino IDE. Replace the placeholders `YOUR_WIFI_SSID`, `YOUR_WIFI_PASSWORD`, `YOUR_CLUSTER_ID.s1.eu.hivemq.cloud`, `YOUR_MQTT_USERNAME`, and `YOUR_MQTT_PASSWORD`, then upload the sketch to the ESP32 board.
2. **Backend Setup (Node-RED):** Access Node-RED via your browser at port `1880`. Click the top-right menu, select **Import**, and upload `node-red/flows_final_8.3v.json`. Then configure the HiveMQ Cloud broker hostname and credentials to match the ESP32 settings.
3. **Database Setup (InfluxDB):** Ensure InfluxDB v2 is running on port `8086` and create a bucket named `conveyor_oee`.
4. **Visualization Setup (Grafana):** Open Grafana at port `3000` and import `grafana/Smart Conveyor Belt - IIoT OEE Dashboard-1785916197192.json` to start real-time monitoring.

---

## Run the Whole Backend Locally (Docker Compose)

Don't have an ESP32 or a HiveMQ Cloud account? A ready-to-run development stack
spins up Mosquitto (MQTT), Node-RED (OEE engine), InfluxDB v2, and Grafana —
plus a sensor **simulator** that feeds the `factory/conveyor/*` topics — so you
can see the full pipeline working end-to-end on your machine:

```bash
docker compose --env-file deploy/.env up -d --build
# Grafana : http://localhost:3000   (anonymous access enabled)
# Node-RED: http://localhost:1880
```

See [`deploy/README.md`](deploy/README.md) for full details. Cloud Agents pick
this up automatically via [`.cursor/environment.json`](.cursor/environment.json).
