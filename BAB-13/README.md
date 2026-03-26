# BAB 13: DAC — Output Analog Asli

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami perbedaan PWM dan DAC
- Menggunakan DAC ESP32 untuk menghasilkan tegangan analog
- Membuat sinyal waveform (sawtooth, sine wave)
- Menggunakan DAC untuk audio sederhana

---

## 13.1 DAC vs PWM — Apa Bedanya?

| Aspek | PWM | DAC |
|-------|-----|-----|
| Sinyal | Digital ON/OFF cepat | Tegangan analog asli |
| Output | Rata-rata tegangan (perlu filter) | Tegangan stabil |
| Resolusi | 1–16 bit (tergantung konfigurasi) | **8-bit** (0–255) pada ESP32 |
| Jumlah channel | 16 | **2** (hanya IO25 dan IO26) |
| Cocok untuk | LED, motor, servo | Audio, referensi tegangan, sinyal analog |

```
PWM 50%:                    DAC 50%:
HIGH ─┐   ┌─┐   ┌─         3.3V ─
LOW   └───┘ └───┘           1.65V ─────────────── (stabil)
(kotak-kotak)                0V ─
```

---

## 13.2 DAC pada ESP32

ESP32 memiliki **2 channel DAC** (8-bit):

| Channel | GPIO | Status pada Kit Bluino |
|---------|------|----------------------|
| DAC1 | **IO25** | ✅ Tersedia (custom) |
| DAC2 | **IO26** | ✅ Tersedia (custom) |

### Cara Penggunaan

```cpp
dacWrite(pin, value);
// pin   = 25 (DAC1) atau 26 (DAC2)
// value = 0-255
//   0   = 0V
//   128 = ~1.65V
//   255 = ~3.3V
```

> **Catatan:** DAC ESP32 hanya 8-bit (256 level). Untuk audio berkualitas tinggi, dibutuhkan DAC eksternal (misalnya via I2S).

---

## 13.3 Program: Tegangan Output Terkontrol

```cpp
/*
 * BAB 13 - Program 1: DAC Output
 * Menghasilkan tegangan analog yang bisa diatur
 *
 * Pin: IO25 (DAC1) — ukur output dengan multimeter
 */

#define DAC_PIN 25  // DAC1

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 13: DAC Output");
  Serial.println("Ukur tegangan di IO25 dengan multimeter");
  Serial.println();
}

void loop() {
  // Output beberapa level tegangan
  int levels[] = {0, 64, 128, 192, 255};

  for (int i = 0; i < 5; i++) {
    dacWrite(DAC_PIN, levels[i]);

    float voltage = (levels[i] / 255.0) * 3.3;
    Serial.print("DAC Value: ");
    Serial.print(levels[i]);
    Serial.print(" → Tegangan: ");
    Serial.print(voltage, 2);
    Serial.println(" V");

    delay(3000);  // Tahan 3 detik agar bisa diukur
  }

  Serial.println("--- Siklus selesai ---");
  Serial.println();
}
```

---

## 13.4 Program: Sinyal Sawtooth (Gigi Gergaji)

```cpp
/*
 * BAB 13 - Program 2: Sawtooth Wave
 * Menghasilkan sinyal gigi gergaji pada DAC
 *
 * Pin: IO25 (DAC1) — lihat dengan oscilloscope atau multimeter
 */

#define DAC_PIN 25

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 13: Sawtooth Wave Generator");
}

void loop() {
  // Naik linear 0→255, lalu langsung turun ke 0
  for (int value = 0; value < 256; value++) {
    dacWrite(DAC_PIN, value);
    delayMicroseconds(100);  // ~25.6ms per siklus → ~39 Hz
  }
}
```

---

## 13.5 Program: Sinyal Sine Wave

```cpp
/*
 * BAB 13 - Program 3: Sine Wave Generator
 * Menghasilkan sinyal gelombang sinus pada DAC
 *
 * Pin: IO25 (DAC1)
 */

#define DAC_PIN 25

// Lookup table sine wave (256 titik, 0-255)
const uint8_t sineTable[256] = {
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
  176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
  218,220,222,224,226,228,230,232,234,235,237,239,240,241,243,244,
  245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
  255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
  245,244,243,241,240,239,237,235,234,232,230,228,226,224,222,220,
  218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
  176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
   79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
   37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 16, 15, 14, 12, 11,
   10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
    0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
   10, 11, 12, 14, 15, 16, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
   37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
   79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124
};

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 13: Sine Wave Generator");
}

void loop() {
  for (int i = 0; i < 256; i++) {
    dacWrite(DAC_PIN, sineTable[i]);
    delayMicroseconds(100);  // ~26ms per siklus → ~39 Hz
  }
}
```

---

## 13.6 Aplikasi DAC

| Aplikasi | Cara |
|----------|------|
| **Referensi tegangan** | Menghasilkan tegangan presisi untuk kalibrasi |
| **Audio sederhana** | Sine wave pada speaker/buzzer (kualitas rendah) |
| **Kontrol analog** | Mengontrol komponen yang butuh tegangan analog |
| **Sinyal test** | Generate waveform untuk pengujian |

> **Keterbatasan:** DAC ESP32 hanya 8-bit (256 level). Untuk audio berkualitas, gunakan **I2S + DAC eksternal** (akan dibahas di BAB 73).

---

## 📝 Latihan

1. **Voltage staircase:** Buat DAC menghasilkan "tangga" tegangan: 0V → 0.5V → 1.0V → 1.5V → 2.0V → 2.5V → 3.0V → 3.3V, masing-masing ditahan 2 detik. Ukur dengan multimeter.

2. **Triangle wave:** Buat sinyal segitiga (naik linear, turun linear) pada DAC. Berbeda dengan sawtooth, segitiga turun secara halus.

3. **Pot → DAC:** Baca potensiometer (ADC pada IO34), lalu output nilainya ke DAC (IO25). Ini membuat "pass-through" analog — ukur output DAC dan bandingkan dengan input pot.

4. **Dual waveform:** Gunakan kedua DAC (IO25 dan IO26) untuk menghasilkan sine dan cosine wave secara bersamaan (90° phase shift).

---

## 📚 Referensi

- [dacWrite() — Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/dac.html)
- [ESP32 DAC — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-dac-audio-arduino/)
- [ESP32 Technical Reference — DAC](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

---

[⬅️ BAB 12: PWM dengan LEDC](../BAB-12/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 14 — Capacitive Touch ➡️](../BAB-14/README.md)
