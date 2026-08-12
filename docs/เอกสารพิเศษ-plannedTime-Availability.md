# เอกสารพิเศษ: plannedTime กับ Availability ในระบบ OEE

**โปรเจค:** IIoT OEE Monitoring Dashboard v8.3  
**ไฟล์โค้ดอ้างอิง:** `node-red/flows_final_8.3v.json` → Function node ชื่อ **OEE Calculation Engine**  
**จุดประสงค์เอกสารนี้:** อธิบายว่า `plannedTime` มาจากไหน คำนวณอย่างไร และใช้กับ Availability ได้อย่างไร

---

## 1. คำถามตั้งต้น

ในสูตร Availability มักเขียนว่า:

\[
Availability = \frac{Running\ Time}{Planned\ Time} \times 100
\]

หลายคนสงสัยว่า **Planned Time ในโปรเจคนี้เอามาจากไหน**  
คำตอบสั้น ๆ: **ระบบคำนวณให้อัตโนมัติจากเวลาที่ Node-RED เริ่มทำงาน** ไม่ได้ให้ผู้ใช้กรอกเอง และไม่ได้มาจาก ESP32

ในโค้ดใช้ชื่อตัวแปรว่า `totalPlannedTime`

---

## 2. สรุปนิยามในโปรเจคนี้

| ตัวแปร | ความหมายในโปรเจคนี้ | ที่มา |
| --- | --- | --- |
| `startTime` | เวลาเริ่มนับของระบบ | บันทึกตอน function รันครั้งแรก |
| `now` | เวลาปัจจุบัน | `Date.now()` ทุกรอบคำนวณ |
| `totalPlannedTime` | เวลาทั้งหมดตั้งแต่เปิดระบบจนถึงตอนนี้ (วินาที) | `(now - startTime) / 1000` |
| `runningTimeSec` | เวลาที่ถือว่าเครื่อง RUNNING จริง | สะสมทีละรอบ ถ้ายังไม่ IDLE |
| `availability` | เปอร์เซ็นต์ความพร้อม | `runningTimeSec / totalPlannedTime * 100` |

ส่งออกไป Dashboard/InfluxDB ในชื่อ:
- `running_time_sec` ← จาก `runningTimeSec`
- `total_time_sec` ← จาก `totalPlannedTime`

---

## 3. โค้ดที่เกี่ยวข้อง (ย่อ)

```js
let now = Date.now();

// ดึงเวลาเริ่มต้นจาก memory ของ Node-RED
let startTime = flow.get("startTime") || now;
let lastTime = flow.get("lastTime") || now;
let runningTimeSec = flow.get("runningTimeSec") || 0;

// รอบแรกเท่านั้น: จำ startTime ไว้
if (!flow.get("startTime")) {
    flow.set("startTime", startTime);
}

// เวลาที่ผ่านไปจากรอบก่อนหน้า (วินาที)
let timeDiff = (now - lastTime) / 1000;
let idleTimeoutMs = 60000; // 1 นาที

// ถ้าไม่มีชิ้นงานผ่านเกิน 1 นาที = IDLE
let status = 1; // RUNNING
if ((now - lastObjectTime) > idleTimeoutMs) {
    status = 0; // IDLE/STOPPED
}

// สะสม running time เฉพาะตอน RUNNING
if (status === 1 && timeDiff > 0 && timeDiff < 120) {
    runningTimeSec += timeDiff;
}

flow.set("lastTime", now);
flow.set("runningTimeSec", runningTimeSec);

// ===== จุดคำนวณ plannedTime =====
let totalPlannedTime = (now - startTime) / 1000;
let availability = totalPlannedTime > 0
    ? (runningTimeSec / totalPlannedTime) * 100
    : 100;
```

### อ่านโค้ดทีละประโยค
1. เปิดระบบครั้งแรก → จำ `startTime`
2. ทุก 1 วินาทีโดยประมาณ → อ่านเวลาปัจจุบันเป็น `now`
3. `totalPlannedTime` = ระยะห่างจากเริ่มจนตอนนี้
4. ถ้ายังมีชิ้นงานไหล (ไม่เกิน 60 วินาที) → บวกเวลาเข้า `runningTimeSec`
5. หารกันได้ Availability

---

## 4. แผนภาพการคำนวณ

```text
[เปิด Node-RED / รัน Function ครั้งแรก]
              |
              v
        จำ startTime
              |
              v
   +----------ทุก ~1 วินาที----------+
   |                                 |
   |  now = เวลาปัจจุบัน             |
   |  planned = now - startTime      |
   |                                 |
   |  มีชิ้นงานภายใน 1 นาทีไหม?      |
   |     ใช่ -> บวก runningTime      |
   |     ไม่ -> สถานะ IDLE (ไม่บวก)  |
   |                                 |
   |  Availability =                 |
   |    runningTime / planned * 100  |
   +---------------------------------+
```

---

## 5. ตัวอย่างตัวเลขแบบเข้าใจง่าย

สมมติเริ่มระบบตอน **10:00:00**

### ช่วงที่ 1: ของไหลต่อเนื่อง 5 นาที
- plannedTime = 300 วินาที
- runningTime = 300 วินาที
- Availability = 300 / 300 × 100 = **100%**

### ช่วงที่ 2: หยุดไม่มีชิ้นงาน 2 นาที (IDLE)
- plannedTime = 420 วินาที  
  (เวลานาฬิกายังเดินต่อ เพราะระบบยังเปิดอยู่)
- runningTime = 300 วินาที  
  (ช่วง IDLE ไม่ถูกบวก)
- Availability = 300 / 420 × 100 ≈ **71.4%**

### ช่วงที่ 3: กลับมาวิ่งอีก 3 นาที
- plannedTime = 600 วินาที
- runningTime = 480 วินาที
- Availability = 480 / 600 × 100 = **80%**

สรุปความหมาย:
- **plannedTime** = “เปิดระบบมาแล้วนานแค่ไหน”
- **runningTime** = “ช่วงที่ถือว่าเครื่องทำงานจริงนานแค่ไหน”
- ค่า Availability ต่ำลงเมื่อมีช่วง IDLE ปนอยู่

---

## 6. plannedTime ในทฤษฎีโรงงาน vs ในโปรเจคนี้

| หัวข้อ | โรงงานจริงโดยทั่วไป | โปรเจคนี้ (ชุดสาธิต) |
| --- | --- | --- |
| Planned Time คืออะไร | เวลาตามแผนผลิต เช่น กะ 8 ชั่วโมง | เวลาที่ระบบเปิดทำงานจริง |
| ใครกำหนด | วางแผนโดยคน / ระบบผลิต | คำนวณอัตโนมัติจาก `startTime` |
| ข้อดี | ตรงนิยามมาตรฐานมากขึ้น | สาธิตง่าย ไม่ต้องกรอกกะงาน |
| ข้อจำกัด | ต้องมีข้อมูลแผนผลิต | ถ้าเปิดระบบทิ้งไว้ Idle นาน ค่าจะถูกดึงลงเรื่อย ๆ |

ดังนั้นตอนสอบควรพูดตรง ๆ ว่า:

> “ในโปรเจคนี้ Planned Time คือเวลาสะสมตั้งแต่ Node-RED เริ่มคำนวณ ไม่ใช่เวลาแผนผลิตที่ผู้ใช้กรอก”

---

## 7. ความสัมพันธ์กับตัวแปรอื่นใน OEE

### Availability
\[
Availability = \frac{runningTimeSec}{totalPlannedTime} \times 100
\]

### Performance
ใช้ `runningTimeSec` ร่วมด้วย:
- Ideal Cycle Time = 5 วินาที/ชิ้น
- Expected Output = `runningTimeSec / 5`
- Performance = `total / expectedOutput * 100`

### Quality
ไม่ใช้ plannedTime โดยตรง:
- Quality = `good / total * 100`

### OEE รวม
\[
OEE = Availability \times Performance \times Quality
\]

---

## 8. จุดที่มักเข้าใจผิด

1. **คิดว่า plannedTime มากจาก ESP32**  
   ไม่ใช่ — ESP32 ส่งแค่ยอดนับ Total/Good/Waste

2. **คิดว่าต้องมีฟอร์มกรอกเวลาแผน**  
   เวอร์ชันปัจจุบันยังไม่มี ระบบคำนวณจากเวลาเปิดระบบ

3. **คิดว่าตอน IDLE แล้ว plannedTime หยุดเดิน**  
   ไม่หยุด — นาฬิการวมยังเดิน แต่ runningTime หยุดบวก

4. **คิดว่า Availability 100% ตลอดถ้าเครื่องเคยวิ่ง**  
   ไม่จริง — พอมีช่วงหยุดค้าง plannedTime จะโตต่อ ทำให้เปอร์เซ็นต์ลด

---

## 9. ถ้าจะพัฒนาให้ใกล้โรงงานจริง

แนวทางที่ทำได้ในอนาคต:
1. ให้ผู้ใช้ตั้งค่า `plannedShiftSeconds` เช่น 8 × 60 × 60
2. ใช้ค่านั้นเป็นตัวหารแทน `(now - startTime)`
3. หรือมีโหมด 2 แบบ:
   - โหมดสาธิต = ใช้เวลาเปิดระบบ
   - โหมดโรงงาน = ใช้เวลาแผนผลิต

ตัวอย่างแนวคิดโค้ด:

```js
// โหมดโรงงาน (ยังไม่ได้ใช้ในเวอร์ชันปัจจุบัน)
let plannedShiftSeconds = 8 * 60 * 60;
let availability = (runningTimeSec / plannedShiftSeconds) * 100;
```

---

## 10. ประโยคพร้อมใช้ตอนสอบ/รายงาน

**สั้นสุด**
> Planned Time ในระบบนี้คือระยะเวลาตั้งแต่ Node-RED เริ่มทำงานจนถึงปัจจุบัน คำนวณจาก `now - startTime`

**แบบอธิบายเต็ม**
> Availability คำนวณจาก Running Time หารด้วย Planned Time โดย Planned Time ได้จากเวลาที่ระบบเปิดอยู่จริง ส่วน Running Time สะสมเฉพาะช่วงที่มีชิ้นงานไหลผ่านภายใน 1 นาที หากเกิน 1 นาทีจะถือเป็น IDLE และไม่บวก Running Time

**แบบเทียบทฤษฎี**
> ตามทฤษฎี Planned Time ควรเป็นเวลาแผนผลิต แต่เพื่อให้ชุดสาธิตใช้งานง่าย โปรเจคนี้ใช้เวลาเปิดระบบเป็น Planned Time แทน

---

## 11. อ้างอิงภายในโปรเจค

- Flow คำนวณ: `node-red/flows_final_8.3v.json`
- Function name: `OEE Calculation Engine`
- เอกสารทฤษฎีรวม: [บทที่-02-ทฤษฎีและงานที่เกี่ยวข้อง.md](./บทที่-02-ทฤษฎีและงานที่เกี่ยวข้อง.md)
- เอกสารวิธีทำระบบ: [บทที่-03-วิธีการดำเนินงาน.md](./บทที่-03-วิธีการดำเนินงาน.md)
