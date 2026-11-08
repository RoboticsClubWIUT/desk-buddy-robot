// ШАГ 2: эмоции на экране (OLED 7 pin, SPI). Датчик пока не нужен.
// Робот сам меняет эмоцию каждые 4 секунды.
//
// Подключение (ESP32-C3 Super Mini):
//   OLED GND -> GND
//   OLED VCC -> 3V3
//   OLED D0 (SCK)  -> GPIO 4
//   OLED D1 (MOSI) -> GPIO 6
//   OLED RES -> GPIO 10
//   OLED DC  -> GPIO 3
//   OLED CS  -> GPIO 7
//
// Библиотеки: Adafruit GFX, Adafruit SSD1306, FluxGarage RoboEyes

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

#define OLED_SCK  4
#define OLED_MOSI 6
#define OLED_CS   7
#define OLED_DC   3
#define OLED_RST  10

Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RST, OLED_CS);  // экран 128x64 по SPI
RoboEyes<Adafruit_SSD1306> roboEyes(display);   // глаза рисуются на этом экране

byte emotion = 0;                // номер текущей эмоции
unsigned long lastChange = 0;    // когда последний раз меняли эмоцию

// Сбрасываем все эффекты, чтобы они не "залипали" между эмоциями
void resetFace() {
  roboEyes.setPosition(DEFAULT);      // взгляд прямо
  roboEyes.setIdleMode(ON, 2, 2);     // глаза иногда смотрят по сторонам
  roboEyes.setHFlicker(OFF, 0);       // без тряски
  roboEyes.setSweat(OFF);             // без капли пота
}

// Включаем эмоцию по номеру
void showEmotion(byte n) {
  resetFace();
  switch (n) {
    case 0:                            // обычное лицо
      roboEyes.setMood(DEFAULT);
      break;
    case 1:                            // радость
      roboEyes.setMood(HAPPY);
      roboEyes.anim_laugh();           // смех (один раз)
      break;
    case 2:                            // грусть
      roboEyes.setMood(TIRED);         // опущенные веки
      roboEyes.setPosition(S);         // взгляд вниз
      roboEyes.setIdleMode(OFF, 2, 2); // глаза не бегают
      break;
    case 3:                            // злость
      roboEyes.setMood(ANGRY);
      roboEyes.setHFlicker(ON, 3);     // глаза трясутся
      break;
    case 4:                            // удивление / растерянность
      roboEyes.setMood(DEFAULT);
      roboEyes.setSweat(ON);           // капля пота
      roboEyes.anim_confused();
      break;
  }
}

void setup() {
  SPI.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);   // говорим ESP32, какие пины для SPI
  display.begin(SSD1306_SWITCHCAPVCC);           // запускаем экран
  roboEyes.begin(128, 64, 60);                   // запускаем глаза (ширина, высота, кадров/с)
  roboEyes.setAutoblinker(ON, 3, 2);             // моргают сами
  showEmotion(0);
}

void loop() {
  roboEyes.update();                             // рисует глаза (должно быть в каждом цикле)

  if (millis() - lastChange > 4000) {            // прошло 4 секунды
    lastChange = millis();
    emotion = (emotion + 1) % 5;                 // следующая эмоция (0..4, потом снова 0)
    showEmotion(emotion);
  }
}
