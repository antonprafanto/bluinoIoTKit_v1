# BAB 12: PWM dengan LEDC

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami konsep PWM (Pulse Width Modulation)
- Menggunakan peripheral LEDC pada ESP32
- Mengontrol kecerahan LED secara halus
- Membuat efek fade, breathing, dan rainbow
- Mengontrol servo dengan PWM (preview BAB 38)

---

## 12.1 Apa itu PWM?

**PWM (Pulse Width Modulation)** adalah teknik menghasilkan sinyal "analog" menggunakan sinyal digital. Konsepnya: menyalakan/mematikan pin dengan sangat cepat sehingga rata-rata tegangannya berubah.

```
100% Duty Cycle (selalu ON = 3.3V):
HIGH ─────────────────────────
LOW

50% Duty Cycle (rata-rata = 1.65V):
HIGH ─┐   ┌─┐   ┌─┐   ┌─┐
LOW   └───┘ └───┘ └───┘ └───

25% Duty Cycle (rata-rata = 0.825V):
HIGH ─┐ ┌─┐ ┌─┐ ┌─┐
LOW   └──┘└──┘└──┘└──┘
```

### Parameter PWM

| Parameter | Keterangan | Contoh |
|-----------|------------|--------|
| **Frekuensi** | Berapa kali ON/OFF per detik | 5000 Hz |
| **Duty Cycle** | Persentase waktu ON dalam satu periode | 50% |
| **Resolusi** | Tingkat kehalusan duty cycle | 8-bit (256 level) |

---

## 12.2 LEDC — LED Control Peripheral

ESP32 **tidak mendukung** `analogWrite()` seperti Arduino Uno. Sebagai gantinya, ESP32 memiliki peripheral khusus bernama **LEDC** (LED Control) yang jauh lebih powerful.

### Spesifikasi LEDC

| Parameter | Nilai |
|-----------|-------|
| Jumlah channel | 16 (0–15) |
| Frekuensi | 1 Hz – 40 MHz |
| Resolusi duty | 1-bit – 20-bit |
| Mode | High-speed (hardware) atau Low-speed (software) |

### Arduino ESP32 API (versi baru)

```cpp
// Setup: attach pin ke LEDC
ledcAttach(pin, frekuensi, resolusi);

// Tulis duty cycle
ledcWrite(pin, dutyCycle);

// Detach
ledcDetach(pin);
```

> **Catatan:** API versi lama menggunakan `ledcSetup()` + `ledcAttachPin()`. Versi baru (v3.x) menyederhanakan menjadi `ledcAttach()`.

---

## 12.3 Program: LED Fade (Breathing)

```cpp
/*
 * BAB 12 - Program 1: LED Breathing
 * LED menyala dan meredup secara bertahap (efek bernapas)
 *
 * Wiring: LED Merah Custom → IO4
 */

#define LED_PIN 4

const int FREQ = 5000;       // 5 KHz
const int RESOLUTION = 8;    // 8-bit (0-255)

void setup() {
  Serial.begin(115200);
  ledcAttach(LED_PIN, FREQ, RESOLUTION);

  Serial.println("BAB 12: LED Breathing");
}

void loop() {
  // Fade In (0 → 255)
  for (int duty = 0; duty <= 255; duty++) {
    ledcWrite(LED_PIN, duty);
    delay(8);
  }

  // Fade Out (255 → 0)
  for (int duty = 255; duty >= 0; duty--) {
    ledcWrite(LED_PIN, duty);
    delay(8);
  }

  delay(200);  // Jeda di titik redup
}
```

---

## 12.4 Resolusi dan Frekuensi

### Trade-off: Resolusi vs Frekuensi

Semakin tinggi resolusi, semakin rendah frekuensi maksimum (dan sebaliknya):

```
Frekuensi max = 80 MHz / (2^resolusi)

Resolusi 8-bit  → max = 80M / 256   = 312.500 Hz
Resolusi 10-bit → max = 80M / 1024  =  78.125 Hz
Resolusi 12-bit → max = 80M / 4096  =  19.531 Hz
Resolusi 16-bit → max = 80M / 65536 =   1.220 Hz
```

### Rekomendasi

| Penggunaan | Frekuensi | Resolusi | Keterangan |
|------------|-----------|----------|------------|
| LED dimming | 5 KHz | 8-bit (256 level) | Standar, cukup halus |
| LED presisi | 5 KHz | 12-bit (4096 level) | Sangat halus |
| Servo motor | 50 Hz | 16-bit (65536 level) | Standar servo |
| Audio/buzzer | 1–10 KHz | 8-bit | Tone generation |

---

## 12.5 Program: Kontrol 4 LED dengan PWM

```cpp
/*
 * BAB 12 - Program 2: 4 LED Fade Berurutan
 * Setiap LED fade in/out secara bergantian
 *
 * Wiring:
 *   LED 1 Custom → IO4
 *   LED 2 Custom → IO16
 *   LED 3 Custom → IO17
 *   LED 4 Custom → IO2
 */

const int LED_PINS[] = {4, 16, 17, 2};
const int JUMLAH_LED = 4;

const int FREQ = 5000;
const int RESOLUTION = 8;

void setup() {
  Serial.begin(115200);

  // Attach semua LED ke LEDC
  for (int i = 0; i < JUMLAH_LED; i++) {
    ledcAttach(LED_PINS[i], FREQ, RESOLUTION);
  }

  Serial.println("BAB 12: 4 LED PWM Fade");
}

void loop() {
  for (int led = 0; led < JUMLAH_LED; led++) {
    Serial.print("Fade LED ");
    Serial.println(led + 1);

    // Fade In
    for (int duty = 0; duty <= 255; duty += 5) {
      ledcWrite(LED_PINS[led], duty);
      delay(5);
    }

    // Fade Out
    for (int duty = 255; duty >= 0; duty -= 5) {
      ledcWrite(LED_PINS[led], duty);
      delay(5);
    }
  }
}
```

---

## 12.6 Program: Pot Kontrol Brightness + Frekuensi

```cpp
/*
 * BAB 12 - Program 3: Pot → LED PWM
 * Potensiometer mengontrol brightness LED secara real-time
 *
 * Wiring:
 *   Pot Custom → IO34 (ADC1)
 *   LED Custom → IO4  (PWM)
 */

#define POT_PIN 34
#define LED_PIN 4

const int FREQ = 5000;
const int RESOLUTION = 8;

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  ledcAttach(LED_PIN, FREQ, RESOLUTION);

  Serial.println("BAB 12: Pot → LED PWM");
}

void loop() {
  int potValue = analogRead(POT_PIN);

  // Map ADC (0-4095) ke PWM (0-255)
  int brightness = map(potValue, 0, 4095, 0, 255);

  ledcWrite(LED_PIN, brightness);

  Serial.print("Pot: ");
  Serial.print(potValue);
  Serial.print(" → PWM: ");
  Serial.print(brightness);
  Serial.print(" (");
  Serial.print((brightness * 100) / 255);
  Serial.println("%)");

  delay(50);
}
```

---

## 12.7 Tone Generation (Buzzer)

LEDC juga bisa digunakan untuk menghasilkan **nada** pada buzzer. Meskipun kit Bluino menggunakan **active buzzer** (yang menghasilkan nada tetap), teknik ini berguna jika kamu mengganti ke passive buzzer.

### Buzzer Active vs Passive

| Tipe | Active Buzzer (di Kit) | Passive Buzzer |
|------|----------------------|----------------|
| Cara kerja | Beri HIGH → bunyi | Beri PWM → nada sesuai frekuensi |
| Kontrol nada | ❌ Nada tetap | ✅ Bisa diubah |
| Fungsi | Digital output biasa | PWM / `tone()` |

### Contoh Tone (untuk passive buzzer)

```cpp
/*
 * BAB 12 - Program 4: Tone Generation
 * Menghasilkan nada pada passive buzzer menggunakan LEDC
 * (Jika kamu mengganti active buzzer dengan passive buzzer)
 *
 * Wiring: Passive Buzzer Custom → IO13
 */

#define BUZZER_PIN 13

// Frekuensi not musik (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

const int melody[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
                      NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
const int noteDuration = 300;

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12: Tone Generation (passive buzzer)");
}

void loop() {
  for (int i = 0; i < 8; i++) {
    // Detach dulu jika sebelumnya sudah di-attach
    ledcDetach(BUZZER_PIN);

    // Attach dengan frekuensi nada baru
    ledcAttach(BUZZER_PIN, melody[i], 8);
    ledcWrite(BUZZER_PIN, 128);  // 50% duty cycle

    Serial.print("Nada: ");
    Serial.print(melody[i]);
    Serial.println(" Hz");

    delay(noteDuration);

    // Diam antar nada
    ledcWrite(BUZZER_PIN, 0);
    delay(50);
  }

  delay(2000);
}
```

---

## 📝 Latihan

1. **Gamma correction:** LED manusia mempersepsikan cahaya secara logaritmik. Buat fungsi yang mengubah linear PWM (0–255) menjadi gamma-corrected PWM. (Formula: `output = pow(input/255.0, 2.2) * 255`)

2. **Candle flicker:** Buat efek lilin berkedip pada LED menggunakan `random()` untuk mengubah brightness secara acak.

3. **Multi-LED wave:** Buat 4 LED memiliki efek "gelombang" — masing-masing LED fade in/out dengan delay offset sehingga terlihat bergelombang.

4. **Duty cycle monitor:** Buat program yang membaca pot dan menampilkan duty cycle dalam format grafis di Serial Monitor (misalnya: `[████████░░░░░░░░] 50%`).

---

## 📚 Referensi

- [LEDC API — Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html)
- [ESP32 PWM — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-pwm-arduino-ide/)
- [PWM Frequency vs Resolution Calculator](https://deepbluembedded.com/esp32-pwm-tutorial-examples-analogwrite-arduino/)

---

[⬅️ BAB 11: Analog Input & ADC](../BAB-11/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 13 — DAC ➡️](../BAB-13/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

