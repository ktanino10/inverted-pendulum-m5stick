/*
 * IMU軸確認ツール
 * 生の加速度を画面にリアルタイム表示
 * 直立→前に傾ける→後ろに傾ける で
 * どの軸が + → - に変化するか確認
 */
#include <M5StickCPlus2.h>

void setup() {
  auto cfg = M5.config();
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(3);
  M5.Imu.begin();
  Serial.begin(115200);
}

void loop() {
  float gx, gy, gz, ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  
  StickCP2.Display.fillScreen(BLACK);
  StickCP2.Display.setTextSize(2);
  StickCP2.Display.setTextColor(YELLOW);
  StickCP2.Display.setCursor(5, 5);
  StickCP2.Display.println("IMU Axis Check");
  
  StickCP2.Display.setTextSize(3);
  StickCP2.Display.setTextColor(ax > 0.1 ? GREEN : (ax < -0.1 ? RED : WHITE));
  StickCP2.Display.setCursor(5, 30);
  StickCP2.Display.printf("X:%+.2f", ax);
  
  StickCP2.Display.setTextColor(ay > 0.1 ? GREEN : (ay < -0.1 ? RED : WHITE));
  StickCP2.Display.setCursor(5, 60);
  StickCP2.Display.printf("Y:%+.2f", ay);
  
  StickCP2.Display.setTextColor(az > 0.1 ? GREEN : (az < -0.1 ? RED : WHITE));
  StickCP2.Display.setCursor(5, 90);
  StickCP2.Display.printf("Z:%+.2f", az);
  
  StickCP2.Display.setTextSize(1);
  StickCP2.Display.setTextColor(CYAN);
  StickCP2.Display.setCursor(5, 120);
  StickCP2.Display.print("Tilt FWD/BWD - which changes?");
  
  delay(100);
}
