# BAB 38: Servo Motor

> ✅ **Prasyarat:** Selesaikan **BAB 11 (Analog Input & ADC)**, **BAB 12 (PWM dengan LEDC)**, dan **BAB 16 (Non-Blocking Programming)**. Servo pada Bluino Kit terhubung ke **Servo Header** (3 pin: 5V, GND, Signal) — hubungkan kabel jumper dari pin Signal header ke GPIO ESP32 sesuai panduan bab ini.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **prinsip kerja motor servo** dan perbedaannya dengan motor DC biasa.
- Menjelaskan **hubungan antara lebar pulsa PWM (50Hz) dan sudut posisi** servo.
- Menggunakan library **ESP32Servo** secara profesional untuk mengontrol posisi dan gerakan.
- Membangun **gerakan sweep non-blocking** menggunakan millis() tanpa memblokir CPU.
- Mengintegrasikan servo dengan **potensiometer ADC** untuk kontrol posisi real-time.
- Merancang sistem **preset posisi multi-titik** yang dinavigasi via tombol.
- Membangun **Finite State Machine (FSM) multi-mode** untuk kontrol servo tingkat lanjut.
- Memahami **batasan mekanik dan daya** servo agar tidak merusak perangkat.

---

## 38.1 Motor Servo — Prinsip Kerja

### Apa Itu Motor Servo?

Motor servo adalah aktuator rotasi yang **mengontrol POSISI sudut**, bukan kecepatan. Tidak seperti motor DC biasa yang berputar bebas selama diberi tegangan, servo memiliki **sistem umpan balik internal** berupa potensiometer yang memungkinkan servo "tahu" di mana posisinya saat ini dan bergerak menuju posisi yang diperintahkan.

```
PERBANDINGAN MOTOR DC vs SERVO:

  Motor DC Biasa:                    Motor Servo:
  ┌────────────────────┐             ┌──────────────────────────────────┐
  │  (+)  (-)          │             │  VCC   GND   Signal              │
  │   │    │           │             │   │     │     │   (3 kabel)      │
  │  [MOTOR]           │             │  5V    0V   PWM 50Hz             │
  │   ↓ berputar       │             │                                  │
  │   bebas terus!     │             │  [Motor] ─▶ [Gearbox] ─▶        │
  └────────────────────┘             │  [Potensiometer feedback]        │
  ▶ Tidak tahu posisinya             │  [IC Kontrol PID internal]       │
  ▶ Perlu encoder tambahan           │   ↓ berhenti di sudut TEPAT!     │
  ▶ 2 pin (+ dan -)                  └──────────────────────────────────┘
                                     ▶ Sudah ada umpan balik posisi!
                                     ▶ Presisi 0° – 180° (standar)
                                     ▶ 3 kabel (Power + Signal)
```

### Anatomi Servo Motor (SG90 — Tipe Umum di Kit)

```
STRUKTUR INTERNAL SERVO SG90:

  Kabel MERAH  (VCC)  ──── 5V DC (power motor + IC kontrol)
  Kabel COKLAT (GND)  ──── GND
  Kabel ORANYE (Signal) ── PWM 50Hz dari ESP32

  Di dalam casing servo:
  ┌────────────────────────────────────────────┐
  │   [Konektor 3P]                            │
  │        │                                   │
  │   [IC Kontrol] ←── Sinyal PWM              │
  │        │                                   │
  │   [Driver H-Bridge]                        │
  │        │                                   │
  │   [Motor DC Kecil] ──▶ [Gearbox Plastik]  │
  │                               │           │
  │                      [Potensiometer 5KΩ]  │
  │                               │           │
  │                         [Output Shaft]    │
  └────────────────────────────────────────────┘

  IC kontrol membandingkan sinyal PWM (perintah posisi)
  dengan pembacaan potensiometer (posisi aktual).
  Jika belum sama → motor diputar sampai sama (feedback loop PID).
```

---

## 38.2 Sinyal PWM untuk Servo — Teori Penting

Inilah inti teknis yang HARUS dipahami untuk bisa mengontrol servo:

```
SINYAL KONTROL SERVO (Standard Hobby Servo):

  Frekuensi  : 50 Hz  (periode = 20ms)
  Lebar Pulsa: 500µs  s/d 2500µs (umumnya)

  Representasi visual satu periode (20ms):

  HIGH ──┐           ┌─── LOW (sisanya)
         │  PULSA    │
         │← t µs →  │
  LOW ───┘           └─────────────────
  │←────────── 20ms (total) ──────────│

  PEMETAAN LEBAR PULSA → SUDUT:
  ┌─────────────────────────────────────────────────────────┐
  │  Pulsa  500µs  → Sudut   0°  (posisi paling kiri)      │
  │  Pulsa 1500µs  → Sudut  90°  (posisi tengah/netral)    │
  │  Pulsa 2500µs  → Sudut 180°  (posisi paling kanan)     │
  └─────────────────────────────────────────────────────────┘

  PERHATIAN: Angka 500–2500µs adalah nilai UMUM untuk SG90.
  Servo berbeda-beda. Uji selalu dengan servo fisik kamu!
  Melebihi batas fisik mekanik = merusak gearbox plastik!
```

### Mengapa Harus Library ESP32Servo?

Servo **TIDAK BISA** dikontrol langsung dengan `analogWrite()` atau LEDC biasa tanpa konfigurasi mendalam. Masalahnya:
- LEDC default ESP32 tidak dikonfigurasi untuk resolusi waktu µs yang tepat pada 50Hz
- Kalkulasi duty cycle manual dari pulse µs sangat rentan error

Library **ESP32Servo** menangani semua ini secara abstrak dan menggunakan hardware LEDC secara benar di balik layar.

```
CARA INSTALASI ESP32Servo (Arduino IDE 2.x):
  1. Tools → Manage Libraries...
  2. Cari: "ESP32Servo"
  3. Install (oleh Kevin Harrington / madhephaestus)
  4. Versi yang direkomendasikan: ≥ 0.13.0
  5. Restart Arduino IDE jika diminta
```

---

## 38.3 Koneksi Hardware

### Wiring Servo ke ESP32 (Bluino Kit)

```
KONEKSI SERVO (Via Servo Header + Jumper):
┌────────────────────────────────────────────────────────────┐
│  Servo Header Bluino Kit          ESP32 DEVKIT V1          │
│  ──────────────────────           ──────────────────────   │
│  5V   (kabel MERAH)  ─────────── 5V (pin header kit)      │
│  GND  (kabel COKLAT) ─────────── GND                       │
│  Signal (kabel ORANYE) ───────── GPIO (rek: IO25)          │
└────────────────────────────────────────────────────────────┘

⚠️  JANGAN hubungkan kabel daya servo (MERAH) ke pin 3.3V!
    Servo membutuhkan 5V untuk torsi dan range penuh.
    Pada 3.3V: servo akan gemetar, range sudut berkurang, 
    dan bisa merusak IC kontrol servo jangka panjang.
```

### Pin yang Direkomendasikan untuk Servo Signal

| Pin GPIO | Status | Catatan |
|----------|--------|---------|
| **IO25** | ✅ Aman | DAC1 — bersih, tidak ada konflik, **direkomendasikan** |
| **IO26** | ✅ Aman | DAC2 — aman (jika tidak dipakai relay) |
| **IO32** | ✅ Aman | Digital I/O biasa, mendukung LEDC |
| **IO33** | ✅ Aman | Digital I/O biasa, mendukung LEDC |
| IO12 | ⚠️ Hindari | Strapping pin + sudah dipakai WS2812 |
| IO1, IO3 | ❌ Larang | TX0/RX0 Serial — jangan dipakai |
| IO34–IO39 | ❌ Larang | Input-only, tidak bisa output PWM |

> 💡 **Untuk seluruh contoh program di bab ini**, kita akan menggunakan **IO25** sebagai pin sinyal servo.

### ⚠️ Peringatan Konsumsi Daya Servo

```
KONSUMSI ARUS SERVO SG90:
  Idle (tidak bergerak)    : ~10 mA
  Bergerak tanpa beban     : ~100–200 mA
  Stall (tertahan paksa)   : ~600–800 mA ← BERBAHAYA!

  USB PC/laptop            : ~500 mA total (termasuk ESP32 ~100mA)

  RISIKO: Servo stall + ESP32 aktif dapat melebihi batas USB
          → ESP32 brownout reset secara tiba-tiba!

  SOLUSI:
  ✅ Gunakan adaptor 5V 1A atau lebih sebagai sumber daya
  ✅ Jangan paksa servo memutar melampaui batas fisik 0°–180°
  ✅ Tambahkan kapasitor 100µF elco di jalur 5V servo
     untuk menyerap lonjakan arus transien saat servo bergerak
```

---

## 38.4 Program 1: Hello World Servo — Inisialisasi & Sweep Dasar

Program pertama yang wajib dikuasai: menggerakkan servo perlahan dari 0° ke 180° lalu kembali, secara non-blocking.

```cpp
/*
 * BAB 38 - Program 1: Hello World Servo — Sweep Non-Blocking
 *
 * Tujuan:
 *   Menguasai inisialisasi ESP32Servo dan menggerakkan servo
 *   dalam pola sweep 0°→180°→0° secara non-blocking (tanpa delay()).
 *
 * Hardware:
 *   Servo Signal → IO25 (via jumper dari Servo Header)
 *   Servo VCC    → 5V  (dari header kit)
 *   Servo GND    → GND
 *
 * Gerakan:
 *   Servo bergerak dari 0° → 180° (maju), lalu 180° → 0° (mundur),
 *   berulang terus dengan kecepatan yang bisa dikonfigurasi.
 */

#include <ESP32Servo.h>

// ── Konfigurasi ──────────────────────────────────────────────────────────
#define SERVO_PIN    25   // Pin sinyal servo (via jumper Custom)

// Batas sudut dan pulse — sesuaikan dengan servo fisikmu jika perlu
const int SUDUT_MIN = 0;
const int SUDUT_MAX = 180;

// Lebar pulsa µs untuk servo SG90 (kalibrasi jika servo common tidak akurat)
const int PULSA_MIN_US = 500;   // Pulsa untuk 0°
const int PULSA_MAX_US = 2500;  // Pulsa untuk 180°

// Kecepatan gerakan: interval antar langkah 1°
const unsigned long STEP_MS = 15UL;  // 15ms per derajat = ~2.7 detik untuk 180°

// ── State Non-Blocking ───────────────────────────────────────────────────
Servo myServo;

int  sudutSaat   = 0;
int  arahLangkah = 1;   // +1 = maju (naik), -1 = mundur (turun)
unsigned long tStep  = 0;

void setup() {
  Serial.begin(115200);

  // Daftarkan servo dengan batas pulsa yang tepat
  myServo.attach(SERVO_PIN, PULSA_MIN_US, PULSA_MAX_US);

  // Pindahkan ke posisi awal dengan aman
  myServo.write(SUDUT_MIN);
  sudutSaat = SUDUT_MIN;
  delay(500); // Satu-satunya delay() yang diperbolehkan: menunggu servo
              // mencapai posisi awal sebelum program loop dimulai

  Serial.println("=== BAB 38 Program 1: Servo Sweep ===");
  Serial.printf("Range: %d° – %d° | Kecepatan: %lums/derajat\n",
    SUDUT_MIN, SUDUT_MAX, STEP_MS);
  Serial.println("--------------------------------------");
}

void loop() {
  unsigned long now = millis();

  // Gerakkan servo satu langkah (1°) setiap STEP_MS
  if (now - tStep >= STEP_MS) {
    tStep = now;

    myServo.write(sudutSaat);

    // Log setiap 10° agar Serial Monitor tidak banjir data
    if (sudutSaat % 10 == 0) {
      Serial.printf("[SERVO] Sudut: %3d°  Arah: %s\n",
        sudutSaat, arahLangkah > 0 ? "→ MAJU " : "← MUNDUR");
    }

    // Langkah berikutnya
    sudutSaat += arahLangkah;

    // Balik arah saat mencapai batas
    if (sudutSaat >= SUDUT_MAX) {
      sudutSaat = SUDUT_MAX;
      arahLangkah = -1;
    } else if (sudutSaat <= SUDUT_MIN) {
      sudutSaat = SUDUT_MIN;
      arahLangkah = 1;
    }
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 38 Program 1: Servo Sweep ===
Range: 0° – 180° | Kecepatan: 15ms/derajat
--------------------------------------
[SERVO] Sudut:   0°  Arah: → MAJU
[SERVO] Sudut:  10°  Arah: → MAJU
[SERVO] Sudut:  20°  Arah: → MAJU
...
[SERVO] Sudut: 180°  Arah: ← MUNDUR
[SERVO] Sudut: 170°  Arah: ← MUNDUR
...
[SERVO] Sudut:   0°  Arah: → MAJU
```

> 💡 **Mengapa `delay(500)` di setup() DIPERBOLEHKAN?** Satu-satunya penggunaan `delay()` yang sah dalam embedded systems adalah saat startup — ketika kita perlu memastikan servo telah mencapai posisi fisik awal sebelum loop program berjalan. Di dalam `loop()`, kita TIDAK BOLEH menggunakan `delay()` karena akan memblokir semua sensor dan input lain.

---

## 38.5 Program 2: Kontrol Servo via Potensiometer (ADC Real-Time)

Program ini membangun **antarmuka kontrol analog yang responsif**: posisi servo mengikuti putaran potensiometer secara real-time. Ini adalah fondasi dari banyak aplikasi robotika — joystick, kamera gimbal, dan lengan robot.

```cpp
/*
 * BAB 38 - Program 2: Kontrol Servo via Potensiometer (ADC → Sudut)
 *
 * Fitur:
 *   ▶ Baca ADC potensiometer 12-bit (0–4095)
 *   ▶ Petakan nilai ADC ke sudut servo (0°–180°)
 *   ▶ Filter EMA (Exponential Moving Average) untuk mencegah
 *     servo bergetar akibat noise ADC
 *   ▶ Deadband: servo tidak bergerak jika perubahan < 1°
 *     (mencegah hunting/getaran konstan)
 *   ▶ Dashboard lengkap di Serial Monitor
 *
 * Hardware:
 *   Servo       → IO25 (via Servo Header)
 *   Potensiometer → IO34 (via jumper Custom, input-only aman)
 *
 * Catatan:
 *   ADC ESP32 pada IO34/IO35 adalah input-only (tidak ada pull-up internal)
 *   dan tidak terhubung ke GPIO lain — ideal untuk potensiometer.
 */

#include <ESP32Servo.h>

// ── Konfigurasi Hardware ─────────────────────────────────────────────────
#define SERVO_PIN  25
#define POT_PIN    34   // Input-only ADC pin (aman, tidak ada konflik)

const int PULSA_MIN_US = 500;
const int PULSA_MAX_US = 2500;

// ── Konfigurasi Kontrol ──────────────────────────────────────────────────
const float EMA_ALPHA   = 0.15f;  // Koefisien filter: lebih kecil = lebih mulus
const int   DEADBAND_DEG = 1;     // Servo tidak bergerak jika delta < 1°
const unsigned long ADC_READ_MS  = 50UL;   // Baca ADC setiap 50ms
const unsigned long SERVO_UPD_MS = 20UL;   // Update servo setiap 20ms (50Hz)

// ── State ────────────────────────────────────────────────────────────────
Servo myServo;

float emaADC    = 2047.0f;  // Inisialisasi di tengah range
int   sudutTarget = 90;
int   sudutSaat   = 90;

unsigned long tADC   = 0;
unsigned long tServo = 0;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // 12-bit ADC: 0–4095

  digitalWrite(SERVO_PIN, LOW);  // Aman sebelum attach
  myServo.attach(SERVO_PIN, PULSA_MIN_US, PULSA_MAX_US);
  myServo.write(90);
  sudutSaat = 90;
  delay(500);

  // Pre-inisialisasi EMA dengan bacaan pertama
  emaADC = (float)analogRead(POT_PIN);

  Serial.println("=== BAB 38 Program 2: Servo Potensiometer ===");
  Serial.println("Putar potensiometer untuk menggerakkan servo.");
  Serial.println("---------------------------------------------");
}

void loop() {
  unsigned long now = millis();

  // ── Baca ADC dan filter dengan EMA ───────────────────────────────────
  if (now - tADC >= ADC_READ_MS) {
    tADC = now;

    int rawADC = analogRead(POT_PIN);
    emaADC = EMA_ALPHA * rawADC + (1.0f - EMA_ALPHA) * emaADC;

    // Petakan ADC (0–4095) ke sudut (0°–180°)
    sudutTarget = map((long)emaADC, 0, 4095, 0, 180);
    sudutTarget = constrain(sudutTarget, 0, 180);
  }

  // ── Update posisi servo dengan deadband ──────────────────────────────
  if (now - tServo >= SERVO_UPD_MS) {
    tServo = now;

    int delta = abs(sudutTarget - sudutSaat);

    if (delta > DEADBAND_DEG) {
      sudutSaat = sudutTarget;
      myServo.write(sudutSaat);

      Serial.printf("[SERVO] ADC: %4d | EMA: %6.1f | Sudut: %3d°\n",
        analogRead(POT_PIN), emaADC, sudutSaat);
    }
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 38 Program 2: Servo Potensiometer ===
Putar potensiometer untuk menggerakkan servo.
---------------------------------------------
[SERVO] ADC: 2103 | EMA: 2089.4 | Sudut:  91°
[SERVO] ADC: 2847 | EMA: 2204.7 | Sudut:  96°
[SERVO] ADC: 3612 | EMA: 2479.5 | Sudut: 108°
[SERVO] ADC: 4001 | EMA: 2807.8 | Sudut: 123°
[SERVO] ADC: 4050 | EMA: 3086.6 | Sudut: 135°
```

> 💡 **Mengapa Filter EMA Wajib untuk Servo?** ADC ESP32 memiliki noise inheren ±5–30 LSB yang akan menyebabkan nilai sudut meloncat-loncat di sekitar titik setimbang, sehingga servo terus-menerus bergerak kecil (hunting). Filter EMA menghaluskan nilai sehingga servo bergerak dengan mulus. Parameter `EMA_ALPHA = 0.15` berarti nilai baru hanya memiliki bobot 15% terhadap nilai EMA — makin kecil angkanya, makin halus gerakannya tapi makin lambat responnya. Sesuaikan sesuai kebutuhanmu!

---

## 38.6 Program 3: Preset Posisi Multi-Titik via Tombol

Program ini membangun sistem navigasi posisi servo yang praktis: dua tombol (Maju/Mundur) menavigasi antara 5 posisi preset yang sudah ditentukan — seperti sistem kontrol kamera CCTV multi-posisi atau lengan robot dengan waypoints.

```cpp
/*
 * BAB 38 - Program 3: Preset Posisi Multi-Titik via Tombol
 *
 * Fitur:
 *   ▶ 5 posisi preset yang bisa dinavigasi maju/mundur
 *   ▶ 2 tombol: BTN_NEXT (maju ke preset berikut) & BTN_PREV (mundur)
 *   ▶ Gerakan SMOOTH: servo tidak langsung lompat — bergerak bertahap
 *   ▶ Debounced button non-blocking
 *   ▶ Konfirmasi posisi di Serial Monitor
 *
 * Hardware:
 *   Servo     → IO25 (via Servo Header)
 *   BTN_NEXT  → IO32 (pull-down 10KΩ bawaan kit, aktif HIGH)
 *   BTN_PREV  → IO33 (pull-down 10KΩ bawaan kit, aktif HIGH)
 *
 * Catatan:
 *   Bluino Kit memiliki 2 push button dengan pull-down internal.
 *   Gunakan kedua tombol tersebut sebagai BTN_NEXT dan BTN_PREV.
 */

#include <ESP32Servo.h>

// ── Konfigurasi Hardware ─────────────────────────────────────────────────
#define SERVO_PIN   25
#define BTN_NEXT    32  // Tombol maju (next preset)
#define BTN_PREV    33  // Tombol mundur (prev preset)

const int PULSA_MIN_US = 500;
const int PULSA_MAX_US = 2500;

// ── Preset Posisi ────────────────────────────────────────────────────────
const int PRESET_COUNT = 5;
const int presetSudut[PRESET_COUNT] = { 0, 45, 90, 135, 180 };
const char* presetNama[PRESET_COUNT] = {
  "HOME (0°)", "45°", "CENTER (90°)", "135°", "END (180°)"
};

int presetSaat = 2; // Mulai dari tengah (90°)

// ── Gerakan Smooth ───────────────────────────────────────────────────────
int  sudutSaat   = 90;
int  sudutTarget = 90;
const unsigned long SMOOTH_MS = 10UL;  // Update posisi setiap 10ms (smooth)
unsigned long tSmooth = 0;

// ── Debounce Tombol Universal ─────────────────────────────────────────────
struct Button {
  uint8_t pin;
  bool lastPhysical;
  bool stable;
  bool justPressed;
  unsigned long tEdge;
};

Button btnNext = { BTN_NEXT, false, false, false, 0 };
Button btnPrev = { BTN_PREV, false, false, false, 0 };
const unsigned long DEBOUNCE_MS = 50UL;

void updateButton(Button &btn, unsigned long now) {
  btn.justPressed = false;
  bool reading = (digitalRead(btn.pin) == HIGH);
  if (reading != btn.lastPhysical) btn.tEdge = now;
  if ((now - btn.tEdge) >= DEBOUNCE_MS) {
    if (reading != btn.stable) {
      btn.stable = reading;
      if (btn.stable) btn.justPressed = true;
    }
  }
  btn.lastPhysical = reading;
}

// ── Kontrol Servo ────────────────────────────────────────────────────────
Servo myServo;

void pindahKePreset(int idx) {
  presetSaat  = constrain(idx, 0, PRESET_COUNT - 1);
  sudutTarget = presetSudut[presetSaat];
  Serial.printf("[PRESET] → %s | Target: %d°\n",
    presetNama[presetSaat], sudutTarget);
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_NEXT, INPUT);
  pinMode(BTN_PREV, INPUT);

  myServo.attach(SERVO_PIN, PULSA_MIN_US, PULSA_MAX_US);
  myServo.write(presetSudut[presetSaat]);
  sudutSaat   = presetSudut[presetSaat];
  sudutTarget = sudutSaat;
  delay(500);

  Serial.println("=== BAB 38 Program 3: Preset Posisi Servo ===");
  Serial.println("IO32 = Next Preset | IO33 = Prev Preset");
  Serial.println("---------------------------------------------");
  for (int i = 0; i < PRESET_COUNT; i++) {
    Serial.printf("  Preset %d: %s\n", i, presetNama[i]);
  }
  Serial.printf("\nPosisi awal: %s\n", presetNama[presetSaat]);
}

void loop() {
  unsigned long now = millis();

  updateButton(btnNext, now);
  updateButton(btnPrev, now);

  // ── Navigasi Preset ───────────────────────────────────────────────────
  if (btnNext.justPressed && presetSaat < PRESET_COUNT - 1) {
    pindahKePreset(presetSaat + 1);
  }
  if (btnPrev.justPressed && presetSaat > 0) {
    pindahKePreset(presetSaat - 1);
  }

  // ── Gerakan Smooth menuju target ─────────────────────────────────────
  if (now - tSmooth >= SMOOTH_MS) {
    tSmooth = now;

    if (sudutSaat < sudutTarget) {
      sudutSaat++;
      myServo.write(sudutSaat);
    } else if (sudutSaat > sudutTarget) {
      sudutSaat--;
      myServo.write(sudutSaat);
    }
    // Jika sudah sampai target: tidak ada aksi (efisien!)
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 38 Program 3: Preset Posisi Servo ===
IO32 = Next Preset | IO33 = Prev Preset
---------------------------------------------
  Preset 0: HOME (0°)
  Preset 1: 45°
  Preset 2: CENTER (90°)
  Preset 3: 135°
  Preset 4: END (180°)

Posisi awal: CENTER (90°)
[PRESET] → 135° | Target: 135°
[PRESET] → END (180°) | Target: 180°
[PRESET] → 135° | Target: 135°
```

> 💡 **Arsitektur Gerakan Smooth:** Perhatikan bahwa kita **tidak langsung memanggil `myServo.write(sudutTarget)`** saat tombol ditekan. Kita hanya mengubah variabel `sudutTarget`, lalu di dalam timer 10ms kita menggerakkan `sudutSaat` satu derajat per langkah menuju target. Hasilnya: servo **selalu bergerak mulus** berapa pun jaraknya. Inilah pola *interpolasi linier posisi* yang digunakan di dunia robotika dan CNC!

---

## 38.7 Program 4: FSM Multi-Mode Servo

Program puncak BAB 38 ini menggabungkan semua teknik sebelumnya ke dalam **Finite State Machine (FSM) 4-mode** yang bisa dinavigasi via long-press tombol: Mode Manual Preset, Mode Auto-Sweep, Mode ADC Potensiometer, dan Mode Standby (terkunci di 90°).

```cpp
/*
 * BAB 38 - Program 4: FSM Multi-Mode Servo
 *
 * Empat mode operasi:
 *   MODE_PRESET  → Navigasi 5 posisi via short-press tombol
 *   MODE_SWEEP   → Auto-sweep 0°↔180° otomatis
 *   MODE_ADC     → Posisi mengikuti potensiometer real-time
 *   MODE_STANDBY → Servo terkunci di 90° (netral)
 *
 * Navigasi mode:
 *   Tekan LAMA (>1 detik) BTN_NEXT → Maju ke mode berikutnya
 *   Tekan PENDEK          BTN_NEXT → Aksi dalam mode (next preset / toggle arah sweep)
 *   Tekan PENDEK          BTN_PREV → Aksi terbalik (prev preset)
 *
 * Hardware:
 *   Servo         → IO25 (via Servo Header)
 *   Potensiometer → IO34 (via jumper Custom)
 *   BTN_NEXT      → IO32 (pull-down kit, aktif HIGH)
 *   BTN_PREV      → IO33 (pull-down kit, aktif HIGH)
 */

#include <ESP32Servo.h>

// ── Konfigurasi Hardware ─────────────────────────────────────────────────
#define SERVO_PIN  25
#define POT_PIN    34
#define BTN_NEXT   32
#define BTN_PREV   33

const int PULSA_MIN_US = 500;
const int PULSA_MAX_US = 2500;

// ── Definisi Mode FSM ────────────────────────────────────────────────────
enum ServoMode { MODE_PRESET, MODE_SWEEP, MODE_ADC, MODE_STANDBY, MODE_COUNT };
const char* namaMode[] = { "PRESET   ", "SWEEP    ", "ADC POT  ", "STANDBY  " };
ServoMode currentMode  = MODE_PRESET;

// ── State Servo Global ───────────────────────────────────────────────────
Servo myServo;
int   sudutSaat   = 90;
int   sudutTarget = 90;

void gerakSmooth(unsigned long now);
unsigned long tSmooth = 0;

void setSudutTarget(int sudut) {
  sudutTarget = constrain(sudut, 0, 180);
}

// ── Debounce & Long Press ─────────────────────────────────────────────────
const unsigned long DEBOUNCE_MS   =   50UL;
const unsigned long LONGPRESS_MS  = 1000UL;

struct Button {
  uint8_t  pin;
  bool     physical, stable, shortPress, longPress, longFired;
  unsigned long tEdge, tDown;
};

Button btnN = { BTN_NEXT, false, false, false, false, false, 0, 0 };
Button btnP = { BTN_PREV, false, false, false, false, false, 0, 0 };

void updateBtn(Button &b, unsigned long now) {
  b.shortPress = b.longPress = false;
  bool rd = (digitalRead(b.pin) == HIGH);
  if (rd != b.physical) b.tEdge = now;
  if ((now - b.tEdge) >= DEBOUNCE_MS) {
    if (rd != b.stable) {
      b.stable = rd;
      if (b.stable) { b.tDown = now; b.longFired = false; }
      else {
        unsigned long dur = now - b.tDown;
        if (!b.longFired && dur >= DEBOUNCE_MS && dur < LONGPRESS_MS)
          b.shortPress = true;
      }
    }
  }
  if (b.stable && !b.longFired && (now - b.tDown) >= LONGPRESS_MS) {
    b.longPress = b.longFired = true;
  }
  b.physical = rd;
}

// ── MODE_PRESET ───────────────────────────────────────────────────────────
const int PRESET_COUNT          = 5;
const int presetSudut[PRESET_COUNT] = { 0, 45, 90, 135, 180 };
int  presetIdx = 2;

void initPreset() {
  setSudutTarget(presetSudut[presetIdx]);
  Serial.printf("[PRESET] Posisi: %d° (idx %d/%d)\n",
    presetSudut[presetIdx], presetIdx, PRESET_COUNT - 1);
}

void updatePreset(bool next, bool prev) {
  if (next && presetIdx < PRESET_COUNT - 1) { presetIdx++; initPreset(); }
  if (prev && presetIdx > 0)               { presetIdx--; initPreset(); }
}

// ── MODE_SWEEP ────────────────────────────────────────────────────────────
int  sweepArah   = 1;   // +1 atau -1
const unsigned long SWEEP_STEP_MS = 12UL;
unsigned long tSweep = 0;

void updateSweep(unsigned long now, bool toggle) {
  if (toggle) {
    sweepArah = -sweepArah;
    Serial.printf("[SWEEP] Arah dibalik → %s\n", sweepArah > 0 ? "Maju →" : "Mundur ←");
  }
  if (now - tSweep >= SWEEP_STEP_MS) {
    tSweep = now;
    int next = sudutSaat + sweepArah;
    if (next >= 180) { next = 180; sweepArah = -1; }
    if (next <=   0) { next =   0; sweepArah =  1; }
    setSudutTarget(next);
  }
}

// ── MODE_ADC ──────────────────────────────────────────────────────────────
float emaADC = 2047.0f;
const unsigned long ADC_READ_MS = 50UL;
unsigned long tADC = 0;

void updateADC(unsigned long now) {
  if (now - tADC < ADC_READ_MS) return;
  tADC = now;
  int raw = analogRead(POT_PIN);
  emaADC = 0.15f * raw + 0.85f * emaADC;
  int target = map((long)emaADC, 0, 4095, 0, 180);
  if (abs(target - sudutTarget) > 1) {
    setSudutTarget(target);
    Serial.printf("[ADC] EMA: %6.1f → Target: %3d°\n", emaADC, target);
  }
}

// ── Transisi Mode ─────────────────────────────────────────────────────────
void switchMode(ServoMode newMode, unsigned long now) {
  currentMode = newMode;
  tSweep = now;
  tADC   = now;
  
  Serial.printf("\n[MODE] Berganti ke: %s\n", namaMode[newMode]);
  switch (newMode) {
    case MODE_PRESET:  
      Serial.println("  SHORT NEXT/PREV = Navigasi preset"); 
      presetIdx = 2; // Mulai dari tengah pada iterasi preset (90°)
      initPreset(); 
      break;
    case MODE_SWEEP:   
      Serial.println("  SHORT NEXT = Toggle arah sweep"); 
      break;
    case MODE_ADC:     
      Serial.println("  Putar potensiometer untuk kontrol posisi"); 
      break;
    case MODE_STANDBY: 
      setSudutTarget(90); // Paksa ke posisi netral
      Serial.println("  Servo terkunci di 90° netral"); 
      break;
    default: break;
  }
}

// ── Gerakan Smooth Global ─────────────────────────────────────────────────
void gerakSmooth(unsigned long now) {
  if (now - tSmooth < 10UL) return;
  tSmooth = now;
  if      (sudutSaat < sudutTarget) { sudutSaat++;  myServo.write(sudutSaat); }
  else if (sudutSaat > sudutTarget) { sudutSaat--;  myServo.write(sudutSaat); }
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(BTN_NEXT, INPUT);
  pinMode(BTN_PREV, INPUT);

  myServo.attach(SERVO_PIN, PULSA_MIN_US, PULSA_MAX_US);
  myServo.write(90);
  sudutSaat = sudutTarget = 90;
  delay(500);

  emaADC = (float)analogRead(POT_PIN);

  Serial.println("=== BAB 38 Program 4: FSM Multi-Mode Servo ===");
  Serial.println("TEKAN LAMA BTN_NEXT (>1 dtk) : Ganti mode");
  Serial.println("TEKAN PENDEK BTN_NEXT/PREV   : Aksi dalam mode");
  Serial.println("----------------------------------------------");
  switchMode(MODE_PRESET, 0);
}

// ── Loop ──────────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  updateBtn(btnN, now);
  updateBtn(btnP, now);

  // Long press BTN_NEXT → ganti mode
  if (btnN.longPress) {
    switchMode((ServoMode)((currentMode + 1) % MODE_COUNT), now);
  }

  // Eksekusi mode aktif
  switch (currentMode) {
    case MODE_PRESET:
      updatePreset(btnN.shortPress, btnP.shortPress);
      break;
    case MODE_SWEEP:
      updateSweep(now, btnN.shortPress);
      break;
    case MODE_ADC:
      updateADC(now);
      break;
    case MODE_STANDBY:
      setSudutTarget(90); // Paksa netral terus
      break;
  }

  gerakSmooth(now);
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 38 Program 4: FSM Multi-Mode Servo ===
TEKAN LAMA BTN_NEXT (>1 dtk) : Ganti mode
TEKAN PENDEK BTN_NEXT/PREV   : Aksi dalam mode
----------------------------------------------

[MODE] Berganti ke: PRESET   
  SHORT NEXT/PREV = Navigasi preset
[PRESET] Posisi: 90° (idx 2/4)
[PRESET] Posisi: 135° (idx 3/4)

[MODE] Berganti ke: SWEEP    
  SHORT NEXT = Toggle arah sweep
[SWEEP] Arah dibalik → Mundur ←

[MODE] Berganti ke: ADC POT  
  Putar potensiometer untuk kontrol posisi
[ADC] EMA: 3241.2 → Target: 142°
[ADC] EMA: 1812.7 → Target:  79°

[MODE] Berganti ke: STANDBY  
  Servo terkunci di 90° netral
```

---

## 38.8 Tips & Panduan Troubleshooting

### 1. Servo tidak bergerak sama sekali?
```text
Penyebab paling umum:

A. Library belum terpasang
   → Pastikan "ESP32Servo" sudah ter-install di Library Manager
   → Jangan gunakan library "Servo" standar Arduino (bukan untuk ESP32!)

B. Kabel daya salah
   → Pastikan kabel MERAH servo ke 5V (bukan 3.3V!)
   → Pastikan GND servo dan GND ESP32 terhubung ke titik yang SAMA

C. Pin Signal belum di-attach dengan benar
   → Periksa: myServo.attach(SERVO_PIN, 500, 2500);
   → Pastikan pin yang digunakan bukan IO34–IO39 (input-only!)

D. Brownout saat servo bergerak
   → ESP32 reset akibat tegangan turun saat servo menarik arus
   → Gunakan adaptor 5V 1A (bukan USB PC yang lemah)
   → Tambahkan kapasitor 100µF di jalur 5V servo
```

### 2. Servo bergetar terus / hunting?
```text
Penyebab: ADC noise atau servo tidak bisa mempertahankan posisi.

A. Noise ADC (jika menggunakan potensiometer)
   → Tambahkan atau tingkatkan nilai EMA_ALPHA menjadi lebih kecil (0.05)
   → Perbesar deadband: const int DEADBAND_DEG = 3;
   → Tambahkan kapasitor 100nF dari input ADC ke GND

B. Servo kualitas rendah
   → Servo murah memiliki dead-band internal yang buruk
   → Coba panggil myServo.write() hanya saat sudut berubah > 2°

C. Pulsa tidak tepat
   → Kalibrasi ulang: PULSA_MIN_US dan PULSA_MAX_US
   → Coba range 600–2400 atau 700–2300 untuk servo yang sempit range-nya
```

### 3. Servo hanya bergerak sebagian (tidak sampai 0° atau 180°)?
```text
Penyebab: Kalibrasi lebar pulsa tidak sesuai servo fisik.

Setiap servo memiliki range pulsa sedikit berbeda:

  SG90 (umum)  : 500µs – 2500µs
  MG995        : 600µs – 2400µs
  Servo mahal  : 1000µs – 2000µs (range lebih sempit, presisi tinggi)

PROSEDUR KALIBRASI:
  1. Panggil myServo.writeMicroseconds(500) → catat posisi fisik
  2. Naikkan bertahap: 600, 700, ... sampai servo mencapai batas mekanik
  3. Catat posisi 0° dan 180° mu → masukkan sebagai PULSA_MIN_US & MAX_US

⚠️ JANGAN lampaui batas mekanik servo!
   Terdengar suara gemeretak = servo memaksa melampaui batas → Gearbox rusak!
```

### 4. ESP32 reset/brownout saat servo diperintah bergerak?
```text
Ini masalah DAYA:

  Konsumsi arus:
    ESP32 aktif         : ~100–200 mA
    Servo bergerak      : ~100–200 mA
    Total               : ~300–400 mA
    USB laptop          : 500 mA max (sudah di batas!)

  Tanda-tanda brownout:
    - Serial Monitor menampilkan "Brownout detector was triggered"
    - ESP32 restart tiba-tiba saat servo diperintah ke posisi jauh

  Solusi bertingkat:
  1. Gunakan adaptor 5V 1A+ (bukan koneksi USB)
  2. Tambahkan kapasitor elektrolit 470µF–1000µF di jalur 5V servo
  3. Gerakkan servo lebih perlahan (naikkan STEP_MS / SMOOTH_MS)
  4. Hindari mode full sweep (0°→180°) tanpa jeda pada servo berbeban berat
```

### 5. Servo mengeluarkan bunyi 'dengung' saat diam?
```text
Penyebab: Frekuensi PWM atau lebar pulsa tidak tepat menyebabkan
          motor terus-menerus mencari posisi.

Solusi:
  A. Pastikan frekuensi 50Hz — ESP32Servo sudah menangani ini secara default.
     Jangan ubah frekuensi LEDC secara manual!

  B. Gunakan myServo.write() bertipe integer (bukan float)
     Nilai float akan dikonversi tidak tepat dan membuat jitter.

  C. Setelah servo mencapai posisi target dan tidak perlu bergerak lagi,
     kamu bisa melepas sinyal PWM dengan: myServo.detach()
     Ini menghentikan PWM sepenuhnya → servo "bebas" (tidak dengung),
     tapi tidak lagi aktif menahan posisi terhadap beban.
     GUNAKAN HANYA jika servo tidak menanggung beban!
```

---

## 38.9 Ringkasan

```
┌───────────────────────────────────────────────────────────────────────┐
│                 RINGKASAN BAB 38 — Servo Motor                        │
├───────────────────────────────────────────────────────────────────────┤
│ HARDWARE & PRINSIP KERJA:                                              │
│   Servo = Motor DC + Gearbox + Potensiometer + IC Kontrol PID        │
│   Kontrol posisi via lebar pulsa PWM pada frekuensi 50Hz (20ms)      │
│   SG90 tipikal: 500µs = 0°, 1500µs = 90°, 2500µs = 180°             │
│   IO25 = rekomendasi pin Signal servo di Bluino Kit                   │
│   Servo HARUS daya 5V — jangan pakai 3.3V!                           │
│                                                                        │
│ LIBRARY ESP32Servo:                                                    │
│   myServo.attach(pin, minUs, maxUs)  → Daftarkan servo               │
│   myServo.write(sudut)               → Atur posisi (0–180°)          │
│   myServo.writeMicroseconds(us)      → Kontrol presisi (µs)          │
│   myServo.detach()                   → Hentikan PWM (free-spin)      │
│   Jangan gunakan library "Servo" standar Arduino! Khusus ESP32Servo. │
│                                                                        │
│ POLA DESAIN (BEST PRACTICE):                                           │
│   ✅ Gerakan smooth via interpolasi 1°/langkah (non-blocking)        │
│   ✅ Filter EMA untuk ADC agar bebas noise dan hunting                │
│   ✅ Deadband: skip update jika perubahan sudut < threshold           │
│   ✅ FSM untuk multi-mode (Preset / Sweep / ADC / Standby)           │
│   ✅ Satu delay(500) di setup() untuk posisi awal — sisanya millis() │
│                                                                        │
│ BATAS KESELAMATAN:                                                     │
│   ▶ Jangan lampaui batas fisik mekanik 0°–180° → gearbox rusak!     │
│   ▶ Brownout: gunakan adaptor 5V 1A+, bukan USB laptop               │
│   ▶ Servo stall (tertahan) → arus 600-800mA → bahaya kebakaran!      │
│   ▶ Selalu panggil myServo.write(90) di setup() untuk posisi netral   │
└───────────────────────────────────────────────────────────────────────┘
```

---

## 38.10 Latihan

1. **Servo Speedometer:** Gunakan ADC potensiometer untuk mengontrol servo, namun tambahkan tampilan sudut OLED 128×64 (IO21/IO22). Tampilkan jarum speedometer bergaya ASCII: skala 0–180° dengan marker setiap 30°. Setiap putaran potensiometer =  jarum bergerak di layar OLED **sekaligus** servo fisik. (Petunjuk: integrasikan library `Adafruit SSD1306` dari BAB 35).

2. **Servo Alarm PIR:** Integrasikan PIR AM312 (IO33) dengan servo. Saat tidak ada gerakan, servo diam di 90°. Saat PIR mendeteksi gerakan, servo sweep dari 0° ke 180° dan kembali 3 kali sebagai "alarm visual" lalu kembali ke 90°. Buat FSM dengan state: `IDLE`, `ALERT_SWEEP`, `COOLDOWN` (jeda 10 detik setelah sweep selesai).

3. **Pan-Tilt Simulator:** Jika kamu memiliki 2 buah servo, bangun sistem Pan-Tilt (kamera gimbal). Potensiometer di IO34 mengontrol **Pan** (servo 1, IO25: kiri-kanan). Potensiometer di IO35 mengontrol **Tilt** (servo 2, IO26: atas-bawah). Kedua servo bergerak independent dan smooth sesuai posisi masing-masing potensiometer.

4. **Servo dengan LEDC Langsung (Tanpa Library):** Sebagai latihan pemahaman mendalam, implementasikan kontrol servo **tanpa** library ESP32Servo — gunakan LEDC langsung dengan perhitungan duty cycle manual. Frekuensi 50Hz, resolusi timer 16-bit. Rumus: `duty = (pulseWidth_µs / 20000µs) * 65535`. Bandingkan hasilnya dengan Program 1 menggunakan library.

5. **Servo Sequencer:** Buat program yang menyimpan urutan gerakan (sequence) di sebuah array `struct Move { int sudut; unsigned long tunda; }`. Saat tombol ditekan, servo mengeksekusi sequence tersebut secara otomatis — mirip seperti perekam gerakan robot sederhana. Coba dengan sequence: `0° → tunda 1 detik → 90° → tunda 500ms → 180° → tunda 1 detik → 45°`.

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 37: Relay — Kontrol AC/DC](../BAB-37/README.md)*

*Selanjutnya → [BAB 39: NVS — Non-Volatile Storage](../BAB-39/README.md)*
