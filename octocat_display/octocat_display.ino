/*
 * GitHub Sticker Slideshow for M5StickC Plus2
 * Aボタンで4つのステッカーを切り替え表示
 * - Octocat
 * - Copilot
 * - Duck (ラバーダック)
 * - Mascot
 */

#include <M5StickCPlus2.h>
#include "octocat_img.h"
#include "copilot_img.h"
#include "duck_img.h"
#include "mascot_img.h"

#define IMG_W 110
#define IMG_H 110
#define BG    0x0861

const uint16_t* images[] = {octocat_img, copilot_img, duck_img, mascot_img};
const char* names[] = {"Octocat", "Copilot", "Duck", "Mascot"};
const int imgCount = 4;
int current = 0;

void showImage(int idx) {
  StickCP2.Display.fillScreen(BG);
  
  int ox = (240 - IMG_W) / 2;
  int oy = 0;
  
  StickCP2.Display.pushImage(ox, oy, IMG_W, IMG_H, images[idx]);
  
  // 名前
  StickCP2.Display.setTextSize(1);
  StickCP2.Display.setTextColor(WHITE);
  StickCP2.Display.setCursor(5, 118);
  StickCP2.Display.printf("[A] Next  ");
  StickCP2.Display.setTextColor(0xA11F);
  StickCP2.Display.printf("%s", names[idx]);
  StickCP2.Display.setTextColor(0x7BEF);
  StickCP2.Display.printf("  %d/%d", idx + 1, imgCount);
}

void setup() {
  auto cfg = M5.config();
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(3);
  pinMode(37, INPUT_PULLUP);
  showImage(0);
}

void loop() {
  StickCP2.update();
  
  if (digitalRead(37) == 0) {
    current = (current + 1) % imgCount;
    showImage(current);
    delay(300);  // デバウンス
  }
  
  delay(20);
}
