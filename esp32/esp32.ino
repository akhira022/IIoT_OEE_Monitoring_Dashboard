/*
 * โปรเจกต์: สายพานลำเลียงอัจฉริยะ (Smart Conveyor Belt) + IoT Cloud
 * เวอร์ชัน: 3.2 (อัปเดตคอมเมนต์อธิบายฟีเจอร์ระบบแยกของเสีย Manual QC และช่อง Defect Chute)
 * ⚙️ สิ่งที่เพิ่ม/ปรับปรุงในเวอร์ชันนี้:
 * 1. กำกับคอมเมนต์อธิบาย "เซนเซอร์ตัวที่ 1 (ต้นสายพาน)" สำหรับนับยอดรวมทั้งหมด (Total)
 * 2. กำกับคอมเมนต์อธิบาย "เซนเซอร์ตัวที่ 2 (ช่องของเสีย Defect Chute)" สำหรับนับยอดของเสีย (Waste/Defect)
 * 3. คงระบบกู้คืนเครือข่ายอัจฉริยะและระบบส่งข้อมูลอัตโนมัติทุก 5 วินาที (Heartbeat) ไว้ตามเดิม
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ==========================================
// 🌐 1. ตั้งค่าอินเทอร์เน็ต และ MQTT
// ==========================================
const char* ssid = "ใส่ชื่อ_WIFI_ของคุณตรงนี้"; 
const char* password = "ใส่รหัส_WIFI_ของคุณตรงนี้";

const char* mqtt_server = "6fc975c703db4324859a6b7bd9c2d149.s1.eu.hivemq.cloud";
const int mqtt_port = 8883; 
const char* mqtt_user = "conveyor_admin";
const char* mqtt_pass = "Password123456";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ==========================================
// ⚙️ 2. กำหนดขาพิน (ตรงตามตำแหน่งหน้างานจริง)
// ==========================================
const int PIN_IR_TOTAL  = 32; // [เซนเซอร์ตัวที่ 1] ติดตั้งที่ต้นสายพาน เพื่อจับวัตถุทุกชิ้นก้าวเข้าสู่ระบบ (นับยอดรวม Total)
const int PIN_IR_WASTE  = 33; // [เซนเซอร์ตัวที่ 2] ติดตั้งที่สไลด์ช่องของเสีย (Defect Chute) เมื่อพนักงานปัดของเสียลงมาจะนับทันที
const int PIN_RELAY     = 25; // ควบคุมมอเตอร์สายพาน
const int PIN_BTN_START = 26; // ปุ่มสีเขียว START
const int PIN_BTN_STOP  = 27; // ปุ่มสีแดง STOP
const int PIN_BTN_RESET = 14; // ปุ่มสีเหลือง/น้ำเงิน RESET

// ==========================================
// 📊 3. ตัวแปรระบบและการจับเวลา
// ==========================================
int countTotal = 0; 
int countWaste = 0; // ยอดนับของเสีย (Defect Count)
int systemState = 0; 

unsigned long lastIrTotalTime = 0;
unsigned long lastIrWasteTime = 0;
unsigned long lastBtnTime = 0;
const int debounceDelay = 300; 

unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 5000; // ส่งข้อมูลรายงานตัวขึ้น Cloud อัตโนมัติทุกๆ 5 วินาที

// ==========================================
// 🔄 4. ฟังก์ชันจัดการระบบเชื่อมต่อ MQTT
// ==========================================
void checkMQTTConnection() {
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    Serial.print("📡 [MQTT] กำลังเชื่อมต่อคลาวด์ HiveMQ...");
    if (client.connect("ESP32_Conveyor", mqtt_user, mqtt_pass)) {
      Serial.println(" สำเร็จ! 🟢");
    } else {
      Serial.print(" พลาด! รหัส Error: ");
      Serial.print(client.state());
      Serial.println(" จะลองใหม่ในอีก 5 วินาที");
    }
  }
}

// ฟังก์ชันมัดรวมส่งข้อมูลขึ้น Cloud
void sendDataToCloud() {
  client.publish("factory/conveyor/total", String(countTotal).c_str());
  client.publish("factory/conveyor/waste", String(countWaste).c_str()); // ส่งค่าของเสียไปให้ Node-RED หักลบหาค่าของดีต่อไป
  client.publish("factory/conveyor/status", String(systemState).c_str());
  Serial.println("🚀 [MQTT] ส่งข้อมูลชุด Telemetry ขึ้น Cloud เรียบร้อย");
}

// ==========================================
// 🚀 5. ฟังก์ชันตั้งค่าเริ่มต้น (Setup)
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_IR_TOTAL, INPUT);
  pinMode(PIN_IR_WASTE, INPUT);
  pinMode(PIN_BTN_START, INPUT_PULLUP); 
  pinMode(PIN_BTN_STOP, INPUT_PULLUP);  
  pinMode(PIN_BTN_RESET, INPUT_PULLUP); 
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW); 

  WiFi.setAutoReconnect(true); // เปิดระบบกู้คืน Wi-Fi อัตโนมัติเบื้องหลัง

  Serial.println();
  Serial.print("กำลังเริ่มเชื่อมต่อ Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi เชื่อมต่อสำเร็จ! 🟢");

  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  
  Serial.println("--- 🟢 ระบบ v3.2 พร้อมสาธิตกระบวนการแยกของเสีย Manual QC แล้ว ---");
}

// ==========================================
// 🔁 6. ฟังก์ชันหลัก (Loop)
// ==========================================
void loop() {
  static unsigned long lastMqttCheck = 0;
  if (millis() - lastMqttCheck > 5000) { 
    checkMQTTConnection();
    lastMqttCheck = millis();
  }
  
  if (client.connected()) {
    client.loop(); 
  }

  unsigned long currentMillis = millis();

  // 1. ระบบส่งข้อมูลอัตโนมัติทุก 5 วินาที (Heartbeat) เพื่อสถิติ 720 ชุดข้อมูล/ชั่วโมง
  if (currentMillis - lastTelemetryTime >= telemetryInterval) {
    if (client.connected()) {
      sendDataToCloud(); 
    }
    lastTelemetryTime = currentMillis; 
  }

  // 2. ระบบจัดการปุ่มกด (Start / Stop / Reset)
  if (currentMillis - lastBtnTime > debounceDelay) {
    if (digitalRead(PIN_BTN_START) == LOW && systemState == 0) {
      systemState = 1;
      digitalWrite(PIN_RELAY, HIGH); 
      Serial.println("▶️ START: มอเตอร์ทำงาน สายพานเริ่มหมุน");
      if (client.connected()) sendDataToCloud(); 
      lastBtnTime = currentMillis;
    }
    if (digitalRead(PIN_BTN_STOP) == LOW && systemState == 1) {
      systemState = 0;
      digitalWrite(PIN_RELAY, LOW); 
      Serial.println("⏸️ STOP: หยุดสายพานชั่วคราว");
      if (client.connected()) sendDataToCloud(); 
      lastBtnTime = currentMillis;
    }
    if (digitalRead(PIN_BTN_RESET) == LOW && systemState == 0) {
      countTotal = 0;
      countWaste = 0;
      Serial.println("🔄 RESET: ล้างค่าตัวนับทั้งหมดเป็น 0");
      if (client.connected()) sendDataToCloud(); 
      lastBtnTime = currentMillis;
    }
  }

  // 3. ระบบอ่านเซนเซอร์นับชิ้นงาน (จะทำงานเมื่อสั่ง START เปิดสายพานเท่านั้น)
  if (systemState == 1) {
    
    // [เซนเซอร์ตัวที่ 1 - ต้นสายพาน] นับวัตถุทุกชิ้นที่ปล่อยลงมา
    if (digitalRead(PIN_IR_TOTAL) == LOW) {
      if (currentMillis - lastIrTotalTime > debounceDelay) {
        countTotal++; 
        Serial.print("📦 วัตถุเข้าสายพาน! ยอดรวม (Total): ");
        Serial.println(countTotal);
        
        if (client.connected()) {
          client.publish("factory/conveyor/total", String(countTotal).c_str());
        }
        lastIrTotalTime = currentMillis;
      }
    }

    // [เซนเซอร์ตัวที่ 2 - ช่องของเสีย Defect Chute] นับเฉพาะชิ้นงานที่พนักงาน QC ปัดแยกออกมา
    if (digitalRead(PIN_IR_WASTE) == LOW) {
      if (currentMillis - lastIrWasteTime > debounceDelay) {
        countWaste++; 
        Serial.print("⚠️ พนักงาน QC พบของเสีย! ปัดลงช่อง Defect ยอดสะสม: ");
        Serial.println(countWaste);
        
        if (client.connected()) {
          client.publish("factory/conveyor/waste", String(countWaste).c_str());
        }
        lastIrWasteTime = currentMillis;
      }
    }
  }
}