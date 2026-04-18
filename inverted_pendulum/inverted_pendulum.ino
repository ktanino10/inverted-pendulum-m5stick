/*
 * 倒立振子 (Inverted Pendulum) — RemoteXY BLE チューニング対応
 * for M5StickC Plus2 + FS90R サーボ × 2
 *
 * n_shinichi氏のオリジナルコードをベースに、
 * IMU軸とモーター方向を実機テスト済みの値に変更。
 * スマホ(RemoteXY)からBLEでリアルタイムPIDチューニング可能。
 *
 * Based on n_shinichi's Plus2 sketch:
 * https://n-shinichi.hatenablog.com/entry/2025/08/31/163903
 */

#include "EEPROM.h"
#include <M5StickCPlus2.h>
#include <Kalman.h>

#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>
#include <RemoteXY.h>
#define REMOTEXY_BLUETOOTH_NAME "M5Stick2_IP2"

#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 714 bytes
  { 255,4,0,217,0,195,2,19,0,0,0,0,31,2,126,200,200,80,1,1,
  5,0,67,0,251,127,10,0,253,202,6,64,8,201,68,30,53,25,100,85,
  3,115,70,36,8,94,178,1,16,80,0,73,0,68,0,83,112,100,0,4,
  22,115,9,90,70,76,130,7,160,2,26,12,7,158,25,25,0,2,25,7,
  255,30,26,78,111,46,48,0,49,32,58,32,83,112,101,101,100,95,99,97,
  108,0,50,32,58,32,107,80,0,51,32,58,32,107,73,0,52,32,58,32,
  107,68,0,53,32,58,32,107,115,112,100,0,54,32,58,32,107,100,115,116,
  0,78,111,46,55,0,78,111,46,56,0,78,111,46,57,0,49,48,32,58,
  32,80,105,116,99,104,95,111,102,102,115,101,116,50,0,49,49,32,58,32,
  109,111,116,111,114,95,111,102,102,115,101,116,76,0,49,50,32,58,32,109,
  111,116,111,114,95,111,102,102,115,101,116,82,0,78,111,46,49,51,0,78,
  111,46,49,52,0,78,111,46,49,53,0,78,111,46,49,54,0,78,111,46,
  49,55,0,78,111,46,49,56,0,78,111,46,49,57,0,78,111,46,50,48,
  0,111,112,116,105,111,110,32,50,49,0,111,112,116,105,111,110,32,50,50,
  0,111,112,116,105,111,110,32,50,51,0,111,112,116,105,111,110,32,50,52,
  0,111,112,116,105,111,110,32,50,53,0,111,112,116,105,111,110,32,50,54,
  0,111,112,116,105,111,110,32,50,55,0,111,112,116,105,111,110,32,50,56,
  0,111,112,116,105,111,110,32,50,57,0,111,112,116,105,111,110,32,51,48,
  0,111,112,116,105,111,110,32,51,49,0,111,112,116,105,111,110,32,51,50,
  0,111,112,116,105,111,110,32,51,51,0,111,112,116,105,111,110,32,51,52,
  0,111,112,116,105,111,110,32,51,53,0,111,112,116,105,111,110,32,51,54,
  0,111,112,116,105,111,110,32,51,55,0,111,112,116,105,111,110,32,51,56,
  0,111,112,116,105,111,110,32,51,57,0,111,112,116,105,111,110,32,52,48,
  0,111,112,116,105,111,110,32,52,49,0,111,112,116,105,111,110,32,52,50,
  0,111,112,116,105,111,110,32,52,51,0,111,112,116,105,111,110,32,52,52,
  0,111,112,116,105,111,110,32,52,53,0,111,112,116,105,111,110,32,52,54,
  0,111,112,116,105,111,110,32,52,55,0,111,112,116,105,111,110,32,52,56,
  0,111,112,116,105,111,110,32,52,57,0,111,112,116,105,111,110,32,53,48,
  0,111,112,116,105,111,110,32,53,49,0,111,112,116,105,111,110,32,53,50,
  0,111,112,116,105,111,110,32,53,51,0,111,112,116,105,111,110,32,53,52,
  0,111,112,116,105,111,110,32,53,53,0,111,112,116,105,111,110,32,53,54,
  0,111,112,116,105,111,110,32,53,55,0,111,112,116,105,111,110,32,53,56,
  0,111,112,116,105,111,110,32,53,57,0,111,112,116,105,111,110,32,54,48,
  0,111,112,116,105,111,110,32,54,49,0,111,112,116,105,111,110,32,54,50,
  0,111,112,116,105,111,110,32,54,51,0,111,112,116,105,111,110,32,54,52,
  0,5,201,30,150,150,255,2,86,86,3,2,26,31 };

struct {
    int8_t slider_01;
    uint8_t Select_01;
    int8_t joystick_01_x;
    int8_t joystick_01_y;
    char text_1[201];
    float onlineGraph_01_var1;
    float onlineGraph_01_var2;
    float onlineGraph_01_var3;
    float onlineGraph_01_var4;
    uint8_t connect_flag;
} RemoteXY;
#pragma pack(pop)

// ============================================================
//  ハードウェア設定
// ============================================================
#define MOTOR_PIN_L 0
#define MOTOR_PIN_R 26
#define BTN_A 37
#define BTN_B 39
#define M5_LED 19
#define INIT_FLAG_ADDR 0
#define INIT_MAGIC     0xA5A5

// ============================================================
//  PIDパラメータ
// ============================================================
int motor_offsetL = 0, motor_offsetR = 0;
int16_t motor_init_L = 1500, motor_init_R = 1500;
float kpower = 0.003;
float kp = 30.0;
float ki = 3.0;
float kd = 2.0;
float kspd = 5.0;
float kdst = 0.14;
float gain[10];
float Kyaw = 10.0;
float Kyawtodeg = 0.069;
float Kspin = 0.0;
float Pitch_offset = 0, Pitch_offset2 = 0.0, Pitch_power = 0.0;
int Pitch_offset2_address = 4, motor_offsetL_address = 8, motor_offsetR_address = 12;
int fil_N = 5;

// ============================================================
//  制御変数
// ============================================================
Kalman kalman;
long lastMs = 0;
float acc[3], accOffset[3];
float gyro[3], gyroOffset[3];
float Pitch, yaw, Pitch_filter;
int wait_count, sec_count;
unsigned char motor_sw, servo_offset_sw;
int16_t power, powerL, powerR;
unsigned long ms10, ms100, ms1000;
float Speed, yawAng, yawAngx10;
float Angle, dAngle, k_speed, P_Angle, I_Angle, D_Angle;
int joy_x, joy_y;
int8_t selector_No1, select_val, slider_val;
float tmp_gain;
float batt;

// ============================================================
//  IMU
// ============================================================
void readGyro() {
  float gx, gy, gz, ax, ay, az;
  M5.Imu.getGyro(&gx, &gy, &gz);
  M5.Imu.getAccel(&ax, &ay, &az);
  gyro[0] = gx; gyro[1] = gy; gyro[2] = gz;
  acc[0] = ax;  acc[1] = ay;  acc[2] = az;
  dAngle = (gyro[2] - gyroOffset[2]);
  yawAngx10 += (gyro[1] - gyroOffset[1]) * 0.001;
  yawAng = yawAngx10 * Kyawtodeg;
}

void calibration() {
  float gyroSum[3] = {0}, accSum[3] = {0};
  for (int i = 0; i < 500; i++) {
    readGyro();
    for (int j = 0; j < 3; j++) {
      gyroSum[j] += gyro[j];
      accSum[j] += acc[j];
    }
    delay(2);
  }
  for (int j = 0; j < 3; j++) {
    gyroOffset[j] = gyroSum[j] / 500;
    accOffset[j] = accSum[j] / 500;
  }
  accOffset[2] -= 1.0;
}

void applyCalibration() {
  for (int i = 0; i < 3; i++) {
    gyro[i] -= gyroOffset[i];
    acc[i] -= accOffset[i];
  }
}

float getPitch() {
  // 実機テスト済み: Z軸が前後傾き
  float val = constrain(acc[2], -1.0, 1.0);
  return asin(val) * RAD_TO_DEG;
}

// ============================================================
//  角度取得
// ============================================================
void get_Angle() {
  readGyro();
  applyCalibration();
  float kalman_dt = (micros() - lastMs) / 1000000.0;
  lastMs = micros();
  Pitch = kalman.getAngle(getPitch(), gyro[2], kalman_dt) + Pitch_offset + Pitch_offset2 + Pitch_power;
  Pitch_filter = (Pitch + Pitch_filter * (fil_N - 1)) / fil_N;
  Angle = Pitch_filter;
}

// ============================================================
//  サーボ駆動
// ============================================================
void pulse_drive(int16_t pL, int16_t pR) {
  pL = constrain(pL, 500, 2500);
  pR = constrain(pR, 500, 2500);
  bool doneL = false, doneR = false;
  uint32_t usec = micros();
  digitalWrite(MOTOR_PIN_L, HIGH);
  digitalWrite(MOTOR_PIN_R, HIGH);
  while (!doneL || !doneR) {
    uint32_t w = micros() - usec;
    if (w >= (uint32_t)pL) { digitalWrite(MOTOR_PIN_L, LOW); doneL = true; }
    if (w >= (uint32_t)pR) { digitalWrite(MOTOR_PIN_R, LOW); doneR = true; }
  }
}

void servo_stop() {
  digitalWrite(MOTOR_PIN_L, LOW);
  digitalWrite(MOTOR_PIN_R, LOW);
  powerL = motor_init_L + motor_offsetL;
  powerR = motor_init_R + motor_offsetR;
}

// ============================================================
//  PID制御
// ============================================================
void PID_reset() {
  Pitch_power = wait_count = power = Speed = yawAng = yawAngx10 = I_Angle = 0;
}

void PID_ctrl() {
  Speed += kpower * gain[1] * power;
  P_Angle = -kp * gain[2] * Angle;
  I_Angle += -ki * gain[3] * Angle - kdst * gain[6] * Speed;
  D_Angle = -kd * gain[4] * dAngle;
  k_speed = -kspd * gain[5] * Speed;

  power = P_Angle + I_Angle + D_Angle + k_speed;

  if (I_Angle > 300) { power = Speed = I_Angle = Pitch_power = 0; }
  if (I_Angle < -300) { power = Speed = I_Angle = Pitch_power = 0; }

  if (motor_sw == 1) {
    if (Kspin != 0) { yawAng = 1; yawAngx10 = 0; }

    // 実機テスト済み: +power/-power が前進
    powerL =  power + motor_offsetL + motor_init_L - int16_t(Kyaw * yawAng) + Kspin;
    powerR = -power + motor_offsetR + motor_init_R - int16_t(Kyaw * yawAng) + Kspin;

    pulse_drive(powerL, powerR);
  }
  else {
    digitalWrite(MOTOR_PIN_L, LOW);
    digitalWrite(MOTOR_PIN_R, LOW);
  }
}

// ============================================================
//  RemoteXY操作
// ============================================================
void RemoteXY_ctrl() {
  RemoteXY_Handler();
  joy_x = RemoteXY.joystick_01_x;
  joy_y = RemoteXY.joystick_01_y;

  Kspin = 0;
  if (joy_x > 20) Kspin = (joy_x - 20) * 0.5;
  if (joy_x < -20) Kspin = (joy_x + 20) * 0.5;

  if (joy_y > 20) Pitch_power = Pitch_power + (joy_y - 20) * 0.002;
  if (joy_y < -20) Pitch_power = Pitch_power + (joy_y + 20) * 0.002;

  slider_val = RemoteXY.slider_01;
  select_val = RemoteXY.Select_01;

  if (select_val < 10) {
    if (slider_val < -5) tmp_gain = (float)(slider_val + 100) / 95.0;
    if (slider_val > -5) tmp_gain = 1.0;
    if (slider_val > 5) tmp_gain = (slider_val) * 0.09 + 1.0;
    gain[select_val] = tmp_gain;
  }
  else {
    if (select_val == 10) Pitch_offset2 = (float)(slider_val * 0.05);
    if (select_val == 11) motor_offsetL = (int)slider_val;
    if (select_val == 12) motor_offsetR = (int)slider_val;
  }

  snprintf(RemoteXY.text_1, 200,
    "v2 %3d  Scal:%3.1f     P:%3.1f    I:%3.1f    D:%3.1f     Spd;%3.1f     dst:%3.1f     Pof:%3.1f    L:%3d   R:%3d",
    sec_count, gain[1], gain[2], gain[3], gain[4], gain[5], gain[6], Pitch_offset2, motor_offsetL, motor_offsetR);

  RemoteXY.onlineGraph_01_var1 = constrain(P_Angle, -300, 300);
  RemoteXY.onlineGraph_01_var2 = constrain(I_Angle, -300, 300);
  RemoteXY.onlineGraph_01_var3 = constrain(D_Angle, -300, 300);
  RemoteXY.onlineGraph_01_var4 = constrain(k_speed, -300, 300);
}

// ============================================================
//  ディスプレイ
// ============================================================
void display_ctrl() {
  M5.Lcd.setCursor(0, 25);
  if (servo_offset_sw == 0) {
    if (motor_sw == 1) M5.Lcd.printf("on    ");
    else M5.Lcd.printf("off   ");
    M5.Lcd.printf(" %5.1f   ", Angle);
  }
  else {
    M5.Lcd.printf("offset     ");
    EEPROM.writeInt(Pitch_offset2_address, (int)(Pitch_offset2 * 100));
    EEPROM.writeInt(motor_offsetL_address, motor_offsetL);
    EEPROM.writeInt(motor_offsetR_address, motor_offsetR);
    EEPROM.commit();
  }

  M5.Lcd.setCursor(0, 50);
  M5.Lcd.printf("%3.1fv ", batt);
  M5.Lcd.printf("  %4.1f    ", yawAng);
}

// ============================================================
//  setup
// ============================================================
void setup() {
  M5.begin();
  M5.Lcd.setTextFont(4);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 50);

  if (!EEPROM.begin(100)) M5.Lcd.printf("EEPROM init failed!");
  int flag = EEPROM.readUShort(INIT_FLAG_ADDR);
  if (flag != INIT_MAGIC) {
    M5.Lcd.printf("No_init!");
    delay(2000);
    EEPROM.writeInt(INIT_FLAG_ADDR, INIT_MAGIC);
    EEPROM.writeInt(Pitch_offset2_address, 0);
    EEPROM.writeInt(motor_offsetL_address, 0);
    EEPROM.writeInt(motor_offsetR_address, 0);
    EEPROM.commit();
  }
  Pitch_offset2 = EEPROM.readInt(Pitch_offset2_address) * 0.01;
  motor_offsetL = EEPROM.readInt(motor_offsetL_address);
  motor_offsetR = EEPROM.readInt(motor_offsetR_address);

  RemoteXY_Init();

  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(M5_LED, OUTPUT);
  pinMode(MOTOR_PIN_L, OUTPUT);
  pinMode(MOTOR_PIN_R, OUTPUT);

  M5.Imu.begin();

  M5.Lcd.setCursor(0, 60);
  digitalWrite(M5_LED, HIGH);
  M5.Lcd.println("Calibrating");
  delay(500);

  calibration();

  readGyro();
  kalman.setAngle(getPitch());
  lastMs = micros();
  digitalWrite(M5_LED, LOW);

  M5.Lcd.setRotation(2);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("v2 BLE");

  for (int i = 1; i <= 6; i++) gain[i] = 1.0;

  ms10 = ms100 = ms1000 = millis();
  servo_stop();

  Serial.begin(115200);
  Serial.println("=== Inverted Pendulum + RemoteXY BLE ===");
}

// ============================================================
//  loop
// ============================================================
void loop() {
  get_Angle();

  if (millis() > ms10) {
    if (servo_offset_sw == 0) {
      if (-30 < Pitch_filter && Pitch_filter < 30) {
        wait_count += 1;
        if (wait_count > 200) {
          PID_ctrl();
        }
      }
      else {
        PID_reset();
      }
    }
    else {
      servo_stop();
    }
    ms10 += 10;
  }

  if (millis() > ms100) {
    RemoteXY_ctrl();
    display_ctrl();
    ms100 += 100;
  }

  if (millis() > ms1000) {
    sec_count++;
    batt = M5.Power.getBatteryVoltage() / 1000.0;
    if (digitalRead(BTN_A) == 0) motor_sw = !motor_sw;
    if (digitalRead(BTN_B) == 0) servo_offset_sw = !servo_offset_sw;
    ms1000 += 1000;
  }
}
