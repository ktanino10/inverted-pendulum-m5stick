/*
 * 統合キャリブレーションツール for M5StickC Plus2
 * 
 * 倒立振子のサーボ設定を段階的に確定するためのツール。
 * 3つのモードをAボタンで切り替え、Bボタンで操作。
 * 
 * Mode 1: ピンスキャン — どのGPIOでサーボが回るか確認
 * Mode 2: ニュートラル探索 — サーボが停止するPWM値を探す
 * Mode 3: 方向テスト — 両輪が前進する回転方向を確認
 */

#include <M5StickCPlus2.h>
#include <ESP32Servo.h>

// ========== テスト対象ピン ==========
const int testPins[] = {0, 26, 32, 33, 36};
const char* pinNames[] = {"G0", "G26", "G32", "G33", "G36"};
const int pinCount = 5;

// ========== 状態 ==========
enum Mode { PIN_SCAN, NEUTRAL_FIND, DIR_TEST };
Mode mode = PIN_SCAN;

Servo servo1, servo2;

// Pin Scan
int scanIdx = 0;
bool scanRunning = false;

// Neutral Find
int confirmedPin1 = -1;   // ユーザーが確定したピン1
int confirmedPin2 = -1;   // ユーザーが確定したピン2
int neutralVal = 90;
int whichServo = 1;       // 1 or 2
int neutral1 = 90, neutral2 = 90;

// Direction Test
int dirMode = 0;  // 0=stop, 1=+cmd/+cmd, 2=+cmd/-cmd
int testSpeed = 20;

// ========== 描画ヘルパー ==========
void clearScreen() {
  StickCP2.Display.fillScreen(BLACK);
  StickCP2.Display.setTextSize(2);
}

void drawHeader(const char* title) {
  StickCP2.Display.setTextColor(YELLOW);
  StickCP2.Display.setCursor(5, 5);
  StickCP2.Display.println(title);
  StickCP2.Display.drawFastHLine(0, 25, 240, DARKGREY);
}

// ========== Mode 1: ピンスキャン ==========
void drawPinScan() {
  clearScreen();
  drawHeader("1: Pin Scan");
  
  StickCP2.Display.setTextColor(WHITE);
  StickCP2.Display.setCursor(5, 30);
  StickCP2.Display.printf("Pin: %s (%d/%d)", pinNames[scanIdx], scanIdx+1, pinCount);
  
  StickCP2.Display.setCursor(5, 55);
  if (scanRunning) {
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.println("SPINNING");
  } else {
    StickCP2.Display.setTextColor(RED);
    StickCP2.Display.println("STOPPED");
  }
  
  StickCP2.Display.setTextColor(CYAN);
  StickCP2.Display.setTextSize(1);
  StickCP2.Display.setCursor(5, 85);
  StickCP2.Display.println("[A long] Next mode");
  StickCP2.Display.setCursor(5, 97);
  StickCP2.Display.println("[A short] Next pin");
  StickCP2.Display.setCursor(5, 109);
  StickCP2.Display.println("[B] Spin/Stop toggle");
  
  if (confirmedPin1 >= 0 || confirmedPin2 >= 0) {
    StickCP2.Display.setCursor(5, 125);
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.printf("OK: %s %s",
      confirmedPin1 >= 0 ? pinNames[confirmedPin1] : "--",
      confirmedPin2 >= 0 ? pinNames[confirmedPin2] : "--");
  }
}

void handlePinScan() {
  // Bボタン: スピン/停止トグル
  if (StickCP2.BtnB.wasReleased()) {
    scanRunning = !scanRunning;
    if (scanRunning) {
      servo1.attach(testPins[scanIdx]);
      servo1.write(130);  // 全開で回す
      Serial.printf("PIN SCAN: %s SPINNING (write=130)\n", pinNames[scanIdx]);
    } else {
      servo1.write(90);
      delay(20);
      servo1.detach();
      
      // このピンが使えるか聞く（回っていたなら確定）
      if (confirmedPin1 < 0) {
        confirmedPin1 = scanIdx;
        Serial.printf("PIN CONFIRMED #1: %s\n", pinNames[scanIdx]);
      } else if (confirmedPin2 < 0 && scanIdx != confirmedPin1) {
        confirmedPin2 = scanIdx;
        Serial.printf("PIN CONFIRMED #2: %s\n", pinNames[scanIdx]);
      }
      Serial.printf("PIN SCAN: %s STOPPED\n", pinNames[scanIdx]);
    }
    drawPinScan();
  }
  
  // Aボタン短押し: 次のピンへ
  if (StickCP2.BtnA.wasReleased()) {
    if (scanRunning) {
      servo1.write(90);
      delay(20);
      servo1.detach();
      scanRunning = false;
    }
    scanIdx = (scanIdx + 1) % pinCount;
    Serial.printf("PIN SCAN: switched to %s\n", pinNames[scanIdx]);
    drawPinScan();
  }
}

// ========== Mode 2: ニュートラル探索 ==========
void drawNeutralFind() {
  clearScreen();
  drawHeader("2: Neutral Find");
  
  StickCP2.Display.setTextColor(WHITE);
  StickCP2.Display.setCursor(5, 30);
  int pin = (whichServo == 1) ? confirmedPin1 : confirmedPin2;
  StickCP2.Display.printf("Servo%d (%s)", whichServo, 
    pin >= 0 ? pinNames[pin] : "??");
  
  StickCP2.Display.setCursor(5, 55);
  StickCP2.Display.setTextColor(GREEN);
  StickCP2.Display.setTextSize(3);
  StickCP2.Display.printf(" %3d", neutralVal);
  
  StickCP2.Display.setTextSize(1);
  StickCP2.Display.setTextColor(CYAN);
  StickCP2.Display.setCursor(5, 85);
  StickCP2.Display.println("[A long] Next mode");
  StickCP2.Display.setCursor(5, 97);
  StickCP2.Display.println("[A short] Value +1");
  StickCP2.Display.setCursor(5, 109);
  StickCP2.Display.println("[B] Value -1");
  
  StickCP2.Display.setCursor(5, 125);
  StickCP2.Display.setTextColor(YELLOW);
  StickCP2.Display.printf("Find value where servo STOPS");
}

void handleNeutralFind() {
  int pin = (whichServo == 1) ? confirmedPin1 : confirmedPin2;
  if (pin < 0) {
    // ピン未確定
    return;
  }
  
  // Aボタン: +1
  if (StickCP2.BtnA.wasReleased()) {
    neutralVal++;
    servo1.write(neutralVal);
    Serial.printf("NEUTRAL S%d (%s): %d\n", whichServo, pinNames[pin], neutralVal);
    drawNeutralFind();
  }
  
  // Bボタン: -1
  if (StickCP2.BtnB.wasReleased()) {
    neutralVal--;
    servo1.write(neutralVal);
    Serial.printf("NEUTRAL S%d (%s): %d\n", whichServo, pinNames[pin], neutralVal);
    drawNeutralFind();
  }
}

// ========== Mode 3: 方向テスト ==========
void drawDirTest() {
  clearScreen();
  drawHeader("3: Direction Test");
  
  StickCP2.Display.setTextColor(WHITE);
  StickCP2.Display.setCursor(5, 30);
  
  switch(dirMode) {
    case 0:
      StickCP2.Display.println("STOPPED");
      break;
    case 1:
      StickCP2.Display.println("S1:+  S2:+");
      StickCP2.Display.setCursor(5, 50);
      StickCP2.Display.println("(same direction)");
      break;
    case 2:
      StickCP2.Display.println("S1:+  S2:-");
      StickCP2.Display.setCursor(5, 50);
      StickCP2.Display.println("(mirror/reverse)");
      break;
  }
  
  StickCP2.Display.setTextSize(1);
  StickCP2.Display.setTextColor(CYAN);
  StickCP2.Display.setCursor(5, 75);
  StickCP2.Display.printf("N1=%d N2=%d spd=%d", neutral1, neutral2, testSpeed);
  StickCP2.Display.setCursor(5, 90);
  StickCP2.Display.println("[A long] Next mode");
  StickCP2.Display.setCursor(5, 102);
  StickCP2.Display.println("[A short] Next pattern");
  StickCP2.Display.setCursor(5, 114);
  StickCP2.Display.println("[B] Toggle speed 10/20/30");
  
  StickCP2.Display.setCursor(5, 130);
  StickCP2.Display.setTextColor(YELLOW);
  StickCP2.Display.println("Which goes FORWARD?");
}

void handleDirTest() {
  if (confirmedPin1 < 0 || confirmedPin2 < 0) return;
  
  // Aボタン: 次のパターン
  if (StickCP2.BtnA.wasReleased()) {
    dirMode = (dirMode + 1) % 3;
    switch(dirMode) {
      case 0:
        servo1.write(neutral1);
        servo2.write(neutral2);
        break;
      case 1:
        servo1.write(neutral1 + testSpeed);
        servo2.write(neutral2 + testSpeed);
        break;
      case 2:
        servo1.write(neutral1 + testSpeed);
        servo2.write(neutral2 - testSpeed);
        break;
    }
    Serial.printf("DIR TEST: mode=%d\n", dirMode);
    drawDirTest();
  }
  
  // Bボタン: 速度切り替え
  if (StickCP2.BtnB.wasReleased()) {
    if (testSpeed == 10) testSpeed = 20;
    else if (testSpeed == 20) testSpeed = 30;
    else testSpeed = 10;
    // 動いている場合は速度更新
    if (dirMode == 1) {
      servo1.write(neutral1 + testSpeed);
      servo2.write(neutral2 + testSpeed);
    } else if (dirMode == 2) {
      servo1.write(neutral1 + testSpeed);
      servo2.write(neutral2 - testSpeed);
    }
    Serial.printf("DIR TEST: speed=%d\n", testSpeed);
    drawDirTest();
  }
}

// ========== モード切替 ==========
unsigned long btnADownTime = 0;
bool btnAIsDown = false;

void switchMode(Mode newMode) {
  // 現モードのクリーンアップ
  servo1.detach();
  servo2.detach();
  
  mode = newMode;
  
  switch(mode) {
    case PIN_SCAN:
      scanRunning = false;
      drawPinScan();
      break;
      
    case NEUTRAL_FIND:
      whichServo = 1;
      neutralVal = 90;
      if (confirmedPin1 >= 0) {
        servo1.attach(testPins[confirmedPin1]);
        servo1.write(neutralVal);
      }
      drawNeutralFind();
      break;
      
    case DIR_TEST:
      dirMode = 0;
      if (confirmedPin1 >= 0 && confirmedPin2 >= 0) {
        servo1.attach(testPins[confirmedPin1]);
        servo2.attach(testPins[confirmedPin2]);
        servo1.write(neutral1);
        servo2.write(neutral2);
      }
      drawDirTest();
      break;
  }
  
  Serial.printf("=== MODE: %d ===\n", (int)mode);
}

// ========== setup / loop ==========
void setup() {
  auto cfg = M5.config();
  cfg.output_power = true;
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(3);
  Serial.begin(115200);
  
  Serial.println("===================================");
  Serial.println("  Servo Calibration Tool");
  Serial.println("  A long press = next mode");
  Serial.println("  A short = action");
  Serial.println("  B = action");
  Serial.println("===================================");
  
  drawPinScan();
}

void loop() {
  StickCP2.update();
  unsigned long now = millis();
  
  // 長押し検出（モード切替用）
  if (StickCP2.BtnA.isPressed() && !btnAIsDown) {
    btnADownTime = now;
    btnAIsDown = true;
  }
  if (!StickCP2.BtnA.isPressed() && btnAIsDown) {
    unsigned long held = now - btnADownTime;
    btnAIsDown = false;
    
    if (held >= 1000) {
      // 長押し → モード切替
      // ニュートラル探索モードでは、サーボ切替 or モード切替
      if (mode == NEUTRAL_FIND && whichServo == 1 && confirmedPin2 >= 0) {
        // サーボ1の値を保存してサーボ2へ
        neutral1 = neutralVal;
        whichServo = 2;
        neutralVal = 90;
        servo1.detach();
        servo1.attach(testPins[confirmedPin2]);
        servo1.write(neutralVal);
        Serial.printf("NEUTRAL S1 confirmed: %d, switching to S2\n", neutral1);
        drawNeutralFind();
      } else if (mode == NEUTRAL_FIND && whichServo == 2) {
        neutral2 = neutralVal;
        Serial.printf("NEUTRAL S2 confirmed: %d\n", neutral2);
        servo1.detach();
        switchMode(DIR_TEST);
      } else {
        // 通常のモード切替
        Mode next;
        if (mode == PIN_SCAN) next = NEUTRAL_FIND;
        else if (mode == NEUTRAL_FIND) next = DIR_TEST;
        else next = PIN_SCAN;
        switchMode(next);
      }
      return;  // 短押し処理をスキップ
    }
    // 短押しは各モードの handler で処理済み（wasReleased で検出）
  }
  
  // 各モードのハンドラ
  switch(mode) {
    case PIN_SCAN:    handlePinScan(); break;
    case NEUTRAL_FIND: handleNeutralFind(); break;
    case DIR_TEST:    handleDirTest(); break;
  }
  
  delay(20);
}
