# BAB 9: Digital Input — Push Button

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Membaca input digital dari push button
- Memahami pull-up dan pull-down resistor
- Mengatasi masalah bouncing pada tombol
- Membuat toggle switch menggunakan push button
- Menggunakan internal pull-up ESP32

---

## 9.1 Konsep Digital Input

Digital input membaca **kondisi** suatu pin:
- **HIGH** = ada tegangan (≥2.475V pada ESP32)
- **LOW** = tidak ada tegangan (≤0.825V pada ESP32)

```cpp
pinMode(pin, INPUT);            // Set pin sebagai input
int nilai = digitalRead(pin);   // Baca nilai: HIGH atau LOW
```

---

## 9.2 Push Button pada Kit Bluino

### Desain Hardware

Push button pada kit Bluino menggunakan konfigurasi **pull-down 10KΩ** yang sudah terpasang di PCB:

```
    3.3V
     │
     ┤ Push Button (Normally Open)
     │
     ├──── Output → pin Custom → ke GPIO ESP32
     │
    ┌┤ 10KΩ (pull-down, sudah di PCB)
    │┤
     │
    GND
```

**Logika:**
- **Tombol TIDAK ditekan:** Output ditarik ke GND oleh resistor 10KΩ → GPIO membaca **LOW**
- **Tombol DITEKAN:** Output terhubung langsung ke 3.3V → GPIO membaca **HIGH**

### Pin yang Direkomendasikan

| Komponen | GPIO Rekomendasi |
|----------|-----------------|
| Push Button 1 (S1) | **IO15** |
| Push Button 2 (S2) | **IO25** |

### Wiring

```
Kit Bluino Header          ESP32 DEVKIT V1
─────────────────          ────────────────

Button S1 Custom ─────── IO15
Button S2 Custom ─────── IO25
```

---

## 9.3 Program: Baca Tombol Sederhana

```cpp
/*
 * BAB 9 - Program 1: Baca Push Button
 * Membaca status tombol dan tampilkan di Serial Monitor
 *
 * Wiring: Button S1 Custom → IO15
 *         LED Merah Custom  → IO4
 */

#define BUTTON_PIN 15  // GPIO15 → Push Button S1
#define LED_PIN    4   // GPIO4  → LED Merah

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);  // Input tanpa pull-up internal (pull-down sudah di PCB)
  pinMode(LED_PIN, OUTPUT);

  Serial.println("BAB 9: Push Button - LED Control");
  Serial.println("Tekan tombol untuk menyalakan LED...");
}

void loop() {
  int statusTombol = digitalRead(BUTTON_PIN);

  if (statusTombol == HIGH) {
    // Tombol ditekan → LED ON
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Tombol: DITEKAN → LED ON");
  } else {
    // Tombol dilepas → LED OFF
    digitalWrite(LED_PIN, LOW);
  }

  delay(50);  // Delay kecil untuk stabilitas
}
```

---

## 9.4 Masalah Bouncing

### Apa itu Bouncing?

Saat tombol mekanis ditekan atau dilepas, kontak logam **memantul** beberapa kali sebelum stabil. Ini disebut **bouncing** dan bisa membuat ESP32 menganggap tombol ditekan berkali-kali.

```
Sinyal yang diharapkan:        Sinyal sebenarnya (bouncing):

HIGH ────┐                     HIGH ─┐ ┌┐ ┌─────────
         │                           │ ││ │
LOW  ────┘                     LOW  ─┘ └┘ └──
     (bersih)                        (bouncing ~5-20ms)
```

### Solusi: Software Debouncing

```cpp
/*
 * BAB 9 - Program 2: Debounce Push Button
 * Membaca tombol dengan debounce untuk menghindari pembacaan ganda
 *
 * Wiring: Button S1 Custom → IO15
 *         LED Merah Custom  → IO4
 */

#define BUTTON_PIN 15
#define LED_PIN    4

// Variabel debounce
int lastButtonState = LOW;       // Status tombol sebelumnya
int buttonState = LOW;           // Status tombol saat ini
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;  // 50ms debounce

bool ledState = false;  // Status LED (toggle)

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("BAB 9: Push Button dengan Debounce");
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);

  // Jika ada perubahan, reset timer debounce
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Jika sudah stabil selama DEBOUNCE_DELAY
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Jika status benar-benar berubah
    if (reading != buttonState) {
      buttonState = reading;

      // Hanya toggle LED saat tombol DITEKAN (rising edge)
      if (buttonState == HIGH) {
        ledState = !ledState;  // Toggle
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        Serial.print("Toggle! LED sekarang: ");
        Serial.println(ledState ? "ON" : "OFF");
      }
    }
  }

  lastButtonState = reading;
}
```

### Penjelasan Alur Debounce

```
Tombol ditekan → bouncing terjadi (~5ms)
                    │
                    ▼
            Setiap perubahan → reset timer
                    │
                    ▼
            Stabil selama 50ms?
              │           │
             Tidak        Ya
              │           │
           Abaikan    Baca sebagai perubahan yang valid
                          │
                          ▼
                    Toggle LED
```

---

## 9.5 Program: Toggle dengan 2 Tombol

```cpp
/*
 * BAB 9 - Program 3: Dua Tombol — Kontrol 4 LED
 * Tombol 1: Nyalakan LED berikutnya
 * Tombol 2: Matikan LED berikutnya
 *
 * Wiring:
 *   Button S1 Custom → IO15
 *   Button S2 Custom → IO25
 *   LED 1 Custom → IO4
 *   LED 2 Custom → IO16
 *   LED 3 Custom → IO17
 *   LED 4 Custom → IO2
 */

#define BTN_1 15
#define BTN_2 25

const int LED_PINS[] = {4, 16, 17, 2};
const int JUMLAH_LED = 4;

// Debounce untuk 2 tombol
unsigned long lastDebounce1 = 0;
unsigned long lastDebounce2 = 0;
int lastState1 = LOW;
int lastState2 = LOW;
int btnState1 = LOW;
int btnState2 = LOW;
const unsigned long DEBOUNCE = 50;

int ledAktif = 0;  // Jumlah LED yang menyala (0-4)

void setup() {
  Serial.begin(115200);
  pinMode(BTN_1, INPUT);
  pinMode(BTN_2, INPUT);

  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  Serial.println("BAB 9: 2 Tombol Kontrol 4 LED");
  Serial.println("Tombol 1 = Tambah LED | Tombol 2 = Kurangi LED");
}

void loop() {
  // Debounce tombol 1
  int read1 = digitalRead(BTN_1);
  if (read1 != lastState1) lastDebounce1 = millis();
  if ((millis() - lastDebounce1) > DEBOUNCE) {
    if (read1 != btnState1) {
      btnState1 = read1;
      if (btnState1 == HIGH && ledAktif < JUMLAH_LED) {
        digitalWrite(LED_PINS[ledAktif], HIGH);
        ledAktif++;
        Serial.print("LED aktif: ");
        Serial.println(ledAktif);
      }
    }
  }
  lastState1 = read1;

  // Debounce tombol 2
  int read2 = digitalRead(BTN_2);
  if (read2 != lastState2) lastDebounce2 = millis();
  if ((millis() - lastDebounce2) > DEBOUNCE) {
    if (read2 != btnState2) {
      btnState2 = read2;
      if (btnState2 == HIGH && ledAktif > 0) {
        ledAktif--;
        digitalWrite(LED_PINS[ledAktif], LOW);
        Serial.print("LED aktif: ");
        Serial.println(ledAktif);
      }
    }
  }
  lastState2 = read2;
}
```

---

## 9.6 Internal Pull-up ESP32

ESP32 memiliki **internal pull-up resistor** (~45KΩ) yang bisa diaktifkan tanpa resistor eksternal. Berguna jika kamu merangkai tombol sendiri tanpa PCB kit.

```cpp
// Tombol terhubung antara GPIO dan GND (tanpa resistor eksternal)
pinMode(pin, INPUT_PULLUP);

// Logika TERBALIK:
// Tombol ditekan  → pin terhubung ke GND → digitalRead = LOW
// Tombol dilepas  → pull-up menarik ke 3.3V → digitalRead = HIGH
```

> **Pada kit Bluino:** Push button sudah menggunakan pull-down 10KΩ di PCB, jadi kita pakai `INPUT` biasa (bukan `INPUT_PULLUP`). Logikanya: ditekan = HIGH, dilepas = LOW.

### Perbandingan

| Konfigurasi | Resistor | Tidak Ditekan | Ditekan |
|-------------|----------|---------------|---------|
| **Pull-down (kit Bluino)** | 10KΩ ke GND (di PCB) | LOW | HIGH |
| **Pull-up internal** | ~45KΩ ke 3.3V (internal) | HIGH | LOW |
| **Pull-up eksternal** | 10KΩ ke 3.3V (tambahan) | HIGH | LOW |

---

## 📝 Latihan

1. **LED kontrol:** Buat program dimana tombol 1 menyalakan LED hijau dan tombol 2 menyalakan LED merah. Kedua LED mati saat tidak ada tombol yang ditekan.

2. **Counter:** Setiap kali tombol ditekan (dengan debounce), angka counter bertambah 1 dan ditampilkan di Serial Monitor. Setelah mencapai 10, reset ke 0.

3. **Speed control:** Buat LED running (seperti BAB 8 Program 3). Tombol 1 mempercepat running, Tombol 2 memperlambat. Kecepatan berubah antara 50ms–500ms.

4. **Kombinasi tombol:** Buat program yang mendeteksi:
   - Tombol 1 saja → LED Merah
   - Tombol 2 saja → LED Hijau
   - Kedua tombol bersamaan → Buzzer ON
   - Tidak ada tombol → semua OFF

---

## 📚 Referensi

- [digitalRead() — Arduino Reference](https://www.arduino.cc/reference/en/language/functions/digital-io/digitalread/)
- [Debouncing — Arduino](https://docs.arduino.cc/built-in-examples/digital/Debounce/)
- [ESP32 Digital Inputs — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-digital-inputs-outputs-arduino/)

---

[⬅️ BAB 8: Digital Output](../BAB-08/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 10 — Interrupt & ISR ➡️](../BAB-10/README.md)
