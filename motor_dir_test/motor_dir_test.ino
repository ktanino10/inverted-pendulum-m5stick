#include <M5StickCPlus2.h>
void setup() {
  auto cfg = M5.config();
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(3);
  StickCP2.Display.fillScreen(BLACK);
  StickCP2.Display.setTextSize(3);
  StickCP2.Display.setTextColor(GREEN);
  StickCP2.Display.setCursor(30, 40);
  StickCP2.Display.println("STOPPED");
}
void loop() { delay(1000); }
