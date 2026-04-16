#include <M5StickCPlus2.h>
#include <ESP32Servo.h>

Servo servo;
bool running = false;

void setup() {
  auto cfg = M5.config();
  cfg.output_power = true;
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(3);
  Serial.begin(115200);
  
  servo.attach(0);
  servo.write(90);
  
  StickCP2.Display.fillScreen(BLACK);
  StickCP2.Display.setTextSize(2);
  StickCP2.Display.setTextColor(YELLOW);
  StickCP2.Display.setCursor(10, 10);
  StickCP2.Display.println("G0 Test");
  StickCP2.Display.setTextColor(RED);
  StickCP2.Display.setCursor(10, 40);
  StickCP2.Display.println("STOPPED");
  StickCP2.Display.setTextColor(WHITE);
  StickCP2.Display.setCursor(10, 80);
  StickCP2.Display.println("[B] Run/Stop");
}

void loop() {
  StickCP2.update();
  if (StickCP2.BtnB.wasReleased()) {
    running = !running;
    if (running) {
      servo.write(130);
      StickCP2.Display.fillRect(0, 40, 240, 25, BLACK);
      StickCP2.Display.setCursor(10, 40);
      StickCP2.Display.setTextColor(GREEN);
      StickCP2.Display.setTextSize(2);
      StickCP2.Display.println("RUNNING");
    } else {
      servo.write(90);
      StickCP2.Display.fillRect(0, 40, 240, 25, BLACK);
      StickCP2.Display.setCursor(10, 40);
      StickCP2.Display.setTextColor(RED);
      StickCP2.Display.setTextSize(2);
      StickCP2.Display.println("STOPPED");
    }
  }
  delay(20);
}
