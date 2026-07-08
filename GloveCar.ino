/*
--Это код машинки

Проект Glove состоит из перчатки(манипулятора) и машинки (которым управляют)

Машинка подключается к серверу , которая находится в перчатке
Машинка получает строку типа "A0 B0 C0 D0 " где на месте 0 стоит число ШИМ которае подается на двигатель : A,B,C или D 

*/
#include <esp_now.h>
#include <WiFi.h>

#define FOR_N(n) for (int i = 0; i < (n); i++)

//пины логической схемы для управления драйвером двигателей
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

void motorWrite(int motor, int pwr){
  Serial.printf("motor=%d  pwr=%d\n", motor, pwr);
  if (motor == 1 || motor == 0) pwr = (-1) * pwr;
  if (pwr > 0) {
    digitalWrite(motorPin0[motor], 1);  // Ставим двигатель на поездку вперед
    digitalWrite(motorPin1[motor], 0);
  } else if (pwr < 0) {
    digitalWrite(motorPin0[motor], 0);  // Ставим двигатель на поездку назад
    digitalWrite(motorPin1[motor], 1);
  } else {
    digitalWrite(motorPin0[motor], 0);  // Ставим двигатель на тормоз
    digitalWrite(motorPin1[motor], 0);
  }

  if (pwr < 0) pwr = abs(pwr);
  ledcWrite(enablePin[motor], enablePin[motor] == ena2 ? pwr * 0.97 : pwr);
}

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&motorData, incomingData, sizeof(motorData));
  FOR_N(4) motorWrite(i, motorData.arr[i] * 2);
}

void setupMotors(){
  FOR_N(4) {
    pinMode(motorPin0[i], OUTPUT);
    digitalWrite(motorPin0[i], 0);

    pinMode(motorPin1[i], OUTPUT);
    digitalWrite(motorPin1[i], 0);

    // ledcAttach(enablePin[i], 40000, 8); 
    ledcAttach(enablePin[i], 5000, 8); // Пин, частота 5кГц, разрешение 8 бит
    ledcWrite(enablePin[i], 0);
  }
}

void setup(){
  Serial.begin(921600);
  Serial.println("Start!");

  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK){
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  setupMotors();

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(onDataRecv));
}

void loop() {
}