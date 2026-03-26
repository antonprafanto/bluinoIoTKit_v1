# BAB 8: Digital Output — LED & Buzzer

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Mengontrol LED eksternal menggunakan GPIO ESP32
- Memahami konsep active-high dan active-low
- Mengontrol active buzzer
- Membuat pola LED running/chasing
- Memahami koneksi custom pada kit Bluino

---

## 8.1 Konsep Digital Output

Digital output memiliki **2 kondisi**:
- **HIGH** = 3.3V (pada ESP32)
- **LOW** = 0V (GND)

```
         3.3V ──┐
                 │ ← digitalWrite(pin, HIGH)
    GPIO ────────┤
                 │ ← digitalWrite(pin, LOW)
           0V ──┘
```

### Fungsi yang Digunakan

```cpp
pinMode(pin, OUTPUT);          // Set pin sebagai output (di setup)
digitalWrite(pin, HIGH);       // Output = 3.3V
digitalWrite(pin, LOW);        // Output = 0V
```

---

## 8.2 Menghubungkan LED pada Kit Bluino

LED pada kit Bluino adalah komponen **custom** — kamu perlu menghubungkannya sendiri ke GPIO ESP32 menggunakan kabel jumper Dupont.

### Cara Kerja LED pada Kit

```
                            Kit Bluino PCB
    ┌─────────────────────────────────────────────┐
    │                                             │
    │   Pin Custom ── Resistor 330Ω ── LED ── GND │
    │                                             │
    └─────────────────────────────────────────────┘
           ↑
           │ Kabel Jumper Dupont
           │
     GPIO ESP32 (misalnya IO4)
```

- Resistor 330Ω **sudah terpasang** di PCB
- LED menggunakan konfigurasi **active-high**: beri HIGH untuk menyalakan
- Kamu hanya perlu menghubungkan pin "Custom" ke GPIO yang dipilih

### Pin yang Direkomendasikan (dari BAB 6)

| LED | Warna | GPIO Rekomendasi |
|-----|-------|-----------------|
| LED 1 | Merah | **IO4** |
| LED 2 | Kuning | **IO16** |
| LED 3 | Hijau | **IO17** |
| LED 4 | Biru | **IO2** |

> **Catatan:** Kamu bebas menggunakan GPIO lain (lihat BAB 6 untuk pembahasan lengkap GPIO yang tersedia). Pin di atas hanya rekomendasi.

### Wiring

```
Kit Bluino Header          ESP32 DEVKIT V1
─────────────────          ────────────────

LED1 Custom ──────────── IO4
LED2 Custom ──────────── IO16
LED3 Custom ──────────── IO17
LED4 Custom ──────────── IO2
```

---

## 8.3 Program: Menyalakan Satu LED

```cpp
/*
 * BAB 8 - Program 1: LED ON/OFF
 * Menyalakan dan mematikan LED merah
 *
 * Wiring: LED1 Custom → IO4
 */

#define LED_MERAH 4  // GPIO4 → LED Merah (via header Custom)

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);

  Serial.println("BAB 8: Digital Output - LED");
}

void loop() {
  digitalWrite(LED_MERAH, HIGH);  // LED ON (active-high)
  Serial.println("LED Merah: ON");
  delay(1000);

  digitalWrite(LED_MERAH, LOW);   // LED OFF
  Serial.println("LED Merah: OFF");
  delay(1000);
}
```

---

## 8.4 Program: Kontrol 4 LED

```cpp
/*
 * BAB 8 - Program 2: Kontrol 4 LED
 * Menyalakan 4 LED secara bergantian
 *
 * Wiring:
 *   LED1 (Merah)  Custom → IO4
 *   LED2 (Kuning) Custom → IO16
 *   LED3 (Hijau)  Custom → IO17
 *   LED4 (Biru)   Custom → IO2
 */

// Konfigurasi pin
#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2

// Gunakan array untuk akses iteratif
const int LED_PINS[] = {LED_MERAH, LED_KUNING, LED_HIJAU, LED_BIRU};
const int JUMLAH_LED = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

void setup() {
  Serial.begin(115200);

  // Inisialisasi semua pin LED sebagai output
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);  // Pastikan semua OFF saat mulai
  }

  Serial.println("BAB 8: Kontrol 4 LED dimulai");
}

void loop() {
  // Nyalakan semua LED satu per satu
  for (int i = 0; i < JUMLAH_LED; i++) {
    digitalWrite(LED_PINS[i], HIGH);
    Serial.print("LED ");
    Serial.print(i + 1);
    Serial.println(" ON");
    delay(500);
  }

  delay(1000);  // Semua ON selama 1 detik

  // Matikan semua LED satu per satu
  for (int i = 0; i < JUMLAH_LED; i++) {
    digitalWrite(LED_PINS[i], LOW);
    Serial.print("LED ");
    Serial.print(i + 1);
    Serial.println(" OFF");
    delay(500);
  }

  delay(1000);  // Semua OFF selama 1 detik
}
```

---

## 8.5 Program: LED Running (Knight Rider)

```cpp
/*
 * BAB 8 - Program 3: LED Running (Knight Rider)
 * LED menyala bergantian maju-mundur
 *
 * Wiring sama dengan Program 2
 */

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2

const int LED_PINS[] = {LED_MERAH, LED_KUNING, LED_HIJAU, LED_BIRU};
const int JUMLAH_LED = sizeof(LED_PINS) / sizeof(LED_PINS[0]);
const int DELAY_MS = 150;  // Kecepatan running

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  Serial.println("LED Running (Knight Rider) dimulai!");
}

void loop() {
  // Maju: LED 0 → 1 → 2 → 3
  for (int i = 0; i < JUMLAH_LED; i++) {
    matikanSemua();
    digitalWrite(LED_PINS[i], HIGH);
    delay(DELAY_MS);
  }

  // Mundur: LED 3 → 2 → 1 → 0
  for (int i = JUMLAH_LED - 2; i > 0; i--) {
    matikanSemua();
    digitalWrite(LED_PINS[i], HIGH);
    delay(DELAY_MS);
  }
}

// Fungsi helper: matikan semua LED
void matikanSemua() {
  for (int i = 0; i < JUMLAH_LED; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}
```

---

## 8.6 Active Buzzer

Active buzzer menghasilkan bunyi **saat diberi tegangan tinggi** (active-high). Berbeda dengan passive buzzer yang membutuhkan sinyal PWM untuk menghasilkan nada.

### Koneksi pada Kit Bluino

Active buzzer adalah komponen **custom** — hubungkan pin Custom buzzer ke GPIO yang dipilih.

| Komponen | GPIO Rekomendasi |
|----------|-----------------|
| Active Buzzer | **IO13** |

### Program: Buzzer ON/OFF

```cpp
/*
 * BAB 8 - Program 4: Active Buzzer
 * Membunyikan buzzer secara berkala
 *
 * Wiring: Buzzer Custom → IO13
 */

#define BUZZER_PIN 13

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Pastikan OFF saat mulai

  Serial.println("Active Buzzer test dimulai");
}

void loop() {
  // Bunyi pendek 3 kali
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);  // Buzzer ON
    delay(100);                      // Bunyi 100ms
    digitalWrite(BUZZER_PIN, LOW);   // Buzzer OFF
    delay(100);                      // Diam 100ms
  }

  delay(2000);  // Jeda 2 detik sebelum ulang
}
```

---

## 8.7 Program: Alarm Sederhana (LED + Buzzer)

Gabungkan LED dan buzzer untuk membuat alarm visual dan audio:

```cpp
/*
 * BAB 8 - Program 5: Alarm Sederhana
 * LED merah berkedip + buzzer berbunyi bergantian
 *
 * Wiring:
 *   LED Merah Custom → IO4
 *   Buzzer Custom    → IO13
 */

#define LED_MERAH  4
#define BUZZER_PIN 13

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("Sistem Alarm dimulai");
}

void loop() {
  // Pola alarm: flash + beep
  for (int i = 0; i < 5; i++) {
    // ON
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);

    // OFF
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }

  // Jeda antar siklus alarm
  Serial.println("--- Jeda 3 detik ---");
  delay(3000);
}
```

---

## 8.8 Arus dan Keamanan GPIO

### Batas Arus GPIO ESP32

| Parameter | Nilai |
|-----------|-------|
| Arus maks per pin | 40 mA (absolute maximum) |
| Arus rekomendasi | **12 mA** |
| Total arus semua GPIO | 1200 mA (total chip) |

### Perhitungan Arus 4 LED

Dari BAB 3, arus setiap LED dengan resistor 330Ω:

```
I = (3.3V - 2.0V) / 330Ω ≈ 3.9 mA per LED

Total 4 LED = 4 × 3.9 mA = 15.6 mA ✅ (aman)
```

### ⚠️ Peringatan

- **Jangan hubungkan beban besar langsung ke GPIO** (motor, relay coil)
- Untuk beban > 12mA, gunakan transistor driver (seperti relay pada kit ini)
- Kit Bluino sudah dilengkapi transistor BC547 untuk relay — **aman** dipakai

---

## 📝 Latihan

1. **Traffic Light:** Buat simulasi lampu lalu lintas:
   - Hijau ON 5 detik → Kuning ON 2 detik → Merah ON 5 detik → ulangi
   - Tampilkan status di Serial Monitor

2. **Pola biner:** Buat 4 LED menampilkan bilangan biner 0–15 (0000–1111), berubah setiap 1 detik. (Tip: gunakan bitwise operator)

3. **Buzzer Morse:** Buat buzzer membunyikan kode morse "SOS" (sama seperti LED SOS di BAB 7, tapi pakai buzzer)

4. **LED + Buzzer game:** Buat pola acak: salah satu dari 4 LED menyala selama 1 detik, lalu buzzer bunyi pendek. Ulangi dengan LED acak berikutnya. (Tip: gunakan `random()`)

---

## 📚 Referensi

- [digitalWrite() — Arduino Reference](https://www.arduino.cc/reference/en/language/functions/digital-io/digitalwrite/)
- [pinMode() — Arduino Reference](https://www.arduino.cc/reference/en/language/functions/digital-io/pinmode/)
- [ESP32 GPIO Output — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-digital-inputs-outputs-arduino/)

---

[⬅️ BAB 7: Program Pertama](../BAB-07/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 9 — Digital Input ➡️](../BAB-09/README.md)
