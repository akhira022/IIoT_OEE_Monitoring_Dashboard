
(thai)
#  IIoT OEE Monitoring Dashboard v 8.3

ระบบตรวจสอบประสิทธิภาพสายพานลำเลียงอัจฉริยะแบบ Real-time ด้วยสถาปัตยกรรม Industrial IoT (IIoT) รองรับการคำนวณดัชนีประสิทธิผลโดยรวมของเครื่องจักร (OEE) บนระบบหลังบ้าน และแสดงผลแบบอัตโนมัติผ่าน Dashboard มาตรฐานอุตสาหกรรม

---

##  เทคโนโลยีที่เลือกใช้งาน (Tech Stack)

* **Hardware:** ESP32 Microcontroller + IR Sensor x 3 (Total, Good, Waste)
* **Protocol:** MQTT (ผ่านทาง HiveMQ Cloud Secure Port 8883)
* **Backend Logic:** Node-RED (ประมวลผลสมการ OEE และระบบ Watchdog จับเวลาคนหาย 1 นาที)
* **Database:** InfluxDB v2 (จัดเก็บข้อมูลอนุกรมเวลา Time-Series ขาเข้าและผลลัพธ์)
* **Visualization:** Grafana Dashboard (แสดงผลกราฟเปอร์เซ็นต์และเกจวัดสถานะ)

---

##  สถาปัตยกรรมข้อมูล (Data Flow & Logic)

ระบบทำการดักจับสตรีมข้อมูลชิ้นงานผ่านโปรโตคอล MQTT จากเซนเซอร์ 3 จุด และนำมาประมวลผลที่ระบบหลังบ้าน (Node-RED) เพื่อหาค่าประสิทธิภาพตามหลักการทางวิศวกรรม:

$$OEE = Availability \times Performance \times Quality$$

**ฟีเจอร์เพิ่มเติมฝั่ง Logic:**

* **Watchdog / Timeout:** หากไม่มีชิ้นงานไหลผ่านเซนเซอร์ตัวใดตัวหนึ่งเกินเวลา 1 นาที Node-RED จะทำการ Override ค่าเพื่อเปลี่ยนสถานะเครื่องจักรเป็น **IDLE (หยุดพัก)** อัตโนมัติ

###  MQTT Topic Structure

* `factory/conveyor/total` : ยอดนับชิ้นงานรวมจากต้นสายพาน (Total Count)
* `factory/conveyor/good` : ยอดนับชิ้นงานที่ผ่านการตรวจสอบคุณภาพ (Good Count)
* `factory/conveyor/waste` : ยอดนับชิ้นงานเสียที่ถูกคัดออก (Waste Count)

---

##  การต่อใช้งานขาพินฮาร์ดแวร์ (Pin Assignment)

| อุปกรณ์ฮาร์ดแวร์ | ขาพิน ESP32 | หน้าที่การทำงาน |
| --- | --- | --- |
| **IR Sensor 1 (Total)** | `GPIO 14` | นับจำนวนชิ้นงานทั้งหมดที่เข้าสู่สายพาน |
| **IR Sensor 2 (Good)** | `GPIO 13` | นับจำนวนชิ้นงานดีที่ผ่านเกณฑ์ QC |
| **IR Sensor 3 (Waste)** | `GPIO 12` | นับจำนวนชิ้นงานเสียที่ถูกปัดออก |

*(หมายเหตุ: สามารถปรับเปลี่ยนขา GPIO ของเซนเซอร์ได้ตามความเหมาะสมของโค้ดฝั่ง Arduino)*

---

##  วิธีนำโปรเจกต์ไปติดตั้งเพื่อใช้งาน (Installation)

1. **ฝั่งฮาร์ดแวร์ (ESP32):** เปิดไฟล์โค้ดในโฟลเดอร์ `esp32/` ด้วย Arduino IDE ทำการตั้งค่าชื่อ Wi-Fi (SSID), รหัสผ่าน และข้อมูลประจำตัว HiveMQ Cloud (Username/Password) จากนั้นอัปโหลดลงบอร์ด
2. **ฝั่งหลังบ้าน (Node-RED):** เปิด Node-RED ผ่านบราวเซอร์ที่พอร์ต `1880` กดเมนูขวาบน เลือก **Import** แล้วนำไฟล์ `node-red/flows.json` ไปวาง เพื่อนำเข้า Flow การคำนวณ OEE และระบบ Watchdog
3. **ฝั่งฐานข้อมูล (InfluxDB):** ตรวจสอบการรันเซิร์ฟเวอร์ที่พอร์ต `8086` และสร้าง Bucket ชื่อ `conveyor_oee`
4. **ฝั่งแสดงผล (Grafana):** เปิดบราวเซอร์ที่พอร์ต `3000` นำเข้าไฟล์ Dashboard Template เพื่อเริ่มมอนิเตอร์สถานะเครื่องจักรได้ทันที







---
 `README .md` (Eng)
# IIoT OEE Monitoring Dashboard v 1.0

A real-time smart conveyor belt efficiency monitoring system powered by Industrial IoT (IIoT) architecture. Features backend calculation of Overall Equipment Effectiveness (OEE) metrics and automated visualization via an industry-standard dashboard.

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

| Hardware Component | ESP32 Pin | Function / Description |
| --- | --- | --- |
| **IR Sensor 1 (Total)** | `GPIO 14` | Counts total incoming items on the conveyor |
| **IR Sensor 2 (Good)** | `GPIO 13` | Counts items meeting quality criteria (Good) |
| **IR Sensor 3 (Waste)** | `GPIO 12` | Counts defective/rejected items (Waste) |

*(Note: Pin assignments can be customized in the Arduino code to fit your specific hardware configuration.)*

---

## Installation & Setup Guide

1. **Hardware Setup (ESP32):** Open the project code in `esp32/` using Arduino IDE. Configure your Wi-Fi credentials (SSID & Password) and HiveMQ Cloud authentication details (Username/Password), then upload the sketch to the ESP32 board.
2. **Backend Setup (Node-RED):** Access Node-RED via your browser at port `1880`. Click the top-right menu, select **Import**, and paste/upload `node-red/flows.json` to load the OEE calculation engine and Watchdog logic.
3. **Database Setup (InfluxDB):** Ensure InfluxDB v2 is running on port `8086` and create a bucket named `conveyor_oee`.
4. **Visualization Setup (Grafana):** Open Grafana at port `3000` and import the dashboard template file to start real-time monitoring.
