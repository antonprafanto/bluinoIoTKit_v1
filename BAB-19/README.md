# BAB 19: Kode Terstruktur — Multi-File

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami mengapa kode satu file tidak scalable
- Memecah kode menjadi beberapa file (.h dan .cpp)
- Menggunakan multi-tab di Arduino IDE
- Membuat modul reusable untuk komponen kit Bluino
- Menghindari masalah umum #include dan linkage

---

## 19.1 Mengapa Perlu Multi-File?

Seiring bertambahnya fitur, kode satu file menjadi **tidak terkelola**:

```
Program kecil (BAB 8):        Program menengah (BAB 17+):
~50 baris                     ~300+ baris
Mudah di-scroll               Sulit dicari fungsi
Satu tanggung jawab            Banyak tanggung jawab
Tidak perlu dipecah            HARUS dipecah
```

### Keuntungan Multi-File

| Aspek | Satu File | Multi-File |
|-------|-----------|-----------|
| Navigasi kode | Scroll panjang | Langsung ke file yang tepat |
| Reusability | Copy-paste | `#include` |
| Kolaborasi | Konflik terus | Kerja di file berbeda |
| Testing | Sulit diisolasi | Bisa test per modul |
| Maintenance | Risiko break | Perubahan terisolasi |

---

## 19.2 Struktur File C/C++

### Header File (.h) — Deklarasi

```cpp
// led_controller.h — DEKLARASI (apa yang tersedia)

#ifndef LED_CONTROLLER_H   // Include guard — mencegah double-include
#define LED_CONTROLLER_H

#include <Arduino.h>

// Deklarasi fungsi
void ledSetup(int pin);
void ledOn();
void ledOff();
void ledToggle();
void ledBlink(unsigned long interval);
bool getLedState();

#endif
```

### Source File (.cpp) — Implementasi

```cpp
// led_controller.cpp — IMPLEMENTASI (bagaimana cara kerjanya)

#include "led_controller.h"

static int _ledPin = -1;
static bool _ledState = false;
static unsigned long _lastBlink = 0;

void ledSetup(int pin) {
  _ledPin = pin;
  pinMode(_ledPin, OUTPUT);
  _ledState = false;
  digitalWrite(_ledPin, LOW);
}

void ledOn() {
  _ledState = true;
  digitalWrite(_ledPin, HIGH);
}

void ledOff() {
  _ledState = false;
  digitalWrite(_ledPin, LOW);
}

void ledToggle() {
  _ledState = !_ledState;
  digitalWrite(_ledPin, _ledState ? HIGH : LOW);
}

void ledBlink(unsigned long interval) {
  if (millis() - _lastBlink >= interval) {
    _lastBlink = millis();
    ledToggle();
  }
}

bool getLedState() {
  return _ledState;
}
```

### Sketch Utama (.ino)

```cpp
// project.ino — PROGRAM UTAMA

#include "led_controller.h"

void setup() {
  Serial.begin(115200);
  ledSetup(4);  // LED di IO4
  Serial.println("LED Controller ready!");
}

void loop() {
  ledBlink(500);  // Berkedip setiap 500ms
}
```

---

## 19.3 Multi-Tab di Arduino IDE

Arduino IDE mendukung **multi-tab** — file .h, .cpp, dan .ino tambahan dalam folder yang sama.

### Cara Membuat

1. Buka Arduino IDE
2. **Sketch** → **New Tab** (atau Ctrl+Shift+N)
3. Beri nama file (contoh: `button_handler.h`)
4. Tulis kode di tab baru

### Struktur Folder

```
project_folder/
├── project_folder.ino     ← Sketch utama (wajib sama nama folder)
├── led_controller.h       ← Header LED
├── led_controller.cpp     ← Implementasi LED
├── button_handler.h       ← Header Button
├── button_handler.cpp     ← Implementasi Button
└── config.h               ← Konfigurasi pin dan konstanta
```

> **Aturan Arduino IDE:** File `.ino` utama **harus bernama sama** dengan folder project.

---

## 19.4 Program: Proyek Terstruktur

Mari buat proyek "Smart Alarm" dengan kode yang terstruktur rapi:

### File 1: `config.h` — Semua Pin dan Konstanta

```cpp
// config.h — Konfigurasi hardware terpusat
// Ubah pin di sini, seluruh proyek ikut berubah

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// PIN ASSIGNMENT (sesuai rekomendasi BAB 6)
// ============================================================
#define PIN_LED_RED     4    // LED Merah → IO4
#define PIN_LED_GREEN   17   // LED Hijau → IO17
#define PIN_BUTTON      15   // Button S1 → IO15
#define PIN_BUZZER      13   // Buzzer → IO13
#define PIN_LDR         35   // LDR → IO35 (ADC1_CH7)

// ============================================================
// KONSTANTA
// ============================================================
#define DEBOUNCE_MS     50
#define BLINK_FAST_MS   100
#define BLINK_SLOW_MS   500
#define SERIAL_BAUD     115200

#endif
```

### File 2: `smart_button.h`

```cpp
// smart_button.h — Modul tombol dengan debounce dan callback

#ifndef SMART_BUTTON_H
#define SMART_BUTTON_H

#include <Arduino.h>

typedef void (*ButtonCallback)();

struct SmartButton {
  int pin;
  unsigned long debounceMs;
  ButtonCallback onPress;
  ButtonCallback onRelease;

  void begin();
  void update();

  // Internal state (prefixed _ by convention, not private)
  int _lastReading;
  int _state;
  unsigned long _lastDebounce;
};

#endif
```

### File 3: `smart_button.cpp`

```cpp
// smart_button.cpp

#include "smart_button.h"

void SmartButton::begin() {
  pinMode(pin, INPUT);
  _lastReading = LOW;
  _state = LOW;
  _lastDebounce = 0;
  onPress = nullptr;
  onRelease = nullptr;
}

void SmartButton::update() {
  int reading = digitalRead(pin);

  if (reading != _lastReading) {
    _lastDebounce = millis();
  }

  if ((millis() - _lastDebounce) > debounceMs) {
    if (reading != _state) {
      _state = reading;

      if (_state == HIGH && onPress != nullptr) {
        onPress();
      }
      if (_state == LOW && onRelease != nullptr) {
        onRelease();
      }
    }
  }

  _lastReading = reading;
}
```

### File 4: `alarm_system.ino` — Sketch Utama

```cpp
/*
 * BAB 19 - Smart Alarm (Multi-File)
 * Demonstrasi kode terstruktur dengan modul terpisah
 *
 * File: config.h, smart_button.h, smart_button.cpp, alarm_system.ino
 */

#include "config.h"
#include "smart_button.h"

// ============================================================
// GLOBAL STATE
// ============================================================
bool alarmArmed = false;
bool buzzerState = false;
unsigned long prevBuzz = 0;

// ============================================================
// BUTTON INSTANCE
// ============================================================
SmartButton btn = {PIN_BUTTON, DEBOUNCE_MS};

// ============================================================
// CALLBACKS
// ============================================================
void onButtonPress() {
  alarmArmed = !alarmArmed;

  if (alarmArmed) {
    digitalWrite(PIN_LED_GREEN, HIGH);
    digitalWrite(PIN_LED_RED, LOW);
    Serial.println("🟢 Alarm ARMED");
  } else {
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    Serial.println("⚪ Alarm DISARMED");
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  analogSetAttenuation(ADC_11db);

  btn.begin();
  btn.onPress = onButtonPress;

  Serial.println("BAB 19: Smart Alarm (Multi-File)");
  Serial.println("Tekan tombol untuk ARM/DISARM");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  btn.update();

  if (alarmArmed) {
    // Baca LDR (ADC rendah = terang, ADC tinggi = gelap)
    int ldrValue = analogRead(PIN_LDR);

    // Jika cahaya berubah drastis (misalnya pintu dibuka)
    if (ldrValue < 500) {  // Cahaya terang terdeteksi
      // Alarm berbunyi (non-blocking)
      if (millis() - prevBuzz >= BLINK_FAST_MS) {
        prevBuzz = millis();
        buzzerState = !buzzerState;
        digitalWrite(PIN_BUZZER, buzzerState ? HIGH : LOW);
        digitalWrite(PIN_LED_RED, buzzerState ? HIGH : LOW);
      }
    } else {
      // Pastikan mati jika cahaya kembali normal
      if (buzzerState) {
        buzzerState = false;
        digitalWrite(PIN_BUZZER, LOW);
        digitalWrite(PIN_LED_RED, LOW);
      }
    }
  }
}
```

---

## 19.5 Include Guard — Mencegah Double Include

Tanpa include guard, file yang di-`#include` dari beberapa tempat akan **didefinisikan ulang** dan menyebabkan error.

```cpp
// ❌ TANPA INCLUDE GUARD — error jika di-include dari 2 file
struct MyStruct { int x; };

// ✅ DENGAN INCLUDE GUARD — aman di-include berkali-kali
#ifndef MY_HEADER_H
#define MY_HEADER_H

struct MyStruct { int x; };

#endif

// ✅ ALTERNATIF (non-standard tapi didukung semua compiler modern)
#pragma once

struct MyStruct { int x; };
```

---

## 19.6 Tips Organisasi Kode

### 1. Satu File = Satu Tanggung Jawab

```
✅ led_controller.h/.cpp  → Hanya urusan LED
✅ button_handler.h/.cpp  → Hanya urusan tombol
✅ sensor_reader.h/.cpp   → Hanya urusan sensor
✅ config.h               → Hanya pin dan konstanta

❌ utils.h/.cpp           → Terlalu umum, jadi "tempat sampah"
```

### 2. `config.h` untuk Semua Pin

Jangan hardcode pin di mana-mana:

```cpp
// ❌ Pin tersebar di banyak file
// file_a.cpp: digitalWrite(4, HIGH);
// file_b.cpp: pinMode(4, OUTPUT);

// ✅ Pin terpusat di config.h
// config.h:  #define PIN_LED 4
// file_a.cpp: digitalWrite(PIN_LED, HIGH);
// file_b.cpp: pinMode(PIN_LED, OUTPUT);
```

### 3. Variabel `static` di .cpp

Gunakan `static` untuk variabel internal yang tidak perlu diakses dari luar:

```cpp
// led_controller.cpp
static int _pin = -1;        // Hanya bisa diakses di file ini
static bool _state = false;  // Tidak "bocor" ke file lain
```

### 4. Forward Declaration

Jika dua header saling membutuhkan, gunakan forward declaration:

```cpp
// Daripada #include "sensor.h" (bisa circular)
struct Sensor;  // Forward declaration — cukup tahu bahwa Sensor ada
```

---

## 📝 Latihan

1. **Refactor BAB 17:** Pecah program Security System (BAB 17, Program 2) menjadi file terpisah:
   - `config.h` — pin dan konstanta
   - `smart_button.h/.cpp` — modul tombol
   - `security_system.ino` — logika state machine

2. **Modul buzzer:** Buat `buzzer.h/.cpp` yang memiliki fungsi: `buzzerSetup(pin)`, `buzzerBeep(duration)`, `buzzerAlarm(interval)` (non-blocking), `buzzerStop()`.

3. **Modul LDR:** Buat `ldr_sensor.h/.cpp` dengan callback `onChange` yang dipanggil saat tingkat cahaya berubah kategori (Terang/Sedang/Gelap).

4. **Library sendiri:** Pindahkan modul `SmartButton` ke folder library Arduino (`Documents/Arduino/libraries/SmartButton/`). Pastikan bisa di-include dengan `#include <SmartButton.h>` dari sketch manapun.

---

## 📚 Referensi

- [Arduino Multi-File Sketches](https://docs.arduino.cc/learn/contributions/arduino-creating-library-guide/)
- [Header Files in C/C++ — GeeksforGeeks](https://www.geeksforgeeks.org/header-files-in-c-cpp-and-its-uses/)
- [Include Guards — CPP Reference](https://en.cppreference.com/w/cpp/preprocessor/include)

---

[⬅️ BAB 18: Callback & Event-Driven](../BAB-18/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 20 — Memory Management ➡️](../BAB-20/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

