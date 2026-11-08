// ШАГ 1: датчик касания (TTP223)
// Цель: проверить, что датчик работает. Результат смотрим в Serial Monitor.
//
// Подключение (ESP32-C3 Super Mini):
//   Датчик VCC -> 3V3
//   Датчик GND -> GND
//   Датчик I/O -> GPIO 1
//
// Настройки Arduino IDE: Tools -> USB CDC On Boot -> Enabled
// Serial Monitor: скорость 115200

#define TOUCH_PIN 1           // пин, к которому подключен датчик

bool wasTouched = false;      // помним прошлое состояние, чтобы писать только при изменении

void setup() {
  Serial.begin(115200);       // включаем связь с компьютером (скорость 115200)
  pinMode(TOUCH_PIN, INPUT);  // пин будет "слушать" датчик
}

void loop() {
  bool touched = digitalRead(TOUCH_PIN) == HIGH;   // HIGH = палец на датчике

  if (touched && !wasTouched) {      // палец только что коснулся
    Serial.println("Touched");
  }
  if (!touched && wasTouched) {      // палец только что убрали
    Serial.println("Released");
  }

  wasTouched = touched;       // запоминаем состояние для следующего круга
  delay(20);                  // маленькая пауза
}
