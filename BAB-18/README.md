# BAB 18: Callback & Event-Driven Programming

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami konsep callback (fungsi sebagai parameter)
- Menggunakan function pointer di C/C++
- Membuat event system sederhana
- Membedakan polling, interrupt, dan event-driven
- Mendesain kode yang modular dan loosely-coupled

---

## 18.1 Apa itu Callback?

**Callback** adalah fungsi yang "disisipkan" ke fungsi lain untuk dipanggil nanti saat suatu event terjadi.

### Analogi

Bayangkan kamu memesan makanan di restoran:
1. Kamu memberikan **nomor meja** (callback) ke kasir
2. Kasir **tidak langsung membawakan** makanannya
3. Saat pesanan siap, kasir **memanggil** nomor meja kamu

Kamu tidak perlu berdiri di depan kasir menunggu (polling). Kamu juga tidak harus setengah terganggu (interrupt). Kamu **mendaftarkan callback**, lalu mengerjakan hal lain.

### Callback yang Sudah Kamu Kenal

Kamu sebenarnya sudah menggunakan callback tanpa sadar:

```cpp
// BAB 10: attachInterrupt() menerima callback
attachInterrupt(pin, fungsiISR, RISING);  // fungsiISR = callback
                      ↑
                      fungsi yang dipanggil saat event terjadi

// BAB 14: touchAttachInterrupt() menerima callback
touchAttachInterrupt(pin, touchCallback, threshold);
                           ↑
                           fungsi yang dipanggil saat disentuh
```

---

## 18.2 Function Pointer di C/C++

Callback dimungkinkan oleh **function pointer** — variabel yang menyimpan alamat fungsi.

### Sintaks Dasar

```cpp
// Deklarasi function pointer
// tipeReturn (*namaPointer)(tipeParameter1, tipeParameter2, ...)

void (*myCallback)(int);  // Pointer ke fungsi void yang menerima int

// Contoh penggunaan
void cetakAngka(int x) {
  Serial.println(x);
}

void gandakan(int x) {
  Serial.println(x * 2);
}

void setup() {
  Serial.begin(115200);

  myCallback = cetakAngka;  // Arahkan ke cetakAngka
  myCallback(5);            // Output: 5

  myCallback = gandakan;    // Arahkan ke gandakan
  myCallback(5);            // Output: 10
}
```

### Typedef untuk Keterbacaan

```cpp
// Tanpa typedef (sulit dibaca)
void (*onPressCallback)(int pin, bool state);

// Dengan typedef (lebih bersih)
typedef void (*ButtonCallback)(int pin, bool state);

ButtonCallback onPress;    // Jauh lebih mudah dibaca
ButtonCallback onRelease;
```

---

## 18.3 Program: Button Event System

Membuat tombol yang bisa memanggil fungsi berbeda tanpa mengubah kode tombolnya:

```cpp
/*
 * BAB 18 - Program 1: Button dengan Callback
 * Tombol memanggil fungsi yang didaftarkan oleh pengguna
 *
 * Wiring:
 *   Button S1 Custom → IO15
 *   LED Merah Custom → IO4
 *   Buzzer Custom    → IO13
 */

#define BTN_PIN    15
#define LED_PIN    4
#define BUZZER_PIN 13

// ============================================================
// TIPE CALLBACK
// ============================================================
typedef void (*EventCallback)();

// ============================================================
// SMART BUTTON — class dengan callback
// ============================================================
struct SmartButton {
  int pin;
  EventCallback onPress;     // Callback saat tombol ditekan
  EventCallback onRelease;   // Callback saat tombol dilepas

  // Internal state
  int _lastState;
  int _state;
  unsigned long _lastDebounce;

  void begin() {
    pinMode(pin, INPUT);
    _lastState = LOW;
    _state = LOW;
    _lastDebounce = 0;
    onPress = nullptr;
    onRelease = nullptr;
  }

  void update() {
    int reading = digitalRead(pin);
    if (reading != _lastState) _lastDebounce = millis();

    if ((millis() - _lastDebounce) > 50) {
      if (reading != _state) {
        _state = reading;

        if (_state == HIGH && onPress != nullptr) {
          onPress();      // Panggil callback saat DITEKAN
        }
        if (_state == LOW && onRelease != nullptr) {
          onRelease();    // Panggil callback saat DILEPAS
        }
      }
    }
    _lastState = reading;
  }
};

// ============================================================
// CALLBACK FUNCTIONS
// ============================================================
bool ledState = false;

void toggleLED() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  Serial.print("LED Toggled: ");
  Serial.println(ledState ? "ON" : "OFF");
}

void beepBuzzer() {
  // Beep singkat non-blocking (boleh blocking singkat di callback)
  digitalWrite(BUZZER_PIN, HIGH);
  delay(50);  // 50ms beep — cukup singkat
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Beep!");
}

// ============================================================
// MAIN
// ============================================================
SmartButton tombol = {BTN_PIN};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  tombol.begin();
  tombol.onPress = toggleLED;    // Daftarkan callback
  tombol.onRelease = beepBuzzer; // Daftarkan callback

  Serial.println("BAB 18: Button Event System");
  Serial.println("Tekan: toggle LED | Lepas: beep");
}

void loop() {
  tombol.update();  // SmartButton mengurus debounce + callback
}
```

**Keunggulan:** Kode `SmartButton` bisa di-reuse tanpa modifikasi. Perilaku tombol ditentukan oleh callback yang didaftarkan, bukan hardcoded di dalam struct.

---

## 18.4 Program: Sensor Monitor dengan Callback

```cpp
/*
 * BAB 18 - Program 2: Sensor Monitor Event-Driven
 * Callback dipanggil hanya saat nilai sensor BERUBAH signifikan
 *
 * Wiring: Potensiometer Custom → IO34
 */

#define POT_PIN 34

// Tipe callback untuk event sensor
typedef void (*SensorCallback)(int oldValue, int newValue);

// ============================================================
// SENSOR MONITOR
// ============================================================
struct SensorMonitor {
  int pin;
  int threshold;            // Perubahan minimum untuk trigger
  SensorCallback onChange;  // Callback saat nilai berubah

  // Internal
  int _lastValue;
  unsigned long _lastRead;
  unsigned long _interval;

  void begin(unsigned long readInterval) {
    analogSetAttenuation(ADC_11db);
    _lastValue = analogRead(pin);
    _lastRead = 0;
    _interval = readInterval;
    onChange = nullptr;
  }

  void update() {
    if (millis() - _lastRead < _interval) return;
    _lastRead = millis();

    int current = analogRead(pin);

    // Hanya panggil callback jika perubahan melebihi threshold
    if (abs(current - _lastValue) >= threshold && onChange != nullptr) {
      onChange(_lastValue, current);
      _lastValue = current;
    }
  }
};

// ============================================================
// CALLBACK
// ============================================================
void onPotChange(int oldVal, int newVal) {
  int persen = map(newVal, 0, 4095, 0, 100);
  Serial.print("Pot berubah: ");
  Serial.print(oldVal);
  Serial.print(" → ");
  Serial.print(newVal);
  Serial.print(" (");
  Serial.print(persen);
  Serial.println("%)");
}

// ============================================================
// MAIN
// ============================================================
SensorMonitor potMonitor = {POT_PIN, 50};  // Threshold = 50 digit

void setup() {
  Serial.begin(115200);
  potMonitor.begin(100);             // Baca setiap 100ms
  potMonitor.onChange = onPotChange;  // Daftarkan callback

  Serial.println("BAB 18: Sensor Event Monitor");
  Serial.println("Putar potensiometer — callback dipanggil saat berubah");
}

void loop() {
  potMonitor.update();
}
```

---

## 18.5 Event-Driven vs Polling vs Interrupt

| Aspek | Polling | Interrupt | Event-Driven |
|-------|---------|-----------|-------------|
| **Cara kerja** | Cek terus-menerus | Hardware menyela CPU | Callback dipanggil saat event |
| **Efisiensi CPU** | ❌ Boros | ✅ Efisien | ✅ Efisien |
| **Kompleksitas ISR** | — | Harus sangat singkat | Bebas — berjalan di loop() |
| **Fleksibilitas** | Tinggi | Terbatas | Sangat tinggi |
| **Modularitas** | Rendah | Sedang | ✅ Tinggi |
| **Contoh** | `if (digitalRead())` | `attachInterrupt()` | `button.onPress = fn` |

### Kapan Pakai Apa?

```
Event sangat cepat (<1ms)?  → Interrupt (BAB 10)
Butuh respon real-time?     → Interrupt
Butuh kode modular/reusable → Event-Driven (BAB ini)
Program sederhana?          → Polling (BAB 9)
```

---

## 18.6 Program: Multi-Event System

```cpp
/*
 * BAB 18 - Program 3: Multi-Event Dashboard
 * Beberapa event source, masing-masing dengan callback sendiri
 *
 * Wiring:
 *   Button S1 Custom → IO15
 *   Pot Custom       → IO34
 *   LDR Custom       → IO35
 *   LED Custom       → IO4
 */

#define BTN_PIN 15
#define POT_PIN 34
#define LDR_PIN 35
#define LED_PIN 4

typedef void (*EventCallback)();
typedef void (*SensorCallback)(int oldVal, int newVal);

// ── SmartButton (simplified) ──
struct SmartButton {
  int pin;
  EventCallback onPress;
  int _lastState, _state;
  unsigned long _lastDebounce;

  void begin() { pinMode(pin, INPUT); _lastState = _state = LOW; onPress = nullptr; }
  void update() {
    int r = digitalRead(pin);
    if (r != _lastState) _lastDebounce = millis();
    if ((millis() - _lastDebounce) > 50 && r != _state) {
      _state = r;
      if (_state == HIGH && onPress) onPress();
    }
    _lastState = r;
  }
};

// ── SensorMonitor (simplified) ──
struct SensorMonitor {
  int pin, threshold, _lastVal;
  unsigned long _lastRead, _interval;
  SensorCallback onChange;

  void begin(unsigned long interval) {
    analogSetAttenuation(ADC_11db);
    _lastVal = analogRead(pin); _lastRead = 0; _interval = interval; onChange = nullptr;
  }
  void update() {
    if (millis() - _lastRead < _interval) return;
    _lastRead = millis();
    int v = analogRead(pin);
    if (abs(v - _lastVal) >= threshold && onChange) { onChange(_lastVal, v); _lastVal = v; }
  }
};

// ── Instances ──
SmartButton btn = {BTN_PIN};
SensorMonitor pot = {POT_PIN, 100};
SensorMonitor ldr = {LDR_PIN, 200};

// ── Callbacks ──
bool ledState = false;

void onBtnPress() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  Serial.println("[BTN] Toggle LED");
}

void onPotChange(int oldV, int newV) {
  Serial.print("[POT] ");
  Serial.print(map(newV, 0, 4095, 0, 100));
  Serial.println("%");
}

void onLdrChange(int oldV, int newV) {
  Serial.print("[LDR] Cahaya: ");
  if (newV < 1000) Serial.println("Terang");
  else if (newV < 3000) Serial.println("Sedang");
  else Serial.println("Gelap");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  btn.begin();       btn.onPress = onBtnPress;
  pot.begin(200);    pot.onChange = onPotChange;
  ldr.begin(500);    ldr.onChange = onLdrChange;

  Serial.println("BAB 18: Multi-Event Dashboard");
}

void loop() {
  btn.update();
  pot.update();
  ldr.update();
}
```

---

## 📝 Latihan

1. **Long press:** Modifikasi `SmartButton` agar mendukung callback `onLongPress` (ditekan > 2 detik) selain `onPress` biasa.

2. **Threshold callback:** Buat `SensorMonitor` yang memanggil callback `onHigh` saat nilai melebihi batas atas, dan `onLow` saat di bawah batas bawah (hysteresis).

3. **Serial event:** Buat event system untuk Serial command — `onCommand("led", toggleLED)`, `onCommand("status", showStatus)`. Parsing non-blocking.

4. **Timer event:** Buat struct `TimerEvent` yang memanggil callback setiap N milidetik (mirip `setInterval()` di JavaScript).

---

## 📚 Referensi

- [Function Pointers in C — GeeksforGeeks](https://www.geeksforgeeks.org/function-pointer-in-c/)
- [Callback Functions — Programiz](https://www.programiz.com/cpp-programming/function-pointer)
- [Event-Driven Programming for Embedded — Barr Group](https://barrgroup.com/embedded-systems/how-to/state-machines-event-driven-systems)

---

[⬅️ BAB 17: State Machine](../BAB-17/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 19 — Kode Terstruktur ➡️](../BAB-19/README.md)
