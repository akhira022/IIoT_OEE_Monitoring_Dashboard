/*
 * โปรเจกต์: Conveyor Belt Model for Real-Time Overall Equipment Effectiveness (OEE) Evaluation + IoT Cloud
 * เวอร์ชัน: 8.3 (Fixed Compiler & HiveMQ TLS Connection) LASTUPDATE
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ==========================================
//  1. ตั้งค่าอินเทอร์เน็ต และ MQTT HiveMQ
// ==========================================
const char* ssid = "Sompit_2.4G"; 
const char* password = "0636572880";

const char* mqtt_server = "6fc975c703db4324859a6b7bd9c2d149.s1.eu.hivemq.cloud";
const int mqtt_port = 8883; 
const char* mqtt_user = "conveyor_admin";
const char* mqtt_pass = "Password123456";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ==========================================
//  2. กำหนดขาพินเซนเซอร์ IR (3 ตัว)
// ==========================================
const int PIN_IR_TOTAL = 2; // [ตัวที่ 1] นับยอดวัตถุเข้าสายพานทั้งหมด
const int PIN_IR_GOOD  = 4; // [ตัวที่ 2] นับชิ้นงานดีผ่าน QC
const int PIN_IR_WASTE = 5; // [ตัวที่ 3] นับชิ้นงานเสีย

// ==========================================
//  3. ตัวแปรระบบและการจับเวลา Debounce
// ==========================================
int countTotal = 0; 
int countGood  = 0;
int countWaste = 0;

unsigned long lastIrTotalTime = 0;
unsigned long lastIrGoodTime  = 0;
unsigned long lastIrWasteTime = 0;
const int debounceDelay = 150; // หน่วงเวลากันสัญญาณรบกวน (ms)

// ตัวแปรเก็บสถานะขาสัญญาณก่อนหน้า
bool lastStateTotal = HIGH;
bool lastStateGood  = HIGH;
bool lastStateWaste = HIGH;


// ฟังก์ชันส่งค่าเริ่มต้นขึ้น MQTT Cloud
void sendInitialTelemetry() {
  if (client.connected()) {
    client.publish("factory/conveyor/total", String(countTotal).c_str());
    client.loop();
    delay(50);
    client.publish("factory/conveyor/good",  String(countGood).c_str()); 
    client.loop();
    delay(50);
    client.publish("factory/conveyor/waste", String(countWaste).c_str()); 
    client.loop();
    Serial.println(" [MQTT] ส่งข้อมูล Telemetry เริ่มต้นขึ้น Cloud เรียบร้อย");
  }
}

// ฟังก์ชันตรวจสอบและต่อเชื่อม MQTT
void checkMQTTConnection() {
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    Serial.print("📡 [MQTT] กำลังเชื่อมต่อคลาวด์ HiveMQ...");
    // กำหนด Client ID แบบสุ่มป้องกัน ID ชนกันแล้วโดนตัดสาย
    String clientId = "ESP32_Conveyor_" + String(random(1000, 9999));
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" สำเร็จ! 🟢");
      sendInitialTelemetry();
    } else {
      Serial.print(" พลาด! รหัส Error: ");
      Serial.print(client.state());
      Serial.println(" จะลองใหม่ในอีก 5 วินาที");
    }
  }
}

void setup() {
  Serial.begin(115200);

  // ตั้งค่าขาพินเซนเซอร์ IR แบบ INPUT_PULLUP
  pinMode(PIN_IR_TOTAL, INPUT_PULLUP);
  pinMode(PIN_IR_GOOD,  INPUT_PULLUP);
  pinMode(PIN_IR_WASTE, INPUT_PULLUP);
  
  WiFi.setAutoReconnect(true); 

  Serial.println();
  Serial.print("กำลังเริ่มเชื่อมต่อ Wi-Fi...");
  WiFi.begin(ssid, password);
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi เชื่อมต่อสำเร็จ! 🟢");
  } else {
    Serial.println("\n⚠️ Wi-Fi เชื่อมต่อไม่สำเร็จ (ทำงานโหมด Offline)");
  }

  // ปรับแต่งการเชื่อมต่อ SSL/TLS สำหรับ HiveMQ Cloud
  espClient.setInsecure(); // ไม่ตรวจสอบ CA Certificate ป้องกัน SSL Fail
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(512); // ขยาย Buffer รองรับแพ็กเกจ TLS SSL
  
  checkMQTTConnection();

  // อ่านค่าสถานะแรกของเซนเซอร์เก็บไว้
  lastStateTotal = digitalRead(PIN_IR_TOTAL);
  lastStateGood  = digitalRead(PIN_IR_GOOD);
  lastStateWaste = digitalRead(PIN_IR_WASTE);

  Serial.println("--- 🟢 ระบบ v8.3 พร้อมทำงาน ---");
}

void loop() {
  // 1. ตรวจสอบ MQTT Connection ทุก 5 วินาที
  static unsigned long lastMqttCheck = 0;
  if (millis() - lastMqttCheck > 5000) { 
    checkMQTTConnection();
    lastMqttCheck = millis();
  }
  
  if (client.connected()) {
    client.loop(); 
  }

  unsigned long currentMillis = millis();

  // 2. อ่านค่าเซนเซอร์ทั้ง 3 ตัว
  bool currentStateTotal = digitalRead(PIN_IR_TOTAL);
  bool currentStateGood  = digitalRead(PIN_IR_GOOD);
  bool currentStateWaste = digitalRead(PIN_IR_WASTE);

  //  [เซนเซอร์ตัวที่ 1: TOTAL (ยอดรวม)]
  if (lastStateTotal == HIGH && currentStateTotal == LOW) { // จังหวะวัตถุตัดผ่านเซนเซอร์ (Falling Edge)
    if (currentMillis - lastIrTotalTime > debounceDelay) {
      countTotal++; 
      Serial.print("📦 วัตถุเข้าสายพาน! Total: ");
      Serial.print(countTotal);
      
      if (client.connected()) {
        bool ok = client.publish("factory/conveyor/total", String(countTotal).c_str());
        if (ok) Serial.println(" -> 🟢 [MQTT] ส่งสำเร็จ");
        else Serial.println(" -> 🔴 [MQTT] ส่งล้มเหลว!");
      } else {
        Serial.println(" -> ⚠️ [MQTT] ไม่ได้เชื่อมต่อ");
      }
      lastIrTotalTime = currentMillis;
    }
  }
  lastStateTotal = currentStateTotal;

  //  [เซนเซอร์ตัวที่ 2: GOOD (ของดี)]
  if (lastStateGood == HIGH && currentStateGood == LOW) {
    if (currentMillis - lastIrGoodTime > debounceDelay) {
      countGood++; 
      Serial.print("✅ ของดีผ่าน QC! Good: ");
      Serial.print(countGood);
      
      if (client.connected()) {
        bool ok = client.publish("factory/conveyor/good", String(countGood).c_str());
        if (ok) Serial.println(" -> 🟢 [MQTT] ส่งสำเร็จ");
        else Serial.println(" -> 🔴 [MQTT] ส่งล้มเหลว!");
      } else {
        Serial.println(" -> ⚠️ [MQTT] ไม่ได้เชื่อมต่อ");
      }
      lastIrGoodTime = currentMillis;
    }
  }
  lastStateGood = currentStateGood;

  //  [เซนเซอร์ตัวที่ 3: WASTE (ของเสีย)]
  if (lastStateWaste == HIGH && currentStateWaste == LOW) {
    if (currentMillis - lastIrWasteTime > debounceDelay) {
      countWaste++; 
      Serial.print("❌ บันทึกของเสีย! Waste: ");
      Serial.print(countWaste);
      
      if (client.connected()) {
        bool ok = client.publish("factory/conveyor/waste", String(countWaste).c_str());
        if (ok) Serial.println(" -> 🟢 [MQTT] ส่งสำเร็จ");
        else Serial.println(" -> 🔴 [MQTT] ส่งล้มเหลว!");
      } else {
        Serial.println(" -> ⚠️ [MQTT] ไม่ได้เชื่อมต่อ");
      }
      lastIrWasteTime = currentMillis;
    }
  }
  lastStateWaste = currentStateWaste;
}
