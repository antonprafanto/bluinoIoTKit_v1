# BAB 15: Timer Hardware

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami masalah `delay()` dan mengapa timer dibutuhkan
- Menggunakan `millis()` untuk timing non-blocking
- Menggunakan hardware timer ESP32 dengan interrupt
- Membuat scheduler sederhana untuk multi-task

---

## 15.1 Masalah dengan `delay()`

`delay()` adalah fungsi **blocking** — seluruh program berhenti selama delay berjalan:

```cpp
void loop() {
  baca_sensor();     // Dijalankan
  delay(5000);       // ⛔ Program BERHENTI 5 detik
                     // Tidak bisa baca tombol, update LED, dll
  kirim_data();      // Baru dijalankan setelah 5 detik
}
```

**Masalah:**
- Tombol tidak responsif selama delay
- Tidak bisa menjalankan beberapa tugas secara "bersamaan"
- Timing tidak presisi (delay + waktu eksekusi kode)

---

## 15.2 `millis()` — Timer Software

`millis()` mengembalikan waktu (dalam milidetik) sejak ESP32 dinyalakan. Dengan membandingkan waktu, kita bisa membuat timing **non-blocking**.

### Pattern: millis() Timer

```cpp
unsigned long previousMillis = 0;
const long interval = 1000;  // 1 detik

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    // Kode yang dijalankan setiap 1 detik
  }

  // Kode lain tetap berjalan tanpa terganggu
}
```

### Program: Multi-Task dengan millis()

```cpp
/*
 * BAB 15 - Program 1: Multi-Task dengan millis()
 * LED berkedip setiap 500ms + baca sensor setiap 2 detik
 * TANPA menggunakan delay()
 *
 * Wiring: LED Custom → IO4
 */

#define LED_PIN 4

// Timer untuk LED (500ms)
unsigned long prevLED = 0;
const long intervalLED = 500;
bool ledState = false;

// Timer untuk sensor (2 detik)
unsigned long prevSensor = 0;
const long intervalSensor = 2000;

// Timer untuk serial (1 detik)
unsigned long prevSerial = 0;
const long intervalSerial = 1000;

int counter = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("BAB 15: Multi-Task dengan millis()");
  Serial.println("LED berkedip @ 500ms | Sensor @ 2s | Serial @ 1s");
}

void loop() {
  unsigned long now = millis();

  // Task 1: LED berkedip setiap 500ms
  if (now - prevLED >= intervalLED) {
    prevLED = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }

  // Task 2: Baca sensor setiap 2 detik
  if (now - prevSensor >= intervalSensor) {
    prevSensor = now;
    // Simulasi baca sensor
    int sensorValue = analogRead(34);
    Serial.print("[SENSOR] Nilai: ");
    Serial.println(sensorValue);
  }

  // Task 3: Status setiap 1 detik
  if (now - prevSerial >= intervalSerial) {
    prevSerial = now;
    counter++;
    Serial.print("[STATUS] Uptime: ");
    Serial.print(counter);
    Serial.print("s | LED: ");
    Serial.println(ledState ? "ON" : "OFF");
  }

  // CPU tidak pernah "macet" — semua task berjalan secara bergiliran
}
```

---

## 15.3 Hardware Timer ESP32

ESP32 memiliki **4 hardware timer** (64-bit) yang bisa menghasilkan interrupt presisi:

| Timer | Resolusi | Presisi |
|-------|----------|---------|
| Timer 0 | 64-bit | Sangat tinggi (µs-level) |
| Timer 1 | 64-bit | Sangat tinggi |
| Timer 2 | 64-bit | Sangat tinggi |
| Timer 3 | 64-bit | Sangat tinggi |

### Kapan Pakai Hardware Timer?

| Kebutuhan | Solusi |
|-----------|--------|
| Timing sederhana (>10ms) | `millis()` cukup |
| Timing presisi (<1ms) | Hardware timer |
| Interrupt periodik | Hardware timer |
| Watchdog | Hardware timer |
| Pulse counting | Hardware timer / PCNT |

---

## 15.4 Program: Hardware Timer Interrupt

```cpp
/*
 * BAB 15 - Program 2: Hardware Timer Interrupt
 * LED toggle setiap 1 detik menggunakan hardware timer
 *
 * Wiring: LED Custom → IO4
 */

#define LED_PIN 4

// Handle timer
hw_timer_t *timer = NULL;
volatile bool toggleFlag = false;

// ISR timer — harus singkat dan IRAM_ATTR
void IRAM_ATTR onTimer() {
  toggleFlag = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Inisialisasi hardware timer (API Arduino Core 3.x)
  // Parameter: frekuensi timer dalam Hertz (Hz)
  // Frekuensi 1.000.000 Hz (1 MHz) → 1 tick = 1 µs
  timer = timerBegin(1000000);

  // Attach interrupt
  timerAttachInterrupt(timer, &onTimer);

  // Set alarm: trigger setiap 1.000.000 µs (1 detik), auto-reload
  timerAlarm(timer, 1000000, true, 0);

  Serial.println("BAB 15: Hardware Timer - LED toggle 1Hz");
}

void loop() {
  if (toggleFlag) {
    toggleFlag = false;

    static bool ledState = false;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    Serial.print("Timer! LED: ");
    Serial.println(ledState ? "ON" : "OFF");
  }

  // loop() bebas mengerjakan hal lain
}
```

> **Catatan:** API timer ESP32 berubah antara versi Arduino ESP32 Core. Kode di atas menggunakan API v3.x. Jika menggunakan versi lama, cek dokumentasi versi kamu.

---

## 15.5 Program: Stopwatch Presisi

```cpp
/*
 * BAB 15 - Program 3: Stopwatch dengan micros()
 * Mengukur waktu reaksi menekan tombol
 *
 * Wiring:
 *   Button S1 Custom → IO15
 *   LED Merah Custom  → IO4
 */

#define BUTTON_PIN 15
#define LED_PIN    4

enum State { IDLE, WAITING, REACTING, RESULT };
State currentState = IDLE;

unsigned long startTime = 0;
unsigned long reactionTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("BAB 15: Reaction Time Tester");
  Serial.println("Tekan tombol untuk mulai...");
}

void loop() {
  switch (currentState) {
    case IDLE:
      if (digitalRead(BUTTON_PIN) == HIGH) {
        Serial.println("Bersiap... Tunggu LED menyala, lalu tekan tombol SECEPAT MUNGKIN!");
        currentState = WAITING;
        delay(200);  // Debounce

        // Random delay 2-5 detik
        long randomDelay = random(2000, 5000);
        delay(randomDelay);  // OK pakai delay di sini (sengaja menunggu)

        // LED ON — mulai timer
        digitalWrite(LED_PIN, HIGH);
        startTime = micros();  // micros() untuk presisi µs
        currentState = REACTING;
        Serial.println("SEKARANG! Tekan tombol!");
      }
      break;

    case REACTING:
      if (digitalRead(BUTTON_PIN) == HIGH) {
        reactionTime = micros() - startTime;
        currentState = RESULT;
      }
      break;

    case RESULT:
      digitalWrite(LED_PIN, LOW);

      Serial.print("Waktu reaksi: ");
      Serial.print(reactionTime / 1000.0, 1);
      Serial.println(" ms");

      if (reactionTime < 200000) {
        Serial.println("⚡ LUAR BIASA!");
      } else if (reactionTime < 300000) {
        Serial.println("✅ Bagus!");
      } else if (reactionTime < 500000) {
        Serial.println("👍 Normal");
      } else {
        Serial.println("🐢 Lambat...");
      }

      Serial.println("\nTekan tombol untuk coba lagi...");
      delay(500);  // Debounce
      currentState = IDLE;
      break;
  }
}
```

---

## 15.6 Perbandingan Metode Timing

| Metode | Presisi | Blocking? | Cocok untuk |
|--------|---------|-----------|-------------|
| `delay()` | ~1ms | ✅ Blocking | Program sangat sederhana |
| `millis()` | ~1ms | ❌ Non-blocking | **Multi-task** (paling umum) |
| `micros()` | ~1µs | ❌ Non-blocking | Timing presisi tinggi |
| Hardware Timer | ~1µs | ❌ Interrupt-based | Periodik presisi, watchdog |
| `vTaskDelay()` | ~1ms | ❌ FreeRTOS | Multi-thread (BAB 66) |

> **Rekomendasi untuk pemula:** Gunakan `millis()` untuk hampir semua kasus. Hardware timer hanya dibutuhkan untuk timing presisi tinggi (<1ms).

---

## 📝 Latihan

1. **Traffic light non-blocking:** Buat ulang lampu lalu lintas (BAB 8 latihan) tapi **tanpa `delay()`** — gunakan `millis()`. Tombol harus tetap responsif selama traffic light berjalan.

2. **Multi-blink:** Buat 4 LED berkedip dengan interval BERBEDA tanpa `delay()`:
   - LED 1: 200ms
   - LED 2: 500ms
   - LED 3: 1000ms
   - LED 4: 2000ms

3. **Uptime reporter:** Buat program yang menampilkan uptime dalam format jam:menit:detik di Serial Monitor setiap 1 detik, menggunakan `millis()`.

4. **Timer presisi:** Gunakan hardware timer untuk toggle LED pada frekuensi tepat 10 Hz (100ms ON, 100ms OFF). Ukur dengan oscilloscope atau hitung kedipan per 10 detik — harus tepat 100 kedipan.

---

## 📚 Referensi

- [millis() — Arduino Reference](https://www.arduino.cc/reference/en/language/functions/time/millis/)
- [Blink Without Delay — Arduino](https://docs.arduino.cc/built-in-examples/digital/BlinkWithoutDelay/)
- [ESP32 Timer API — Espressif](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html)

---

[⬅️ BAB 14: Capacitive Touch](../BAB-14/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 16 — Non-Blocking Programming ➡️](../BAB-16/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

