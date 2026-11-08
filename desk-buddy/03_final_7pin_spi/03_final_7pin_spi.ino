// ШАГ 3 (ФИНАЛ): датчик касания + эмоции + часы. OLED 7 pin (SPI).
//
// Что делает робот:
//   - Нет касаний 30 секунд  -> засыпает (глаза закрыты)
//   - Касание                -> просыпается и смеётся
//   - Держим палец ~1 секунду -> показывает время на 5 секунд
//
// Подключение (ESP32-C3 Super Mini):
//   OLED:   GND -> GND, VCC -> 3V3, D0 -> GPIO 4, D1 -> GPIO 6,
//           RES -> GPIO 10, DC -> GPIO 3, CS -> GPIO 7
//   Датчик: GND -> GND, VCC -> 3V3, I/O -> GPIO 1
//
// ВАЖНО: не публикуй на GitHub свой настоящий пароль от Wi-Fi!

#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

// ===== НАСТРОЙКИ (меняй здесь) =====
const char* WIFI_NAME = "YOUR_WIFI";      // имя Wi-Fi (только 2.4 GHz)
const char* WIFI_PASS = "YOUR_PASSWORD";  // пароль Wi-Fi

#define TOUCH_PIN   1       // пин датчика касания
#define OLED_SCK    4       // пин D0 экрана (такт)
#define OLED_MOSI   6       // пин D1 экрана (данные)
#define OLED_CS     7       // пин CS экрана (выбор)
#define OLED_DC     3       // пин DC экрана (команда/данные)
#define OLED_RST    10      // пин RES экрана (сброс)
#define SLEEP_AFTER 30000   // через сколько мс без касаний засыпает
#define HOLD_TIME   1000    // сколько мс держать палец, чтобы показать время
#define CLOCK_TIME  5000    // сколько мс показывать время

// ===== ЭКРАН И ГЛАЗА =====
Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RST, OLED_CS);  // экран 128x64 по SPI
RoboEyes<Adafruit_SSD1306> roboEyes(display);   // глаза рисуются на этом экране

// ===== СОСТОЯНИЯ РОБОТА =====
enum State { ST_AWAKE, ST_SLEEP, ST_CLOCK };    // бодрствует, спит, показывает время
State state = ST_AWAKE;

bool wasTouched = false;        // было ли касание в прошлом цикле
bool holdDone = false;          // часы уже показали за это касание
unsigned long lastTouch = 0;    // когда последний раз трогали
unsigned long touchStart = 0;   // когда началось текущее касание
unsigned long clockStart = 0;   // когда начали показывать время
unsigned long happyEnd = 0;     // когда закончить смеяться

// ===== ПОЛУЧАЕМ ВРЕМЯ ИЗ ИНТЕРНЕТА (один раз при включении) =====
void getTime() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");        // сообщение на экране
  display.display();

  WiFi.mode(WIFI_STA);
  // WiFi.setTxPower(WIFI_POWER_8_5dBm);        // раскомментируй, если Wi-Fi не подключается
  WiFi.begin(WIFI_NAME, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(250);  // ждём до 15 с

  if (WiFi.status() == WL_CONNECTED) {
    configTime(5 * 3600, 0, "pool.ntp.org");    // 5 * 3600 = часовой пояс UTC+5 (Ташкент)
    struct tm t;
    getLocalTime(&t, 10000);                    // ждём до 10 с, пока придёт время
  }
  WiFi.disconnect(true);                        // Wi-Fi больше не нужен,
  WiFi.mode(WIFI_OFF);                          // время дальше считается само
}

// ===== РИСУЕМ ВРЕМЯ НА ЭКРАНЕ =====
void drawClock() {
  struct tm t;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(4);                       // большие цифры
  display.setCursor(4, 16);
  if (getLocalTime(&t, 0)) {                    // время получено
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
    display.print(buf);
  } else {
    display.print("--:--");                     // времени нет (не было Wi-Fi)
  }
  display.display();
}

void setup() {
  pinMode(TOUCH_PIN, INPUT);                    // датчик касания только "слушаем"
  SPI.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);  // говорим ESP32, какие пины для SPI
  display.begin(SSD1306_SWITCHCAPVCC);          // запускаем экран

  getTime();                                    // берём время из интернета

  roboEyes.begin(128, 64, 60);                  // запускаем глаза
  roboEyes.setAutoblinker(ON, 3, 2);            // моргают сами
  roboEyes.setIdleMode(ON, 2, 2);               // глаза смотрят по сторонам
  lastTouch = millis();
}

void loop() {
  unsigned long now = millis();
  bool touched = digitalRead(TOUCH_PIN) == HIGH;   // HIGH = палец на датчике

  // --- РИСУЕМ: или часы, или глаза (это должно быть в каждом цикле) ---
  if (state == ST_CLOCK) drawClock();
  else roboEyes.update();

  // --- ПАЛЕЦ ТОЛЬКО ЧТО КОСНУЛСЯ ---
  if (touched && !wasTouched) {
    lastTouch = now;
    touchStart = now;
    holdDone = false;

    if (state == ST_SLEEP) {                    // если спал, просыпаемся
      roboEyes.open();
      roboEyes.setAutoblinker(ON, 3, 2);
      roboEyes.setIdleMode(ON, 2, 2);
      state = ST_AWAKE;
    }
    if (state == ST_AWAKE) {                    // смеёмся
      roboEyes.setMood(HAPPY);
      roboEyes.anim_laugh();
      happyEnd = now + 2500;                    // смеёмся 2,5 секунды
    }
  }

  // --- ПАЛЕЦ ДЕРЖИМ ---
  if (touched) {
    lastTouch = now;                            // не даём ему заснуть
    if (state == ST_AWAKE && !holdDone && now - touchStart > HOLD_TIME) {
      holdDone = true;                          // показываем время один раз
      state = ST_CLOCK;
      clockStart = now;
      happyEnd = 0;
    }
  }

  // --- ВРЕМЯ ПОКАЗАНО, ВОЗВРАЩАЕМ ГЛАЗА ---
  if (state == ST_CLOCK && now - clockStart > CLOCK_TIME) {
    state = ST_AWAKE;
    roboEyes.setMood(DEFAULT);
  }

  // --- ПЕРЕСТАЁМ СМЕЯТЬСЯ ---
  if (state == ST_AWAKE && happyEnd && now > happyEnd) {
    roboEyes.setMood(DEFAULT);
    happyEnd = 0;
  }

  // --- ДОЛГО НЕ ТРОГАЮТ: ЗАСЫПАЕМ ---
  if (state == ST_AWAKE && now - lastTouch > SLEEP_AFTER) {
    state = ST_SLEEP;
    roboEyes.setAutoblinker(OFF, 3, 2);
    roboEyes.setIdleMode(OFF, 2, 2);
    roboEyes.close();                           // закрываем глаза
  }

  wasTouched = touched;                         // запоминаем для следующего цикла
}
