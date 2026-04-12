/*
 * FS90R ピン探索テスト for M5StickC Plus
 *
 * 全ピン(G0, G26, G25, G36)を順番に試して
 * どのピンにサーボが繋がっているか確認するスケッチ
 *
 * Aボタン: 次のピンに切り替え
 * Bボタン: 現在のピンでサーボ正転/停止トグル
 */

#include <M5StickCPlus.h>
#include <ESP32Servo.h>

const int pins[] = {0, 26, 25, 36, 32, 33};
const char* pinNames[] = {"G0", "G26", "G25", "G36", "G32", "G33"};
const int pinCount = 6;

int currentPin = 0;
bool running = false;
Servo servo;

void updateDisplay() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);

  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("Pin Finder");

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.printf("Pin: %s", pinNames[currentPin]);

  M5.Lcd.setCursor(10, 65);
  if (running) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("RUNNING");
  } else {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("STOPPED");
  }

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(10, 100);
  M5.Lcd.println("[A] Next Pin");
  M5.Lcd.setCursor(10, 125);
  M5.Lcd.println("[B] Run/Stop");
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  Serial.begin(115200);

  servo.attach(pins[currentPin]);
  servo.write(90);
  updateDisplay();
  Serial.printf("Testing pin: %s\n", pinNames[currentPin]);
}

void loop() {
  M5.update();

  // Aボタン: 次のピンへ
  if (M5.BtnA.wasPressed()) {
    servo.write(90);
    servo.detach();
    running = false;

    currentPin = (currentPin + 1) % pinCount;
    servo.attach(pins[currentPin]);
    servo.write(90);

    updateDisplay();
    Serial.printf("Switched to pin: %s\n", pinNames[currentPin]);
  }

  // Bボタン: 回転/停止トグル
  if (M5.BtnB.wasPressed()) {
    running = !running;
    if (running) {
      servo.write(130);
      Serial.printf("Pin %s: RUNNING\n", pinNames[currentPin]);
    } else {
      servo.write(90);
      Serial.printf("Pin %s: STOPPED\n", pinNames[currentPin]);
    }
    updateDisplay();
  }

  delay(20);
}
