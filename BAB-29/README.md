# BAB 29: MPU-6050 — Akselerometer, Giroskop & Filter Orientasi

> ✅ **Prasyarat:** Selesaikan **BAB 23 (I2C)** dan **BAB 28 (BMP180)**. MPU-6050 berbagi bus I2C yang sama dengan BMP180 dan OLED di Kit Bluino, sehingga pemahaman konsep *Shared Bus* dan alamat I2C sangat penting.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menjelaskan perbedaan fundamental antara **Akselerometer** (mengukur gaya/percepatan) dan **Giroskop** (mengukur kecepatan sudut putar).
- Membaca 6 sumbu data mentah (3-axis akselerasi + 3-axis rotasi) dari MPU-6050.
- Menghitung sudut kemiringan (**Roll** dan **Pitch**) menggunakan trigonometri akselerometer.
- Memahami kelemahan masing-masing sensor dan mengatasinya dengan **Complementary Filter**.
- Menerapkan arsitektur **Non-Blocking State Machine** untuk integrasi IoT profesional.

---

## 29.1 Mengenal MPU-6050 — Si Maestro Gerakan 6 Dimensi

**MPU-6050** buatan *InvenSense* (kini TDK) adalah sebuah chip revolusioner: ia memadatkan **dua sensor fisika berbeda** ke dalam satu modul sirkuit mikro setipis koin. Di industry, gabungan kedua sensor ini disebut **IMU (Inertial Measurement Unit)** dan dikategorikan sebagai **6-DOF** (enam derajat kebebasan).

### Dua Sensor Dalam Satu Chip

**🔵 Akselerometer 3-Sumbu** — Mengukur *Gaya Percepatan*
Membayangkan bagaimana akselerometer bekerja sangat mudah: bayangkan sebuah bola besi kecil yang mengambang di dalam sebuah kubus kosong. Ketika kubus dimiringkan atau diguncang, bola itu akan "menekan" dinding kubus sesuai arah gaya yang dialaminya. Sensor akselerometer mengukur seberapa besar tekanan bola itu pada masing-masing tiga pasang dinding (sumbu X, Y, Z).

Saat sensor diam di atas meja, bola itu menekan lantai kubus ke bawah dengan nilai `~9.81 m/s²` — itulah **gravitasi bumi**! Inilah kenapa akselerometer bisa mengukur kemiringan.

**🔴 Giroskop 3-Sumbu** — Mengukur *Kecepatan Sudut Putar*
Giroskop mengukur seberapa cepat sesuatu **berputar**, bukan seberapa miring. Bayangkan giroscope seperti speedometer putaran — nilainya dalam satuan °/s (derajat per detik). Jika giroskop membaca nilai `90 °/s` pada sumbu X selama `1 detik`, artinya sensor sudah berputar `90°` pada sumbu tersebut.

```
         Sumbu Koordinat MPU-6050
         
            Z ↑   Y
            │    ↗
            │   /
            │  /
            │ /
  ──────────┼──────────→ X
            │
         (datar di atas meja)
         
  Akselerometer membaca: ax=0, ay=0, az=+9.81 m/s²
  (Hanya gravitasi bumi ke bawah yang terdeteksi!)
```

### Spesifikasi Hardware Kit Bluino

```
┌───────────────────────────────────────────────────────────────┐
│                 SPESIFIKASI MPU-6050 — KIT BLUINO             │
├─────────────────────────────┬─────────────────────────────────┤
│ Parameter                   │ Nilai                           │
├─────────────────────────────┼─────────────────────────────────┤
│ Protokol Komunikasi         │ I2C (Shared Bus IO21/IO22)      │
│ Alamat I2C Default          │ 0x68 (pin AD0 → GND)            │
│ Alamat I2C Alternatif       │ 0x69 (pin AD0 → VCC)            │
│ Tegangan Operasi            │ 2.375V – 3.46V                  │
│ Rentang Akselerometer       │ ±2g / ±4g / ±8g / ±16g         │
│ Sensitivitas ±2g (Default)  │ 16,384 LSB/g                    │
│ Rentang Giroskop            │ ±250 / ±500 / ±1000 / ±2000 °/s│
│ Sensitivitas ±250°/s (Def.) │ 131 LSB/(°/s)                   │
│ Sensor Suhu Internal        │ Ada (digunakan sbg kompensasi)  │
│ DMP (Motion Processor)      │ Ada (untuk quaternion)          │
└─────────────────────────────┴─────────────────────────────────┘
```

> ✅ **Alamat I2C Konfirmed:** Di Kit Bluino, pin `AD0` MPU-6050 dihubungkan ke GND, sehingga **alamat I2C-nya adalah `0x68`**. Ini tidak konflik dengan BMP180 (`0x77`) maupun OLED (`0x3C`).

---

## 29.2 Fisika & Matematika Sudut — Dari Gravitasi ke Derajat

### A. Mengapa Akselerometer Bisa Menghitung Sudut?

Ketika sensor dimiringkan, vektor gravitasi `g` yang semula vertikal ke bawah mulai "terbagi" menjadi komponen pada setiap sumbu. Dengan geometri trigonometri sederhana, kita bisa menghitung sudut kemiringan:

```
Roll (Putar ke kiri/kanan):
  Roll = atan2(ay, az) × (180/π)

Pitch (Mengangguk maju/mundur):
  Pitch = atan2(-ax, √(ay² + az²)) × (180/π)

Di mana:
  ax, ay, az = Akselerasi pada sumbu X, Y, Z (dalam m/s²)
  atan2      = "Arc Tangent 2" — versi atan yang memperhitungkan kuadran!
  (180/π)    = Konversi dari Radian ke Derajat (~57.296)
```

> ⚠️ **Yaw tidak bisa dihitung dari akselerometer saja!** Yaw adalah rotasi horizontal (seperti kompas berputar). Karena gravitasi selalu vertikal, akselerometer tidak dapat mendeteksi rotasi horizontal — kita butuh **magnetometer** (kompas) atau GPS untuk Yaw!

### B. Masalah Akselerometer Murni: Guncangan!

Kelemahannya fatal: saat sensor *bergerak cepat* (guncangan, benturan), akselerometer tidak bisa membedakan mana gravitasi mana percepatan gerak. Hasilnya? Sudut yang kita hitung akan *bergetar parah* seperti Richter 7.

### C. Masalah Giroskop Murni: Drift!

Sebaliknya, giroskop unggul di gerakan cepat. Tapi karena kita mendapat **kecepatan sudut** (°/s) dan perlu melakukan **integrasi waktu** untuk mendapat sudut (°), penghitungannya terlihat seperti ini:
```cpp
sudut_baru = sudut_lama + (kecepatan_sudut × dt)
```
Masalahnya: sensor giroskop tidak pernah sempurna (**drift** - ada offset kecil tidak nol bahkan saat diam). Jika kita mengintegrasikan offset kecil ini terus-menerus, sudut yang dihasilkan akan *melayang* perlahan-lahan dan menjadi tidak akurat setelah beberapa menit!

### D. Solusi: Complementary Filter

Filter sederhana nan brilian yang menggabungkan kelebihan keduanya:

```
sudut_filter = α × (sudut_filter_lama + giroskop × dt) + (1-α) × sudut_akselerometer

Di mana:
  α = 0.98 (koefisien kepercayaan giroskop — 98% percaya giroskop saat ini)
  (1-α) = 0.02 (koreksi halus dari akselerometer — 2% koreksi drift jangka panjang)
```
- **98% giroskop**: respons cepat dan halus saat bergerak
- **2% akselerometer**: mencegah drift akumulatif secara bertahap
- **Hasilnya**: Stabil saat diam, responsif saat bergerak!

---

## 29.3 Instalasi Library

```
Library Manager (Ctrl+Shift+I) → Ketik "MPU6050"

1. Instal: "Adafruit MPU6050" by Adafruit Industries
2. Saat ditanya dependensi, pilih "Install All"
   (Ini wajib — library ini menggunakan Adafruit Unified Sensor HAL!)
```

> 💡 Berbeda dengan BMP180 yang *Native Driver*, library `Adafruit MPU6050` membutuhkan `Adafruit_Sensor` sebagai lapisan abstraksi. Ini desain yang sengaja dibuat Adafruit agar kode sensor mudah diportasi ke berbagai platform (Arduino UNO, ESP32, Raspberry Pi Pico).

---

## 29.4 Program 1: Membaca 6 Sumbu Data Mentah (Raw Data)

Program dasar untuk membuktikan sensor bekerja. Kamu akan melihat langsung bagaimana semua 6 sumbu bergerak saat papan dipegang dan dimiringkan.

```cpp
/*
 * BAB 29 - Program 1: MPU-6050 Raw Data — 6 Sumbu Sekaligus
 *
 * Membaca akselerasi (m/s²) dan kecepatan sudut (rad/s)
 * dari IMU 6-DOF MPU-6050 via I2C Shared Bus Kit Bluino.
 *
 * Wiring (Topologi Kit Bluino — Sudah Built-in!):
 *   MPU-6050 SDA → IO21 (Shared I2C Bus)
 *   MPU-6050 SCL → IO22 (Shared I2C Bus)
 *   MPU-6050 AD0 → GND  (Alamat I2C: 0x68)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════════");
  Serial.println("   BAB 29: MPU-6050 — 6-DOF Raw Data");
  Serial.println("══════════════════════════════════════════════");

  if (!mpu.begin()) {
    Serial.println("❌ GAGAL: MPU-6050 tidak ditemukan di 0x68!");
    Serial.println("   Kemungkinan penyebab:");
    Serial.println("   → Kabel SDA/SCL solder buruk di PCB kit");
    Serial.println("   → Konflik bus I2C (cek BMP180 0x77 & OLED 0x3C)");
    Serial.println("   → Chip MPU-6050 rusak atau tegangan tidak stabil");
    while (true) delay(10);
  }

  // ── Konfigurasi Sensitivitas Sensor ───────────────────────────
  // Pilih rentang akselerometer: 2G (presisi) atau 8G/16G (gerakan keras)
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);

  // Pilih rentang giroskop: 250 (presisi halus) atau 500/1000/2000 (gerakan cepat)
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);

  // Filter Low-Pass Hardware Internal (untuk meredam getaran frekuensi tinggi)
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("✅ MPU-6050 siap — Konfigurasi: ±2g | ±250°/s | LPF 21Hz");
  Serial.println("──────────────────────────────────────────────");
}

void loop() {
  // Ambil data event dari sensor (format Adafruit Unified Sensor)
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // ── Aksenterasi (satuan: m/s²) ─────────────────────────────────
  // Catatan: BUKAN dalam satuan 'g'! Harus dibagi 9.81 untuk mendapat 'g'.
  // Saat datar di atas meja: ax≈0, ay≈0, az≈+9.81
  Serial.printf("📐 ACCEL (m/s²) | X: %7.3f | Y: %7.3f | Z: %7.3f\n",
                accel.acceleration.x,
                accel.acceleration.y,
                accel.acceleration.z);

  // ── Kecepatan Sudut Rotasi (satuan: rad/s) ─────────────────────
  // Catatan: BUKAN dalam °/s! Harus dikali (180/π) untuk mendapat °/s.
  // Saat diam: gx≈0, gy≈0, gz≈0 (idealnya, tapi ada offset kecil = drift)
  Serial.printf("🌀 GYRO  (rad/s) | X: %7.4f | Y: %7.4f | Z: %7.4f\n",
                gyro.gyro.x,
                gyro.gyro.y,
                gyro.gyro.z);

  // ── Suhu Internal Chip (kompensasi ADC) ────────────────────────
  Serial.printf("🌡️  TEMP  (°C)    | Internal: %5.1f °C\n", temp.temperature);
  Serial.println("──────────────────────────────────────────────");

  delay(200); // 5 Hz — cukup untuk observasi visual
}
```

**Contoh output saat sensor datar di atas meja:**
```text
══════════════════════════════════════════════
   BAB 29: MPU-6050 — 6-DOF Raw Data
══════════════════════════════════════════════
✅ MPU-6050 siap — Konfigurasi: ±2g | ±250°/s | LPF 21Hz
──────────────────────────────────────────────
📐 ACCEL (m/s²) | X:   0.041 | Y:  -0.092 | Z:   9.782
🌀 GYRO  (rad/s) | X:  0.0023 | Y: -0.0011 | Z:  0.0007
🌡️  TEMP  (°C)    | Internal:  28.3 °C
──────────────────────────────────────────────
```

**Interpretasi data:**
1. `az ≈ 9.78 m/s²` → Gravitasi bumi ke bawah terdeteksi normal ✅
2. `ax, ay ≈ ~0` → Sensor datar, tidak ada komponen gravitasi horizontal ✅
3. `gx, gy, gz ≈ ~0.001` → Sensor diam, ada sedikit *noise* dan *offset* bawaan ✅
4. Suhu internal chip **bukan** suhu udara — biasanya 2–5°C lebih tinggi dari suhu ambient

---

## 29.5 Program 2: Kalkulator Sudut Kemiringan (Akselerometer)

Mengubah data akselerasi mentah menjadi angka sudut Roll dan Pitch yang bisa dipahami manusia.

```cpp
/*
 * BAB 29 - Program 2: Sudut Kemiringan dari Akselerometer
 *
 * Menghitung Roll dan Pitch menggunakan rumus trigonometri
 * atan2 dari akselerasi gravitasi yang terproyeksi di 3 sumbu.
 *
 * ⚠️ Program ini BAGUS untuk posisi statis, tapi akan GEMETAR
 *    saat ada guncangan! Lihat Program 3 untuk solusinya.
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

Adafruit_MPU6050 mpu;

// Konversi radian ke derajat
const float RAD_TO_DEG = 180.0f / M_PI;

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println(" BAB 29: Kalkulator Sudut Akselerometer");
  Serial.println("══════════════════════════════════════════");

  if (!mpu.begin()) {
    Serial.println("❌ MPU-6050 tidak ditemukan!");
    while (true) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("✅ Siap! Miringkan sensor untuk melihat sudut berubah.");
  Serial.println("   Roll  = Miring kiri/kanan  (Axis X)");
  Serial.println("   Pitch = Mengangguk depan/belakang (Axis Y)");
  Serial.println("──────────────────────────────────────────");
}

void loop() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  // ── Kalkulasi Sudut via Persamaan Trigonometri ─────────────────
  // Roll: rotasi di sekitar sumbu X (miring ke kiri/kanan)
  float roll  = atan2(ay, az) * RAD_TO_DEG;

  // Pitch: rotasi di sekitar sumbu Y (mengangguk depan/belakang)
  // 💡 KEUNGGULAN MATEMATIKA (Kekebalan Lintas-Sumbu / Cross-Axis Immunity):
  // Banyak tutorial bodong di internet hanya memakai rumus `atan2(-ax, az)`. 
  // Jika kamu memakai rumus tutorial itu, saat sensor terguling (Roll), nilai Pitch-nya 
  // akan hancur dan ikut terpelanting! Dengan menggunakan Phytagoras `sqrt(ay² + az²)`, 
  // kita menjamin kalkulasi Pitch tetap absolut akurat biarpun sensor sambil salto/berguling!
  float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;

  // ── Tampilkan hasil ────────────────────────────────────────────
  Serial.printf("Roll  : %7.2f °  |  Pitch : %7.2f °\n", roll, pitch);

  delay(100); // 10 Hz
}
```

**Contoh output saat papan dimiringkan 45° ke kanan:**
```text
Roll  :   45.31 °  |  Pitch :    0.52 °
Roll  :   45.28 °  |  Pitch :    0.48 °
Roll  :   45.33 °  |  Pitch :    0.55 °
```

> ⚠️ **Uji Guncangan:** Sekarang cobalah ketuk meja di dekat sensor. Nilai Roll dan Pitch akan *melonjak* dan *gemetar* saat ada guncangan — karena akselerometer tidak bisa membedakan gravitasi dengan goncangan fisik! Inilah alasan kita butuh Program 3.

---

## 29.6 Program 3: Complementary Filter — Stabilitas Drone & Robot

Menggabungkan kecepatan dan stabilitas kedua sensor (akselerometer + giroskop) menggunakan algoritma filter sederhana namun sangat efektif.

```cpp
/*
 * BAB 29 - Program 3: Complementary Filter
 *
 * Menggabungkan akselerometer (stabil jangka panjang, rentan guncangan)
 * dengan giroskop (responsif, rentan drift) menggunakan filter α.
 *
 * Hasil: Sudut Roll & Pitch yang STABIL saat diam DAN RESPONSIF saat bergerak!
 *
 * Implementasi ini dipakai di:
 *   - Quadcopter (kendali kemiringan baling-baling)
 *   - Self-balancing robot
 *   - Game controller (deteksi miring)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

Adafruit_MPU6050 mpu;

// ── Koefisien Complementary Filter ───────────────────────────────
// α mendekati 1.0 = lebih percaya giroskop (responsif, rentan drift)
// α mendekati 0.0 = lebih percaya akselerometer (stabil, rentan guncangan)
// Nilai 0.98 adalah standar industri yang terbukti optimal:
const float ALPHA = 0.98f;

// ── Variabel State Filter ─────────────────────────────────────────
float rollFilter  = 0.0f;
float pitchFilter = 0.0f;

// ── Timer untuk kalkulasi dt (delta waktu) ────────────────────────
unsigned long tSebelumnya = 0;

const float RAD_TO_DEG = 180.0f / M_PI;

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════════════");
  Serial.println(" BAB 29: Complementary Filter — Stabilitas Sensor");
  Serial.println("══════════════════════════════════════════════════");
  Serial.printf( " Koefisien Filter (α): %.2f\n", ALPHA);
  Serial.println("══════════════════════════════════════════════════");

  if (!mpu.begin()) {
    Serial.println("❌ MPU-6050 tidak ditemukan!");
    while (true) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // 💡 FILTER SEEDING (Injeksi Kondisi Awal):
  // Jika ESP32 dinyalakan di permukaan menanjak (misal miring 45°), dan filter
  // kita berawal dari 0°, CF akan butuh 3-4 detik merayap lamban mencapai 45°!
  // Bagi Drone, lag start-up ini berakibat fatal. Solusi instan: Kita suntik
  // pembacaan akselerometer absolut pertama (raw) langsung ke jantung filter!
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  rollFilter  = atan2(a.acceleration.y, a.acceleration.z) * RAD_TO_DEG;
  pitchFilter = atan2(-a.acceleration.x, sqrt(a.acceleration.y*a.acceleration.y + a.acceleration.z*a.acceleration.z)) * RAD_TO_DEG;

  Serial.println("✅ Filter aktif dan telah di-seed! Bandingkan dengan Program 2 saat diguncang.\n");

  tSebelumnya = millis();
}

void loop() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // ── Hitung Delta Waktu (dt) ────────────────────────────────────
  // PALING KRITIS: dt harus dihitung secara aktual per iterasi loop
  // agar integrasi giroskop akurat! Jangan pakai angka hardcode!
  unsigned long sekarang = millis();
  float dt = (sekarang - tSebelumnya) / 1000.0f; // Konversi ms → detik
  tSebelumnya = sekarang;

  // Proteksi: jika dt tidak wajar (boot pertama / lag besar), skip siklus ini
  if (dt <= 0.0f || dt > 0.5f) return;

  // ── Sudut dari Akselerometer (referensi stabil jangka panjang) ─
  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  float rollAcc  = atan2(ay, az) * RAD_TO_DEG;
  float pitchAcc = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;

  // ── Kecepatan Putar dari Giroskop (rad/s → °/s) ────────────────
  // Library Adafruit mengembalikan satuan rad/s, bukan °/s!
  float gx_dps = gyro.gyro.x * RAD_TO_DEG; // Kecepatan Roll dalam °/s
  float gy_dps = gyro.gyro.y * RAD_TO_DEG; // Kecepatan Pitch dalam °/s

  // ── Complementary Filter ───────────────────────────────────────
  // Bagian 1: ALPHA * (sudut + kecepatan_gyro * dt)
  //           = Integrasi giroskop dari posisi terakhir (98% kepercayaan)
  // Bagian 2: (1 - ALPHA) * sudut_accel
  //           = Koreksi drift halus dari akselerometer (2% koreksi)
  rollFilter  = ALPHA * (rollFilter  + gx_dps * dt) + (1.0f - ALPHA) * rollAcc;
  pitchFilter = ALPHA * (pitchFilter + gy_dps * dt) + (1.0f - ALPHA) * pitchAcc;

  // ── Tampilkan Perbandingan ─────────────────────────────────────
  Serial.printf("Accel-Only │ Roll: %7.2f° | Pitch: %7.2f°\n",
                rollAcc, pitchAcc);
  Serial.printf("CF-Filter  │ Roll: %7.2f° | Pitch: %7.2f°  (dt:%.1fms)\n",
                rollFilter, pitchFilter, dt * 1000.0f);
  Serial.println("───────────────────────────────────────────────────");

  delay(20); // 50 Hz — kecepatan update yang baik untuk filter ini
}
```

**Contoh output perbandingan — saat sensor diguncang:**
```text
Accel-Only │ Roll:   45.28° | Pitch:    0.52°
CF-Filter  │ Roll:   45.31° | Pitch:    0.50°  (dt:21.2ms)
───────────────────────────────────────────────────
Accel-Only │ Roll:   78.12° | Pitch:  -23.41°  ← PANIK saat guncangan!
CF-Filter  │ Roll:   45.58° | Pitch:    0.61°  ← Stabil seperti batu!
───────────────────────────────────────────────────
Accel-Only │ Roll:   45.19° | Pitch:    0.48°
CF-Filter  │ Roll:   45.41° | Pitch:    0.53°
───────────────────────────────────────────────────
```

> 💡 **Analisis Perbandingan:** Perhatikan di baris kedua — saat ada guncangan, `Accel-Only` melompat ke angka absurd `78°` dan `-23°`, sedangkan `CF-Filter` hanya bergeser minimal ke `45.58°`. Itulah kekuatan Complementary Filter!

---

## 29.7 Program 4: Non-Blocking State Machine (IMU Profesional)

Arsitektur terbaik untuk integrasi MPU-6050 ke dalam sistem IoT yang kompleks (bersamaan dengan WiFi, OLED, dll):

```cpp
/*
 * BAB 29 - Program 4: MPU-6050 Non-Blocking IMU State Machine
 *
 * Menyimpan data orientasi ke struct global yang bisa dibaca kapan saja
 * oleh modul lain (OLED, Web Server, MQTT publisher).
 *
 * Arsitektur: Complementary Filter + Latch-on-Fail + I2C Ping Guard
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

Adafruit_MPU6050 mpu;

// ── Konfigurasi ──────────────────────────────────────────────────
const unsigned long INTERVAL_IMU_MS = 20UL;   // 50 Hz sampling rate
const float         ALPHA           = 0.98f;
const float         RAD_TO_DEG      = 180.0f / M_PI;

// ── Data Register IMU (Global — Bisa diakses dari mana saja) ─────
struct DataIMU {
  float rollDeg;
  float pitchDeg;
  float gyroDpsX;
  float gyroDpsY;
  float gyroDpsZ;
  float suhuInternal;
  bool  sukses = false;
};

DataIMU imuTerakhir;

// ── State Filter (Global) ─────────────────────────────────────────
float         rollFilter      = 0.0f;
float         pitchFilter     = 0.0f;
unsigned long tBacaTerakhir   = 0;
unsigned long tSebelumnya     = 0;

// ── Fungsi Pembacaan IMU (Non-Blocking) ───────────────────────────
void perbaruiIMU() {
  unsigned long sekarang = millis();
  if (sekarang - tBacaTerakhir < INTERVAL_IMU_MS) return;
  tBacaTerakhir = sekarang;

  // I2C Ping: Pastikan sensor ada sebelum memaksa library bekerja
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() != 0) {
    imuTerakhir.sukses = false;
    Serial.printf("[%6lu ms] ❌ MPU-6050 putus dari bus I2C!\n", sekarang);
    return;
  }

  // Hitung dt aktual untuk integrasi giroskop yang akurat
  float dt = (sekarang - tSebelumnya) / 1000.0f;
  tSebelumnya = sekarang;

  // 💡 SELF-HEALING FILTER (Anti-Ledakan dt):
  // Jika ESP32 nge-lag parah (misal butuh 1 detik untuk konek WiFi), nilai dt 
  // akan jadi raksasa (1.0s). Jika dt raksasa ini dikalikan giroskop, filter akan
  // "meledak" karena terlempar ke sudut konyol. Jika dt > 0.5s, kita SKIP perhitungan!
  // Filter akan membeku sesaat, namun di detik berikutnya ia akan "menyembuhkan 
  // dirinya sendiri" ditarik kembali oleh gravitasi Akselerometer!
  if (dt <= 0.0f || dt > 0.5f) return; 

  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  // 💡 G-SHOCK ADAPTIVE FILTER (Rahasia Navigasi Rudal / Drone):
  // Nilai gravitasi murni akan membuat panjang vektor ini mendekati ~9.81 m/s².
  // Jika totalG > 15 m/s², artinya sensor sedang DIPUKUL/TERHEMPAS! Akselerometer
  // bodoh akan menyangka "pukulan" itu sebagai lantai/gravitasi baru. Terkutuklah!
  // Bagaimana mengatasinya? JANGAN menghentikan siklus perhitungan! 
  // Ingat sifat fisika: Giroskop (Gyro) murni KEBAL terhadap pukulan/shock linier!
  // Jadi, saat sensor dipukul, kita bungkam akselerometer secara ekstrem (Alpha = 1.0)
  // dan 100% MURNI percaya pada tracking Giroskop hingga guncangan mereda!
  float totalG = sqrt(ax*ax + ay*ay + az*az);
  float alphaDinamis = ALPHA;
  
  if (totalG < 5.0f || totalG > 15.0f) {
    alphaDinamis = 1.0f; // Mode "Blind terhadap Akselerometer"
    Serial.printf("[%6lu ms] ⚠️  Shock! (|g|=%.2f m/s²) -> Pindah ke Gyro Murni!\n",
                  sekarang, totalG);
  }

  float rollAcc  = atan2(ay, az) * RAD_TO_DEG;
  float pitchAcc = atan2(-ax, sqrt(ay*ay + az*az)) * RAD_TO_DEG;

  float gx_dps = gyro.gyro.x * RAD_TO_DEG;
  float gy_dps = gyro.gyro.y * RAD_TO_DEG;

  // Terapkan Adaptive Complementary Filter (Sistem Anti-Banting Sesungguhnya)
  rollFilter  = alphaDinamis * (rollFilter  + gx_dps * dt) + (1.0f - alphaDinamis) * rollAcc;
  pitchFilter = alphaDinamis * (pitchFilter + gy_dps * dt) + (1.0f - alphaDinamis) * pitchAcc;

  // Simpan ke Data Register Global
  imuTerakhir.rollDeg       = rollFilter;
  imuTerakhir.pitchDeg      = pitchFilter;
  imuTerakhir.gyroDpsX      = gx_dps;
  imuTerakhir.gyroDpsY      = gy_dps;
  imuTerakhir.gyroDpsZ      = gyro.gyro.z * RAD_TO_DEG;
  imuTerakhir.suhuInternal  = temp.temperature;
  imuTerakhir.sukses        = true; // Selalu sukses mencatat rotasi, bahkan pas diguncang!

  Serial.printf("[%6lu ms] Roll:%7.2f° | Pitch:%7.2f° | Suhu:%.1f°C\n",
                sekarang, rollFilter, pitchFilter, temp.temperature);
}

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════════════");
  Serial.println(" BAB 29: MPU-6050 Non-Blocking IMU State Machine");
  Serial.println("══════════════════════════════════════════════════");

  if (!mpu.begin()) {
    Serial.println("❌ MPU-6050 tidak ditemukan di 0x68!");
    while (true) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Filter Seeding (Meniadakan lag kalkulasi awal)
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  rollFilter  = atan2(a.acceleration.y, a.acceleration.z) * RAD_TO_DEG;
  pitchFilter = atan2(-a.acceleration.x, sqrt(a.acceleration.y*a.acceleration.y + a.acceleration.z*a.acceleration.z)) * RAD_TO_DEG;

  tSebelumnya   = millis();
  tBacaTerakhir = millis();

  Serial.println("✅ IMU siap! Sampling 50Hz dengan Complementary Filter.\n");
}

void loop() {
  // IMU berjalan di latar belakang
  perbaruiIMU();

  // Logika lain bebas berjalan di sini tanpa gangguan!
  // Contoh: if (imuTerakhir.sukses && imuTerakhir.rollDeg > 30.0f) ...
  //         ... aktifkan alarm tilt!
}
```

**Contoh output yang diharapkan:**
```text
══════════════════════════════════════════════════
 BAB 29: MPU-6050 Non-Blocking IMU State Machine
══════════════════════════════════════════════════
✅ IMU siap! Sampling 50Hz dengan Complementary Filter.

[    21 ms] Roll:   0.15° | Pitch:  -0.22° | Suhu:28.3°C
[    41 ms] Roll:   0.17° | Pitch:  -0.20° | Suhu:28.3°C
[    61 ms] Roll:  12.54° | Pitch:  -0.21° | Suhu:28.4°C
[    81 ms] Roll:  28.91° | Pitch:  -0.19° | Suhu:28.4°C
```

---

## 29.8 Tips & Panduan Troubleshooting

### 1. `mpu.begin()` Mengembalikan `false`
```text
Langkah diagnosis berurutan:

A. Scan bus I2C untuk konfirmasi alamat:
     Wire.begin(21, 22);
     Wire.beginTransmission(0x68);
     byte err = Wire.endTransmission();
     Serial.println(err == 0 ? "0x68 OK" : "0x68 GAGAL");

B. Periksa apakah BMP180 (0x77) juga tidak muncul — jika iya,
   seluruh bus I2C bermasalah (power, kabel SDA/SCL, dll.)

C. Periksa tegangan: MPU-6050 HARUS mendapat 3.3V, bukan 5V!
   Tegangan 5V akan merusak chip secara permanen.
```

### 2. Nilai Roll/Pitch Selalu 0 atau Tidak Berubah?
```text
Penyebab paling umum: Library tidak terinisialisasi dengan benar.

Pastikan urutan ini dalam setup():
  mpu.begin();                                    // 1. Inisialisasi dulu
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);   // 2. Baru konfigurasi
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);        // 3. Ini TIDAK bisa
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);     // 4. sebelum begin()!
```

### 3. Nilai Roll/Pitch Gemetar Parah Meski Sensor Diam?
```text
Ini bukan bug — itu karakteristik normal sensor MEMS murah!
Penyebab: Getaran mekanis dari ventilasi, AC, atau meja bergetar.

Solusi bertingkat:
  1. Aktifkan hardware Low-Pass Filter: mpu.setFilterBandwidth(MPU6050_BAND_5_HZ)
     (Semakin rendah Hz-nya, semakin halus tapi semakin lambat respons)
  2. Gunakan Complementary Filter (Program 3) yang memiliki low-pass inherent
  3. Tambahkan software averaging: rata-rata 5 sampel sebelum ditampilkan
```

### 4. Sudut Pitch Hanya Akurat ±90°, Bukan 360°?
```text
Ini BATAS MATEMATIKA dari atan2 — bukan bug!

Penjelasan: Persamaan trigonometri akselerometer hanya bisa membedakan
sudut 0°–90° dan 90°–180° saat sensor dibalik terbalik (di atas kepala).
Informasi "mana atas mana bawah" hilang saat sensor melewati 90°.

Untuk sudut penuh 360°: Butuh fusi sensor dengan magnetometer (kompas).
MPU-9250 memiliki magnetometer internal, sedangkan MPU-6050 tidak.
```

### 5. Gimbal Lock — Saat Pitch = 90°, Roll Menjadi Galau?
```text
Fenomena ini disebut "Gimbal Lock" — Kehilangan satu derajat kebebasan
saat sensor tegak lurus (pitch = ±90°). Ini bug matematis bawaan semua
sistem Euler Angle (Roll/Pitch/Yaw).

Solusi untuk sistem kritis (Drone, VR): Gunakan representasi QUATERNION
alih-alih Euler Angle. MPU-6050 memiliki DMP (Digital Motion Processor)
internal yang dapat menghitung Quaternion langsung di chipnya. Untuk ini,
gunakan library "MPU6050" by Electronic Cats yang mengakses DMP.
```

### 6. Diam Datar di Meja, Tapi Roll Kok Terus Menunjuk Angka 2°?
```text
Ini adalah musuh bebuyutan navigasi modern: Gyro Zero-Rate Offset (Static Drift).
Chip MEMS kelas amatir (seperti di Kit ini) jarang terkalibrasi sempurna dari
pabrik. Saat diam, Giroskop sumbu X-nya mungkin membisikkan bahwa ia sedang
berputar 1.5°/detik! Kebisingan konstan ini merongrong filter (meski sudah ditarik
oleh akselerometer), sehingga menghasilkan kemiringan absolut statis sebesar 2°.

Solusi Masterclass (Kalibrasi Firmware):
  Dalam firmware drone sungguhan, 2 detik pertama saat setup() adalah momen suci.
  Drone tidak boleh disentuh. Selama 2 detik itu, ESP32 membaca 500 sampel Giroskop,
  merata-ratakannya, lalu menjadikannya sebagai angka PENGURANG (Offset Bias) pada 
  setiap hitungan di fungsi loop() selamanya.
```

---

## 29.9 Ringkasan

```
┌──────────────────────────────────────────────────────────────────────┐
│               RINGKASAN BAB 29 — ESP32 + MPU-6050                    │
├──────────────────────────────────────────────────────────────────────┤
│ Protokol    = I2C pada IO21 (SDA) / IO22 (SCL)                       │
│ Alamat I2C  = 0x68 (AD0=GND, default Kit Bluino)                     │
│ Library     = "Adafruit MPU6050" (butuh Adafruit_Sensor dependency!) │
│                                                                      │
│ Setup Wajib (URUT!):                                                 │
│   mpu.begin();                                                       │
│   mpu.setAccelerometerRange(MPU6050_RANGE_2_G);                      │
│   mpu.setGyroRange(MPU6050_RANGE_250_DEG);                           │
│   mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);                        │
│                                                                      │
│ Membaca Data:                                                        │
│   sensors_event_t accel, gyro, temp;                                 │
│   mpu.getEvent(&accel, &gyro, &temp);                                │
│   // accel.acceleration.x/y/z  →  SATUAN m/s² (bukan 'g'!)          │
│   // gyro.gyro.x/y/z           →  SATUAN rad/s (bukan °/s!)         │
│                                                                      │
│ KONVERSI SATUAN (WAJIB DIINGAT!):                                    │
│   m/s² → g       : bagi 9.81                                        │
│   rad/s → °/s    : kalikan (180/π)                                   │
│                                                                      │
│ Rumus Sudut dari Akselerometer:                                      │
│   Roll  = atan2(ay, az)                 × (180/π)                    │
│   Pitch = atan2(-ax, sqrt(ay²+az²))    × (180/π)                    │
│                                                                      │
│ Complementary Filter (α = 0.98):                                     │
│   sudut = α × (sudut_lama + gyro_dps × dt) + (1-α) × sudut_accel    │
│                                                                      │
│ Validasi Data Sehat:                                                 │
│   totalG = sqrt(ax²+ay²+az²) → harus mendekati 9.81 m/s²            │
│   Jika < 5 atau > 15 → hardware bermasalah!                         │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 29.10 Latihan

1. **Detektor Tilt Alarm:** Buat program yang menyalakan buzzer jika roll atau pitch melebihi 30 derajat. Tambahkan hysteresis: buzzer baru akan mati kembali setelah sudut kembali di bawah 20 derajat (bukan 30). Hysteresis mencegah bunyi nyaring on/off terus-menerus saat sensor ada di zona perbatasan 30°.

2. **Visualisasi Serial Plotter:** Modifikasi Program 3 agar hanya mengeluarkan 3 angka berpisah koma setiap baris:
   ```
   RollAccel,RollFilter,PitchFilter
   45.28,45.31,0.50
   ```
   Buka **Serial Plotter** di Arduino IDE (Ctrl+Shift+L) dan amati grafik ketiga sensor sedang bergerak bersamaan. Perhatikan perbedaan Accel-Only vs Filter!

3. **IMU + BMP180 + OLED (Full Weather & Orientation Station):** Gabungkan BAB 28 (BMP180), BAB 29 (MPU-6050), dan BAB 23 (OLED) di satu program menggunakan Non-Blocking Architecture. Tampilkan di OLED 128×64: baris 1 `P:1013hPa Alt:762m`, baris 2 `Roll:12.3° Pitch:1.1°`. Gunakan pola state machine dengan timer terpisah untuk setiap sensor.

4. **Self-Leveling Indicator:** Buat antarmuka "gelembung udara digital" via Serial. Cetak karakter ASCII berupa titik `•` yang bergerak secara horizontal dalam area lebar 40 karakter tergantung nilai roll sensor:
   - Roll = 0°: titik di tengah `────────────•────────────`
   - Roll = 30°: titik bergeser `───────────────────•─────`
   - Roll = -30°: titik bergeser `─────•───────────────────`

---

*Selanjutnya → [BAB 30: HC-SR04 — Sensor Jarak Ultrasonik](../BAB-30/README.md)*
