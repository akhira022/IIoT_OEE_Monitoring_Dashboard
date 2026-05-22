
#  IIoT OEE Monitoring Dashboard
ระบบตรวจสอบประสิทธิภาพสายพานลำเลียงอัจฉริยะแบบ Real-time ด้วยสถาปัตยกรรม Industrial IoT (IIoT) รองรับการคำนวณและประมวลผลข้อมูลชิ้นงานบนระบบหลังบ้าน เพื่อแสดงผลในรูปแบบ Dashboard มาตรฐานอุตสาหกรรม

---

##  เทคโนโลยีที่เลือกใช้งาน (Tech Stack)
* **Hardware:** ESP32 Microcontroller (คุมสายพานและดักจับเซนเซอร์)
* **Protocol:** MQTT (ผ่านทาง HiveMQ Cloud Secure Port 8883)
* **Backend Logic:** Node-RED (ประมวลผลและคำนวณค่าชิ้นงาน)
* **Database:** InfluxDB (จัดเก็บข้อมูลอนุกรมเวลาต่อเนื่อง 1 ชั่วโมง)
* **Visualization:** Grafana Dashboard (แสดงผลกราฟและตัวเลขดิจิทัล)

---

##  สถาปัตยกรรมข้อมูล (Data Flow & Logic)

ระบบทำการดักจับสตรีมข้อมูลผ่านโพรโทคอล MQTT และนำมาประมวลผลคำนวณที่ระบบหลังบ้าน (Node-RED) ตามสมการคณิตศาสตร์:

$$Good\ Count\ (ชิ้นงานดี) = Total\ Count\ (ยอดรวม) - Waste\ Count\ (ของเสีย)$$

###  MQTT Topic Structure
* `factory/conveyor/total` : ยอดนับชิ้นงานรวมจากต้นสายพาน
* `factory/conveyor/waste` : ยอดนับชิ้นงานเสียจากจุดตรวจ Manual QC
* `factory/conveyor/status` : สถานะการทำงานของสายพาน (0 = STOP, 1 = RUN)

---

##  การต่อใช้งานขาพินฮาร์ดแวร์ (Pin Assignment)

| อุปกรณ์ฮาร์ดแวร์ | ขาพิน ESP32 | หน้าที่การทำงาน |
| :--- | :---: | :--- |
| **IR Sensor 1 (Total)** | `GPIO 14` | นับจำนวนชิ้นงานทั้งหมดที่เข้าสายพาน |
| **IR Sensor 2 (Waste)** | `GPIO 12` | นับจำนวนชิ้นงานเสียที่ถูกปัดออก (Manual QC) |
| **Relay Module** | `GPIO 27` | ควบคุมการสั่ง เปิด/ปิด มอเตอร์สายพานลำเลียง |
| **Push Button (Start)** | `GPIO 25` | ปุ่มกดเริ่มการทำงานระบบสายพาน |
| **Push Button (Stop)** | `GPIO 26` | ปุ่มกดหยุดการทำงานระบบสายพาน (Emergency) |

---

##  วิธีนำโปรเจกต์ไปติดตั้งเพื่อใช้งาน (Installation)

1. **ฝั่งฮาร์ดแวร์:** เปิดไฟล์โค้ดในโฟลเดอร์ `esp32/` ด้วย Arduino IDE ทำการเปลี่ยนรหัสผ่าน Wi-Fi และอัปโหลดลงบอร์ด
2. **ฝั่งหลังบ้าน:** เข้าไปที่ Node-RED กดเมนูขวาบน เลือก **Import** แล้วนำไฟล์ `node-red/flows.json` ไปวางเพื่อรันระบบจำลองลอจิกทันที
