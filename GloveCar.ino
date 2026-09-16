#include <esp_now.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>

#define FOR_N(n) for (int i = 0; i < (n); i++)

#define ENABLE_DEBUG_LOG 0
#define ENABLE_UDP 1

#define ena1 14
#define in11 26
#define in21 27

#define enb1 32
#define in31 13
#define in41 33

#define ena2 5
#define in12 18
#define in22 19

#define enb2 23
#define in32 22
#define in42 12
WiFiUDP udp;
const char* ssid = "CarDrone";
const char* password = "";

unsigned int localUdpPort = 4210;
const int WIFI_CHANNEL = 3;

int motorPin0[4] = { in32, in12, in31, in11 };
int motorPin1[4] = { in42, in22, in41, in21 };
int enablePin[4] = { enb2, ena2, enb1, ena1 };

union MotorData {
  struct {
    signed char a, b, c, d;
  };
  signed char arr[4];
};
MotorData motorData;

//For log output
bool isInfoWritten = false;
unsigned long bootStartTime = 0;
String bootInfo = ""; 

void motorWrite(int motor, int pwr) {
  if (motor == 1 || motor == 0) pwr = (-1) * pwr;
  if (pwr > 0) {
    digitalWrite(motorPin0[motor], 1);
    digitalWrite(motorPin1[motor], 0);
  } else if (pwr < 0) {
    digitalWrite(motorPin0[motor], 0);
    digitalWrite(motorPin1[motor], 1);
  } else {
    digitalWrite(motorPin0[motor], 0);
    digitalWrite(motorPin1[motor], 0);
  }

  if (pwr < 0) pwr = abs(pwr);
  ledcWrite(enablePin[motor], enablePin[motor] == ena2 ? pwr * 0.97 : pwr);
}

// Прием ESP-NOW
void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(motorData)) {
    memcpy(&motorData, incomingData, sizeof(motorData));
    FOR_N(4) motorWrite(i, motorData.arr[i] * 2);

#if ENABLE_DEBUG_LOG
    Serial.printf("[ESP-NOW] Motors: A:%d B:%d C:%d D:%d\n", 
      motorData.a * 2, motorData.b * 2, motorData.c * 2, motorData.d * 2);
#endif
  }
}

void setupMotors() {
  FOR_N(4) {
    pinMode(motorPin0[i], OUTPUT);
    digitalWrite(motorPin0[i], 0);

    pinMode(motorPin1[i], OUTPUT);
    digitalWrite(motorPin1[i], 0);

    ledcAttach(enablePin[i], 5000, 8);
    ledcWrite(enablePin[i], 0);
  }
}

void setup() {
  Serial.begin(921600);
  bootStartTime = millis();

  setupMotors();

  //Modem sleep off
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);

  //Wi-Fi init
  WiFi.mode(WIFI_AP_STA);
#if ENABLE_UDP
  WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, 4);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  bootInfo += "\n================ BOOT LOG ================\n";
  bootInfo += "Access point: " + String(ssid) + "\n";
  bootInfo += "IP address:   " + WiFi.softAPIP().toString() + "\n";
  bootInfo += "Wi-Fi ch:     " + String(WIFI_CHANNEL) + "\n";

  //UDP init
  udp.begin(localUdpPort);
  bootInfo += "UDP server:   Port " + String(localUdpPort) + "\n";
#endif

  //ESP-NOW init
  if (esp_now_init() != ESP_OK) {
    bootInfo += "ESP-NOW Status: FAILED!\n";
    bootInfo += "==========================================\n";
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(onDataRecv));
  bootInfo += "ESP-NOW Status: OK\n";
  bootInfo += "==========================================\n";
}

void processUDP() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;

    int valA = 0, valB = 0, valC = 0, valD = 0;
    if (sscanf(packetBuffer, "A%d B%d C%d D%d", &valA, &valB, &valC, &valD) == 4) {
      motorWrite(0, valA);
      motorWrite(1, valB);
      motorWrite(2, valC);
      motorWrite(3, valD);

#if ENABLE_DEBUG_LOG
      Serial.printf("[UDP] Motors: A:%d B:%d C:%d D:%d\n", valA, valB, valC, valD);
#endif
    }
    else{
#if ENABLE_DEBUG_LOG
      Serial.printf("Custom packet came: %s\n", packetBuffer);
#endif
    }
  }
}

void loop() {
#if ENABLE_UDP
  if (!isInfoWritten && (millis() - bootStartTime >= 3000)) {
    Serial.println(bootInfo);
    isInfoWritten = true;
  }
  processUDP();
#endif
}