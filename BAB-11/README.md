# BAB 11: Analog Input & ADC

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami cara kerja ADC (Analog-to-Digital Converter)
- Membaca sensor analog (Potensiometer, LDR) pada ESP32
- Memahami perbedaan ADC1 dan ADC2
- Mengatasi keterbatasan ADC ESP32 (non-linear, noise)
- Menggunakan `analogReadMilliVolts()` untuk pembacaan akurat

---

## 11.1 Apa itu ADC?

**ADC (Analog-to-Digital Converter)** mengubah tegangan analog (kontinu) menjadi nilai digital (diskrit) yang bisa diproses oleh mikrokontroler.

```
Tegangan Analog       ADC          Nilai Digital
  (0V – 3.3V)    ──────────→     (0 – 4095)

    0V     →  0
    1.65V  →  2048   (tengah)
    3.3V   →  4095   (maksimal)
```

### Spesifikasi ADC ESP32

| Parameter | Nilai |
|-----------|-------|
| Resolusi | 12-bit (0–4095) |
| Tegangan input range | 0V – 3.3V (default) |
| Jumlah channel | 18 total (ADC1: 8 ch, ADC2: 10 ch) |
| Sampling rate | Hingga 200 kSPS |
| Atenuasi default | 0 dB (0V – 1.1V) |
| Atenuasi full range | 11 dB (0V – 3.3V) |

---

## 11.2 ADC1 vs ADC2 — Perbedaan Krusial

| ADC | GPIO | Saat WiFi Aktif |
|-----|------|-----------------|
| **ADC1** | IO32, IO33, IO34, IO35, IO36(VP), IO39(VN) | ✅ **Bisa digunakan** |
| **ADC2** | IO0, IO2, IO4, IO12, IO13, IO14, IO15, IO25, IO26, IO27 | ❌ **TIDAK bisa digunakan** |

> ⚠️ **SANGAT PENTING:** Jika project menggunakan WiFi, **hanya gunakan ADC1** (IO32–IO39). ADC2 dikunci oleh WiFi driver.

### Pada Kit Bluino

| Komponen Analog | Pin Rekomendasi | ADC | WiFi Safe? |
|-----------------|----------------|-----|------------|
| Potensiometer 10K | **IO34** | ADC1_CH6 | ✅ |
| LDR | **IO35** | ADC1_CH7 | ✅ |

---

## 11.3 Program: Baca Potensiometer

### Wiring

```
Kit Bluino Header          ESP32 DEVKIT V1
─────────────────          ────────────────

Pot Custom ────────────── IO34
```

### Program

```cpp
/*
 * BAB 11 - Program 1: Baca Potensiometer
 * Membaca nilai analog dari potensiometer 10K
 *
 * Wiring: Potensiometer Custom → IO34 (ADC1_CH6)
 *
 * Catatan: Potensiometer pada kit sudah dirangkai sebagai
 * voltage divider antara GND dan 3.3V
 */

#define POT_PIN 34  // ADC1_CH6 — aman saat WiFi aktif

void setup() {
  Serial.begin(115200);

  // Set atenuasi untuk range penuh 0-3.3V
  analogSetAttenuation(ADC_11db);

  Serial.println("BAB 11: Baca Potensiometer");
  Serial.println("Putar potensiometer dan lihat nilainya...");
}

void loop() {
  // Baca nilai ADC (0-4095)
  int adcValue = analogRead(POT_PIN);

  // Baca dalam miliVolt (lebih akurat, tersedia di ESP32)
  int mV = analogReadMilliVolts(POT_PIN);

  // Konversi ke persentase (0-100%)
  float persen = (adcValue / 4095.0) * 100.0;

  Serial.print("ADC: ");
  Serial.print(adcValue);
  Serial.print(" | mV: ");
  Serial.print(mV);
  Serial.print(" | Persen: ");
  Serial.print(persen, 1);
  Serial.println("%");

  delay(200);
}
```

**Output contoh:**
```
ADC: 0    | mV: 12   | Persen: 0.0%
ADC: 1024 | mV: 823  | Persen: 25.0%
ADC: 2048 | mV: 1650 | Persen: 50.0%
ADC: 4095 | mV: 3300 | Persen: 100.0%
```

---

## 11.4 Program: Baca LDR (Sensor Cahaya)

### Rangkaian pada Kit Bluino

LDR dirangkai sebagai **voltage divider** dengan resistor tetap 10KΩ di atas dan LDR di bawah (sudah terpasang di PCB):

```
    3.3V
     │
    10KΩ (tetap, di PCB)
     │
     ├──── Vout → IO35 (ADC1_CH7)
     │
    LDR (resistansi berubah sesuai cahaya)
     │
    GND
```

**Logika pembacaan:**
- Cahaya **terang** → LDR resistansi kecil → Vout ditarik ke GND → ADC **rendah**
- Cahaya **gelap** → LDR resistansi besar → Vout ditarik ke 3.3V → ADC **tinggi**

### Program

```cpp
/*
 * BAB 11 - Program 2: Sensor Cahaya (LDR)
 * Membaca intensitas cahaya dan kategorisasi
 *
 * Wiring: LDR Custom → IO35 (ADC1_CH7)
 *
 * Cahaya terang → LDR resistansi kecil → Vout rendah → ADC rendah
 * Cahaya gelap  → LDR resistansi besar → Vout tinggi → ADC tinggi
 */

#define LDR_PIN 35  // ADC1_CH7

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);

  Serial.println("BAB 11: Sensor Cahaya LDR");
}

void loop() {
  int adcValue = analogRead(LDR_PIN);
  int mV = analogReadMilliVolts(LDR_PIN);

  // Kategorisasi cahaya (ADC rendah = terang, ADC tinggi = gelap)
  String kategori;
  if (adcValue < 500) {
    kategori = "Sangat Terang";
  } else if (adcValue < 1500) {
    kategori = "Terang";
  } else if (adcValue < 2500) {
    kategori = "Sedang";
  } else if (adcValue < 3500) {
    kategori = "Gelap";
  } else {
    kategori = "Sangat Gelap";
  }

  Serial.print("ADC: ");
  Serial.print(adcValue);
  Serial.print(" | mV: ");
  Serial.print(mV);
  Serial.print(" | Cahaya: ");
  Serial.println(kategori);

  delay(500);
}
```

---

## 11.5 Atenuasi ADC

ESP32 memiliki **attenuator internal** untuk mengubah range input:

| Atenuasi | Range Input | Resolusi Efektif | Penggunaan |
|----------|-------------|------------------|------------|
| `ADC_0db` | 0 – 1.1V | Terbaik | Sensor tegangan rendah |
| `ADC_2_5db` | 0 – 1.5V | Baik | — |
| `ADC_6db` | 0 – 2.2V | Sedang | — |
| `ADC_11db` | 0 – 3.3V | Terendah | **Paling umum** (default di ESP32 Arduino) |

```cpp
// Set atenuasi global (semua pin ADC)
analogSetAttenuation(ADC_11db);

// Set atenuasi per pin
analogSetPinAttenuation(34, ADC_11db);
```

> **Rekomendasi:** Gunakan `ADC_11db` untuk keperluan umum (range penuh 0–3.3V).

---

## 11.6 Keterbatasan ADC ESP32 & Solusinya

### Masalah 1: Non-Linearitas

ADC ESP32 **tidak linear di ujung-ujung range**:

```
Tegangan aktual vs ADC reading:

ADC    4095 ─ ─ ─ ─ ─ ─ ─ ─┐    ← Saturasi di ujung atas
value       ╱               │
           ╱   (akurat      │
          ╱    di tengah)    │
         ╱                   │
    0 ──┘─ ─ ─ ─ ─ ─ ─ ─ ─ ─
       0V                3.3V
            ↑          ↑
       Tidak akurat   Tidak akurat
        (0-0.1V)      (3.2-3.3V)
```

**Solusi:** Gunakan `analogReadMilliVolts()` — fungsi ini sudah menerapkan **koreksi kalibrasi internal** (eFuse calibration).

### Masalah 2: Noise

ADC ESP32 bisa memiliki noise yang signifikan. Pembacaan bisa fluktuasi ±20–50 digit.

**Solusi: Multi-sampling (rata-rata)**

```cpp
int bacaADCStabil(int pin, int jumlahSampel) {
  long total = 0;
  for (int i = 0; i < jumlahSampel; i++) {
    total += analogRead(pin);
    delayMicroseconds(100);  // Jeda singkat antar sampel
  }
  return total / jumlahSampel;
}

void loop() {
  int nilai = bacaADCStabil(34, 64);  // Rata-rata 64 sampel
  Serial.println(nilai);
  delay(200);
}
```

### Masalah 3: ADC2 + WiFi

ADC2 **dikunci** saat WiFi aktif. Tidak ada solusi selain menggunakan ADC1.

---

## 11.7 Program: Pot → LED Brightness (ADC → PWM)

Kombinasi ADC input dan PWM output untuk kontrol brightness LED:

```cpp
/*
 * BAB 11 - Program 3: Pot Kontrol LED Brightness
 * Putar potensiometer untuk mengatur kecerahan LED
 *
 * Wiring:
 *   Pot Custom → IO34 (ADC1_CH6)
 *   LED Custom → IO4  (PWM)
 */

#define POT_PIN 34
#define LED_PIN 4

// LEDC config
const int LEDC_FREQ = 5000;
const int LEDC_RESOLUTION = 8;  // 0-255

int bacaADCStabil(int pin, int samples) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
  }
  return total / samples;
}

void setup() {
  Serial.begin(115200);

  // Setup ADC
  analogSetAttenuation(ADC_11db);

  // Setup LED PWM
  ledcAttach(LED_PIN, LEDC_FREQ, LEDC_RESOLUTION);

  Serial.println("BAB 11: Pot → LED Brightness");
}

void loop() {
  // Baca pot (0-4095) dengan filtering
  int potValue = bacaADCStabil(POT_PIN, 16);

  // Map ke range PWM (0-255)
  int brightness = map(potValue, 0, 4095, 0, 255);

  // Set LED brightness
  ledcWrite(LED_PIN, brightness);

  Serial.print("Pot: ");
  Serial.print(potValue);
  Serial.print(" → LED: ");
  Serial.println(brightness);

  delay(50);
}
```

---

## 📝 Latihan

1. **Voltage meter:** Buat voltmeter sederhana yang menampilkan tegangan input (dalam Volt, bukan miliVolt) di Serial Monitor. Gunakan `analogReadMilliVolts()`.

2. **Light alarm:** Buat sistem yang membunyikan buzzer saat LDR mendeteksi gelap (ADC < 500). LED menyala sebagai indikator.

3. **ADC comparison:** Bandingkan 10 pembacaan `analogRead()` vs `analogReadMilliVolts()` pada tegangan tetap (pot di posisi tengah). Mana yang lebih stabil?

4. **Multi-sampling test:** Baca potensiometer dengan 1, 10, 50, dan 100 sampel rata-rata. Bandingkan kestabilan pembacaan — berapa sampel yang optimal?

---

## 📚 Referensi

- [analogRead() — Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html)
- [ESP32 ADC — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/)
- [ESP32 ADC Calibration — Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_calibration.html)

---

[⬅️ BAB 10: Interrupt & ISR](../BAB-10/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 12 — PWM ➡️](../BAB-12/README.md)
