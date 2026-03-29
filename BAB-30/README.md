# BAB 30: HC-SR04 — Sensor Jarak Ultrasonik & Deteksi Objek

> ✅ **Prasyarat:** Selesaikan **BAB 10 (GPIO Digital)** dan **BAB 15 (Interupsi & Timer)**. HC-SR04 di Kit Bluino menggunakan pin **Custom** yang harus dihubungkan dengan kabel jumper secara manual — ini adalah latihan *wiring* dasar yang disengaja oleh desainer kit!

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menjelaskan prinsip kerja sonar ultrasonik (*echolocation*) seperti yang digunakan kelelawar dan kapal selam.
- Menghitung jarak dari selisih waktu (Time-of-Flight) gelombang suara.
- Menerapkan koreksi kecepatan suara berdasarkan suhu udara untuk akurasi tinggi.
- Membangun sistem deteksi zona multi-level (Aman, Waspada, Bahaya).
- Menerapkan arsitektur **Non-Blocking State Machine** yang aman dari deadlock `pulseIn()`.

---

## 30.1 Mengenal HC-SR04 — Indera SONAR Buatan

**HC-SR04** adalah sensor jarak ultrasonik yang mengadopsi prinsip *SONAR* (*Sound Navigation And Ranging*) — teknologi yang sama digunakan oleh kelelawar, lumba-lumba, dan kapal selam militer sejak Perang Dunia I.

Cara kerjanya elegan dan brilian: sensor memancarkan **sinyal suara ultrasonik** (40.000 Hz — di atas batas pendengaran manusia 20.000 Hz), lalu mengukur berapa lama waktu yang dibutuhkan gelombang itu untuk memantul kembali dari benda di depannya. Dari selisih waktu itu, kita dapat menghitung jarak dengan presisi tinggi.

```
   HC-SR04
  ┌───────┐
  │ [Trig]│──── Menembak: Sinyal 40kHz 8 siklus selama 10µs
  │       │
  │ [Echo]│──── Mendengar: HIGH selama waktu perjalanan pulang-pergi
  └───────┘

  Timeline Sinyal:
  
  TRIG  ___┌──┐_____________________________________________ (10µs pulse)
            │  │
  ECHO  ____│__└──────────────┐__________________________ (HIGH = waktu tempuh)
            │                  │
  Jarak = (Lebar Pulsa Echo × Kecepatan Suara) / 2
  (/2 karena gelombang menempuh jarak PERGI + BALIK!)
```

### Persamaan Jarak

```
Jarak (cm) = (Waktu Echo (µs) × Kecepatan Suara (cm/µs)) / 2

Pada suhu 20°C standard:
  Kecepatan Suara = 34,300 cm/s = 0.0343 cm/µs

Maka:
  Jarak (cm) = WaktuEcho_µs × 0.0343 / 2
             = WaktuEcho_µs / 58.31

Angka ajaib "58" inilah yang sering muncul di tutorial!
Tapi itu hanya akurat pada suhu 20°C — baca Bagian 30.2 untuk versi profesional!
```

### Spesifikasi Hardware Kit Bluino

```
┌─────────────────────────────────────────────────────────────────┐
│              SPESIFIKASI HC-SR04 — KIT BLUINO                   │
├──────────────────────────┬──────────────────────────────────────┤
│ Parameter                │ Nilai                                │
├──────────────────────────┼──────────────────────────────────────┤
│ Koneksi ke Kit           │ Pin CUSTOM (kabel jumper manual)     │
│ Pin Trigger (TX)         │ Bebas dipilih (rekomendasi: IO25)    │
│ Pin Echo (RX)            │ Bebas dipilih (rekomendasi: IO26)    │
│ Tegangan Operasi         │ 5V DC                               │
│ Rentang Pengukuran       │ 2 cm – 400 cm                       │
│ Akurasi Tipikal          │ ±3 mm                               │
│ Sudut Efektif            │ < 15° (kerucut sempit ke depan)     │
│ Frekuensi Ultrasonik     │ 40 kHz                              │
│ Lebar Pulsa Trigger      │ 10 µs (Minimum!)                    │
│ Antarmuka Output         │ Digital GPIO (PWM-like ECHO pulse)  │
└──────────────────────────┴──────────────────────────────────────┘
```

> ⚠️ **Perhatian Wiring:** HC-SR04 membutuhkan tegangan **5V**, bukan 3.3V! Namun pin **Echo** mengeluarkan sinyal 5V yang harus dilindungi ESP32 (3.3V toleran). Kit Bluino kemungkinan sudah memiliki proteksi resistif — periksa skematik. Untuk keamanan maksimal, gunakan pembagi tegangan R1:10KΩ dan R2:20KΩ pada jalur Echo.

---

## 30.2 Fisika Suara — Mengapa Suhu Mempengaruhi Akurasi?

Ini adalah rahasia yang disembunyikan dari sebagian besar tutorial!

**Kecepatan suara di udara BUKAN konstanta** — ia berubah signifikan tergantung suhu udara. Pada suhu tropis Indonesia (30–35°C), kecepatan suara lebih cepat dibanding standar 20°C internasional. Jika kita tetap memakai angka tetap, pengukuran jarak kita akan selalu *melenceng* secara sistematis!

**Persamaan Kecepatan Suara (Fisika Udara):**
```
v_suara (m/s) = 331.3 × √(1 + T/273.15)

Di mana T = Suhu Udara dalam °C

Contoh perbandingan:
  T = 20°C → v = 343.2 m/s  (Standar internasional)
  T = 30°C → v = 349.1 m/s  (Suhu ruangan Indonesia)
  T = 35°C → v = 352.0 m/s  (Siang hari Indonesia)

Selisih kecepatan antara 20°C vs 35°C = 8.8 m/s (2.6%)
Pada jarak pengukuran 100cm, selisih itu menjadi ERROR 2.6cm!
Untuk robot navigasi presisi, 2.6cm = tabrakan fatal!
```

**Versi Aproksimasi Lebih Sederhana (untuk siswa):**
```
v_suara (cm/µs) ≈ (331.3 + 0.606 × T) / 10000

Di mana T = Suhu °C

Contoh:
  T = 20°C → v = 0.034324 cm/µs
  T = 30°C → v = 0.034930 cm/µs
```

> 💡 **Integrasi DHT11 (BAB 26):** Di Kit Bluino, DHT11 sudah terpasang permanen di IO27. Kombinasikan HC-SR04 + DHT11 untuk mendapatkan **Temperature-Compensated Ultrasonic** — pengukuran jarak yang secara otomatis mengkoreksi dirinya berdasarkan suhu udara aktual!

---

## 30.3 Instalasi Library

HC-SR04 tidak memerlukan library eksternal! Seluruh komunikasi dilakukan menggunakan dua fungsi bawaan Arduino:
- `digitalWrite()` — Menembakkan pulsa TRIG
- `pulseIn()` — Mendengar lebar pulsa ECHO

```
Tidak perlu menginstal library apapun!
Cukup pastikan Board ESP32 sudah terpasang di Board Manager.
```

---

## 30.4 Program 1: Jarak Dasar (Halo HC-SR04!)

Program paling sederhana untuk membuktikan sensor bekerja dan mengukur jarak mentah.

```cpp
/*
 * BAB 30 - Program 1: HC-SR04 Jarak Dasar
 *
 * Membaca jarak menggunakan prinsip Time-of-Flight (TOF).
 * Tidak memerlukan library tambahan — murni Arduino GPIO!
 *
 * Wiring (Kit Bluino — Koneksi MANUAL via Kabel Jumper):
 *   HC-SR04 VCC   → 5V   (Header daya kit)
 *   HC-SR04 GND   → GND
 *   HC-SR04 TRIG  → IO25 (Bisa diganti pin mana saja)
 *   HC-SR04 ECHO  → IO26 (via pembagi tegangan 5V→3.3V!)
 */

const int PIN_TRIG = 25;
const int PIN_ECHO = 26;

// Kecepatan suara pada suhu 20°C standar (cm per mikrodetik)
const float KECEPATAN_SUARA_CM_US = 0.0343f;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW); // Pastikan TRIG idle di LOW

  Serial.println("══════════════════════════════════════════");
  Serial.println("  BAB 30: HC-SR04 — Sensor Jarak Dasar");
  Serial.println("══════════════════════════════════════════");
  Serial.println("  Arahkan sensor ke objek → jarak tampil!");
  Serial.println("──────────────────────────────────────────");
}

// Fungsi helper: Mengirim pulsa TRIG dan mendapatkan panjang ECHO
long bacaEchoMikrodetik() {
  // Protokol HC-SR04:
  // 1. Pastikan TRIG LOW selama 2µs (bersihkan sisa sinyal)
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  // 2. Kirim pulsa HIGH selama 10µs (minimum wajib!)
  //    Sensor internal akan memancarkan 8 siklus 40kHz
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 3. Tunggu dan ukur lebar pulsa ECHO (timeout 30ms = jarak > 5 meter)
  return pulseIn(PIN_ECHO, HIGH, 30000UL);
}

void loop() {
  long durasi = bacaEchoMikrodetik();

  // Validasi: durasi 0 berarti timeout (tidak ada objek/di luar jangkauan)
  if (durasi == 0) {
    Serial.println("⚠️  Tidak ada objek terdeteksi (> 4 meter atau < 2 cm)");
  } else {
    // Hitung jarak: durasi perjalanan pulang-pergi / 2 × kecepatan suara
    float jarakCm = (durasi * KECEPATAN_SUARA_CM_US) / 2.0f;

    Serial.printf("📏 Jarak: %6.1f cm  |  (%4ld µs)\n", jarakCm, durasi);
  }

  delay(200);
}
```

**Contoh output:**
```text
══════════════════════════════════════════
  BAB 30: HC-SR04 — Sensor Jarak Dasar
══════════════════════════════════════════
  Arahkan sensor ke objek → jarak tampil!
──────────────────────────────────────────
📏 Jarak:   23.5 cm  |  (1370 µs)
📏 Jarak:   23.4 cm  |  (1367 µs)
📏 Jarak:   23.6 cm  |  (1374 µs)
⚠️  Tidak ada objek terdeteksi (> 4 meter atau < 2 cm)
```

---

## 30.5 Program 2: Jarak dengan Koreksi Suhu (Presisi Tinggi)

Mengintegrasikan pembacaan DHT11 (sudah built-in di IO27) untuk mengkoreksi kecepatan suara secara real-time.

```cpp
/*
 * BAB 30 - Program 2: HC-SR04 + DHT11 Temperature Compensation
 *
 * Menggabungkan data suhu udara dari DHT11 (IO27) untuk mengkoreksi
 * kecepatan suara secara real-time. Menghasilkan akurasi jarak yang
 * konsisten di berbagai kondisi suhu, terutama di iklim tropis Indonesia!
 *
 * Perbedaan tanpa koreksi vs dengan koreksi pada 35°C, jarak 1 meter:
 *   Tanpa koreksi : ~97.4 cm (error -2.6 cm)
 *   Dengan koreksi: ~100.0 cm (akurat!)
 */

#include <DHT.h>

// ── Konfigurasi Pin ───────────────────────────────────────────────
const int PIN_TRIG   = 25;
const int PIN_ECHO   = 26;
const int PIN_DHT    = 27; // Built-in di Kit Bluino!

DHT dht(PIN_DHT, DHT11);

// ── Interval Pembaruan Suhu ───────────────────────────────────────
// DHT11 lambat — update setiap 10 detik sudah cukup
const unsigned long INTERVAL_SUHU_MS = 10000UL;
unsigned long tSuhuTerakhir = 0;

float suhuAmbien = 27.0f; // Nilai awal default suhu kamar Indonesia

// Fungsi kalkulasi kecepatan suara berdasarkan suhu (Fisika Akustik)
float hitungKecepatanSuara(float suhuC) {
  // Persamaan aproksimasi linear (akurat hingga 0.1% pada range 0-50°C)
  return (331.3f + 0.606f * suhuC) / 10000.0f; // dalam cm/µs
}

long bacaEchoMikrodetik() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  return pulseIn(PIN_ECHO, HIGH, 30000UL);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // 💡 KRITIS: DHT11 butuh waktu pemanasan absolut 2 detik sejak diberi daya!
  // Jika dibaca instan saat boot, ia PASTI mereturn NaN (Not a Number).
  Serial.println(" Menunggu pemanasan sensor DHT11 (2 detik)...");
  delay(2000);

  // Baca suhu riil pertama untuk titik tolak
  float t = dht.readTemperature();
  if (!isnan(t)) suhuAmbien = t;

  Serial.println("══════════════════════════════════════════════");
  Serial.println(" BAB 30: HC-SR04 + Koreksi Temperatur DHT11");
  Serial.println("══════════════════════════════════════════════");
  Serial.printf( " Suhu Awal   : %.1f °C\n", suhuAmbien);
  Serial.printf( " v_suara     : %.5f cm/µs\n", hitungKecepatanSuara(suhuAmbien));
  Serial.println("══════════════════════════════════════════════");
}

void loop() {
  unsigned long sekarang = millis();

  // ── Update suhu setiap 10 detik ───────────────────────────────
  if (sekarang - tSuhuTerakhir >= INTERVAL_SUHU_MS) {
    tSuhuTerakhir = sekarang;
    float t = dht.readTemperature();
    if (!isnan(t)) {
      suhuAmbien = t;
      Serial.printf("🌡️  Suhu diperbarui: %.1f °C | v_suara: %.5f cm/µs\n",
                    suhuAmbien, hitungKecepatanSuara(suhuAmbien));
    }
  }

  // ── Baca jarak dengan kecepatan suara yang sudah terkoreksi ───
  long durasi = bacaEchoMikrodetik();

  if (durasi == 0) {
    Serial.println("⚠️  Di luar jangkauan (< 2cm atau > 400cm)");
  } else {
    float vSuara  = hitungKecepatanSuara(suhuAmbien);
    float jarakCm = (durasi * vSuara) / 2.0f;

    // Tampilkan perbandingan dengan dan tanpa koreksi suhu
    float jarakTanpaKoreksi = (durasi * 0.0343f) / 2.0f;
    float selisih = jarakCm - jarakTanpaKoreksi;

    Serial.printf("📏 Jarak Koreksi  : %6.1f cm\n", jarakCm);
    Serial.printf("   Tanpa Koreksi  : %6.1f cm  (selisih: %+.1f cm)\n",
                  jarakTanpaKoreksi, selisih);
    Serial.println("──────────────────────────────────────────────");
  }

  delay(500);
}
```

**Contoh output pada suhu 33°C:**
```text
══════════════════════════════════════════════
 BAB 30: HC-SR04 + Koreksi Temperatur DHT11
══════════════════════════════════════════════
 Suhu Awal   : 33.0 °C
 v_suara     : 0.03513 cm/µs
══════════════════════════════════════════════
📏 Jarak Koreksi  :  100.0 cm
   Tanpa Koreksi  :   97.7 cm  (selisih: +2.3 cm)
──────────────────────────────────────────────
```

---

## 30.6 Program 3: Sistem Deteksi Zona Multi-Level

Membangun modul sensor parkir bergaya industri: **Aman → Waspada → Bahaya**.
Di bab ini, kita akan MENGHAPUS perintah `delay()` secara total di dalam `loop()` dan beralih menggunakan pola **"Cooperative Multitasking"** berbasis `millis()`.

Kita akan memecah kerja ESP32 menjadi 3 "Task" (Tugas) paralel:
1. **Task Sensor:** Membaca HC-SR04 setiap 60ms (batas minimum fisik sensor).
2. **Task Layar (Print):** Mencetak jarak ke serial hanya setiap 500ms agar mata kita bisa membacanya tanpa pusing.
3. **Task Buzzer:** Terus-menerus memantau zona bahaya dan mengatur irama kedipan suara dengan kecepatan maksimal.

```cpp
 * BAB 30 - Program 3: Sistem Deteksi Zona (Parking Sensor)
 *
 * Meniru sistem peringatan parkir kendaraan modern!
 * Tiga zona berbeda dengan umpan balik berbeda:
 *   🟢 AMAN    : > 50 cm   → Tidak ada bunyi
 *   🟡 WASPADA : 20 - 50 cm → Beep lambat (1/detik)
 *   🔴 BAHAYA  : < 20 cm   → Beep cepat dan peringatan!
 *
 * Buzzer terhubung ke pin yang ditentukan (aktif HIGH di Kit Bluino).
 */

// ── Konfigurasi Pin ───────────────────────────────────────────────
const int PIN_TRIG  = 25;
const int PIN_ECHO  = 26;
const int PIN_BUZZER = 2; // Pin Active Buzzer Kit Bluino

// ── Konfigurasi Zona Jarak (cm) ───────────────────────────────────
const float ZONA_BAHAYA   = 20.0f; // < 20 cm
const float ZONA_WASPADA  = 50.0f; // 20 - 50 cm
// Lebih dari 50 cm = ZONA AMAN

// ── State Buzzer Non-Blocking ─────────────────────────────────────
unsigned long tBuzzerTerakhir = 0;
bool          statusBuzzer    = false;

// ── Struktur Data Pengukuran ──────────────────────────────────────
struct DataJarak {
  float   jarakCm;
  bool    valid;
};

long bacaEchoMikrodetik() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  return pulseIn(PIN_ECHO, HIGH, 30000UL);
}

DataJarak ukurJarak() {
  DataJarak hasil;
  long durasi = bacaEchoMikrodetik();

  if (durasi == 0) {
    hasil.jarakCm = -1.0f;
    hasil.valid   = false;
  } else {
    hasil.jarakCm = (durasi * 0.0343f) / 2.0f;
    hasil.valid   = (hasil.jarakCm >= 2.0f && hasil.jarakCm <= 400.0f);
  }
  return hasil;
}

// Update buzzer secara Non-Blocking berdasarkan zona
void updateBuzzer(float jarak) {
  unsigned long sekarang = millis();

  unsigned long intervalBeep;

  // Jika jarak tidak valid (misal -1 saat tak ada halangan), anggap ia AMAN ekstrem
  if (jarak < 0) {
    digitalWrite(PIN_BUZZER, LOW);
    statusBuzzer = false;
    return;
  }

  if (jarak < ZONA_BAHAYA) {
    intervalBeep = 100UL;  // Beep sangat cepat: 10 kali/detik!
  } else if (jarak < ZONA_WASPADA) {
    intervalBeep = 500UL;  // Beep lambat: 2 kali/detik
  } else {
    // Zona aman: buzzer diam 100%
    digitalWrite(PIN_BUZZER, LOW);
    statusBuzzer = false;
    return;
  }

  // Toggle buzzer Non-Blocking
  if (sekarang - tBuzzerTerakhir >= intervalBeep) {
    tBuzzerTerakhir = sekarang;
    statusBuzzer = !statusBuzzer;
    digitalWrite(PIN_BUZZER, statusBuzzer ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG,   OUTPUT);
  pinMode(PIN_ECHO,   INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_TRIG,   LOW);
  digitalWrite(PIN_BUZZER, LOW);

  Serial.println("═══════════════════════════════════════════════");
  Serial.println(" BAB 30: Sistem Deteksi Zona (Parking Sensor)");
  Serial.println("═══════════════════════════════════════════════");
  Serial.printf( " Zona BAHAYA  : < %.0f cm  🔴\n", ZONA_BAHAYA);
  Serial.printf( " Zona WASPADA : %.0f - %.0f cm  🟡\n",
                 ZONA_BAHAYA, ZONA_WASPADA);
  Serial.printf( " Zona AMAN    : > %.0f cm  🟢\n", ZONA_WASPADA);
  Serial.println("═══════════════════════════════════════════════\n");
}

// Variabel jembatan komunikasi antar-Task
float jarakParkir = -1.0f; 

void loop() {
  unsigned long sekarang = millis();

  // ── TASK 1: Membaca Sensor Secara Berkala (60ms) ────────────────
  // Hukum Fisika HC-SR04 mutlak: Jangan tembak lebih cepat dari 60ms!
  static unsigned long tSensorTerakhir = 0;
  if (sekarang - tSensorTerakhir >= 60UL) {
    tSensorTerakhir = sekarang;
    
    DataJarak ukuran = ukurJarak();
    jarakParkir = ukuran.valid ? ukuran.jarakCm : -1.0f;
  }

  // ── TASK 2: Cetak Layar Serial (500ms) ──────────────────────────
  static unsigned long tPrintTerakhir = 0;
  if (sekarang - tPrintTerakhir >= 500UL) {
    tPrintTerakhir = sekarang;

    if (jarakParkir < 0) {
      Serial.println("⚠️  Tidak ada halangan di depan.");
    } else {
      const char* zona = (jarakParkir < ZONA_BAHAYA) ? "BAHAYA " : 
                         (jarakParkir < ZONA_WASPADA) ? "WASPADA" : "AMAN   ";
      const char* ikon = (jarakParkir < ZONA_BAHAYA) ? "🔴" : 
                         (jarakParkir < ZONA_WASPADA) ? "🟡" : "🟢";

      Serial.printf("%s %s %5.1f cm\n", ikon, zona, jarakParkir);
    }
  }

  // ── TASK 3: Alarm Buzzer (Non-Stop Execution) ───────────────────
  // Ribuan kali per detik, fungsi ini terus-menerus mengecek apakah
  // ia harus membalik (toggle) status on/off buzzer. Sangat responsif!
  updateBuzzer(jarakParkir);
}
```

**Contoh output saat mendekati objek:**
```text
🟢 AMAN    78.3 cm
🟢 AMAN    52.1 cm
🟡 WASPADA 45.7 cm   ← Buzzer mulai berdenyut lambat
🟡 WASPADA 32.4 cm
🔴 BAHAYA  17.8 cm   ← Buzzer berteriak cepat!
🔴 BAHAYA  14.1 cm
```

---

## 30.7 Program 4: Arsitektur Hardware Interrupt (Non-Blocking Mutlak)

**Batas Kebuntuan Multitasking `millis()`:**
Di Program 3 kita sudah menghancurkan cengkeraman setan `delay()`! Namun, jika kamu perhatikan seksama, irama beep buzze saat berada di Zona Bahaya kadang-kadang terdengar "terpincang" atau tersendat *(Jitter)*.

Mengapa? Karena fungsi `pulseIn()` di dalam `ukurJarak()`! Perintah `pulseIn` akan **memblokir penuh CPU hingga 30ms** jika tak ada halangan. Selama 30ms itu... Task 3 (Buzzer) tidak dapat berdetak, layar tertahan, dan WiFi ESP32 bisa saja *timeout* koneksi!

Untuk sistem satelit, drone, atau WiFi IoT kaliber tinggi, ini adalah **racun laten** mematikan. Kita butuh solusi agung: **Interupsi Perangkat Keras**.

*(Mungkin kamu berpikir: "Kalau begitu matikan pulseIn dan kerjakan deteksi waktu Echo menggunakan batas micros() saja sendiri di dalam loop()!").*
**SALAH BESAR!** Gelombang suara HC-SR04 membalas panggilan dalam level **mikrodetik** (1/1.000.000 detik). Putaran `loop()` biasa membutuhkan waktu ber-mili-mili-detik. Apabila CPU lengah sejentik saja mengurus layar LCD, sinyal Echo akan terlewatkan dan jarak yang dihitung melambung tembus tembok luar angkasa!

**Solusi Industri:** Gunakan **Layanan Interupsi Perangkat Keras (Interrupt Service Routine / ISR)**.
Interupsi membuat ESP32 berjanji: *"Segera setelah pin ECHO tiba-tiba naik/turun di belakang layar, saya akan **MENGGUNTING (MENJEDA INSTAN)** program apapun yang sedang dikerjakan detik itu juga, bergegas merekam mikrodetiknya di sela-sela memori tanpa antre selagi ping berlangsung, lalu melanjutkan program persis di titik tergunting sebelumnya!"*

```cpp
/*
 * BAB 30 - Program 4: Keajaiban Interupsi (Non-Blocking HC-SR04)
 *
 * Menggantikan pulseIn() yang memblokir dengan Hardware Interrupts!
 * Ini adalah arsitektur paling presisi dan efisien CPU di dunia industri
 * untuk mengukur kejadian yang sangat singkat (mikrodetik).
 */

// ── Konfigurasi Pin ───────────────────────────────────────────────
const int PIN_TRIG = 25;
const int PIN_ECHO = 26;

// ── Variabel Volatile untuk Interupsi (SANGAT PENTING!) ───────────
// Kata kunci `volatile` memaksa RAM agar variabel ini selalu dibaca 
// dari memori asli dan tidak di-cache oleh CPU, karena nilainya bisa 
// berubah-ubah tiba-tiba di "belakang layar" oleh fungsi Interupsi.
volatile unsigned long tEchoNaik   = 0;
volatile unsigned long durasiEcho  = 0;
volatile bool          dataBaruSiap = false;

// ── Rutinitas Layanan Interupsi (ISR) ─────────────────────────────
// Fungsi ini dipanggil OTOMATIS oleh otak silikon ESP32 ketika 
// pin ECHO berubah status (Naik menjadi HIGH, atau Turun menjadi LOW)!
// IRAM_ATTR menempatkan fungsi ini di RAM tercepat ESP32 (mengurangi lag).
void IRAM_ATTR isrEcho() {
  if (digitalRead(PIN_ECHO) == HIGH) {
    // Ping berangkat! Rekam stopwatch
    tEchoNaik = micros();
  } else {
    // Ping pulang! 
    // 💡 PROTEKSI GHOST ECHO: Jika CPU sedang sibuk berat dan tak sengaja
    // 'tuli' melewatkan interupsi Naik, ia dilarang keras memakai tEchoNaik basi!
    if (tEchoNaik != 0) {
      durasiEcho = micros() - tEchoNaik;
      tEchoNaik = 0; // Reset stopwatch agar tak terpakai dua kali
      dataBaruSiap = true;
    }
  }
}

// ── Fungsi Pemicu Non-Blocking ────────────────────────────────────
unsigned long tPemicuTerakhir = 0;

void triggerSensor() {
  unsigned long sekarang = millis();
  
  // Tembak sinyal maksimum setiap 60ms (Batas pantulan berganda)
  if (sekarang - tPemicuTerakhir >= 60UL) {
    tPemicuTerakhir = sekarang;
    
    // Tembakan sensor berlangsung sangat cepat (12µs) sehingga tidak perlu
    // interupsi. Ini adalah satu-satunya "blokade" tapi sangat-sangat remeh!
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);  // PIN_ECHO adalah pin penerima Interupsi
  digitalWrite(PIN_TRIG, LOW);

  // 💡 Mendaftarkan Interupsi!
  // Menyuruh ESP32: "Hei, tolong awasi pin ECHO. Kalau sinyalnya BERUBAH (CHANGE), 
  // langsung hentikan kerjaanmu dan jalankan fungsi isrEcho!"
  attachInterrupt(digitalPinToInterrupt(PIN_ECHO), isrEcho, CHANGE);

  Serial.println("══════════════════════════════════════════════════");
  Serial.println(" BAB 30: HC-SR04 Hardware Interrupts (Non-Blocking)");
  Serial.println("══════════════════════════════════════════════════");
  Serial.println(" Menunggu objek di depan sensor...");
  Serial.println("══════════════════════════════════════════════════\n");
}

void loop() {
  // 1. Secara berkala menembak pelatuk sensor (tembak dan lupakan)
  triggerSensor();

  // 2. Apabila fungsi Interupsi di belakang layar sudah selesai mengukur...
  if (dataBaruSiap) {
    
    // 💡 CRITICAL SECTION (Daerah Aman):
    // Matikan interupsi sesaat! Jangan sampai sensor menembak dan menimpa  
    // variabel durasiEcho selagi kita sedang menyalin isinya!
    noInterrupts();
    unsigned long durasiAman = durasiEcho;
    dataBaruSiap = false;
    interrupts(); // Nyalakan interupsi kembali

    // 3. Kalkulasi Jarak Penuh Damai
    float jarakCm = (durasiAman * 0.0343f) / 2.0f;

    // Filter jarak liar/timeout
    if (jarakCm >= 2.0f && jarakCm <= 400.0f) {
      Serial.printf("[%6lu ms] 📏 Jarak: %5.1f cm  (%4lu µs)\n",
                    millis(), jarakCm, durasiAman);
    } else {
      Serial.printf("[%6lu ms] ⚠️  Di luar jangkauan\n", millis());
    }
  }

  // BUKTIKAN KEKUATAN INTERUPSI:
  // Kamu bisa menaruh delay(200) di sini, dan pengukuran JARAK TETAP AKURAT!
  // Karena penghitungan durasi dilakukan oleh Interupsi yang bisa memotong delay!
}
```

**Kekuatan Ajaib Interupsi:** Coba tambahkan perintah yang berat di dalam `loop()` seperti koneksi HTTP atau kalkulasi Math rumit. Pembacaan Sensor HC-SR04 akan tetap **absolut akurat** karena interupsi tidak mengantre!

---

## 30.8 Tips & Panduan Troubleshooting

### 1. Nilai Jarak Terbaca Acak/Tidak Stabil?
```text
Penyebab paling umum:

A. Interferensi sinyal Echo → Trig terlalu dekat (minimal 2 cm antar pembacaan)
   Solusi: Tambahkan delay 60ms minimal antar pembacaan!
   INTERVAL_UKUR = 60UL (default Program 4 sudah benar ini!)

B. Permukaan yang menyerap suara:
   Kapas, busa, kain berbulu, sudut sempit → Sinyal echo melemah!
   Permukaan terbaik: Datar, keras, tegak lurus ke sensor (kayu, tembok, metal)

C. Sudut objek > 15°:
   Jika objek miring lebih dari 15°, gelombang ultrasonik memantul ke arah lain
   dan tidak kembali ke sensor. Hasil: 0 atau noise!
```

### 2. `pulseIn()` Selalu Mengembalikan 0?
```text
Langkah diagnosis:

A. Periksa wiring TRIG dan ECHO tidak tertukar
B. Konfirmasi tegangan VCC sensor = 5V (bukan 3.3V!)
   HC-SR04 tidak bekerja pada 3.3V — jarak terbaca 0 semua!
C. Probe pin ECHO dengan LED: Jika LED berkedip singkat saat TRIG dipicu,
   sinyal ECHO ada namun mungkin terlalu pendek dibaca
D. Periksa pembagi tegangan pada jalur ECHO jika terpasang
```

### 3. Jarak Selalu Terlalu Pendek atau Panjang Secara Konsisten?
```text
Ini adalah masalah "Systematic Offset" — bias yang terjadi setiap saat.

Dua kemungkinan:
A. Suhu udara berbeda dari 20°C → Gunakan Program 2 (Koreksi Temperatur)!
B. Kabel Echo terlalu panjang (>30cm) → Menambah kapasitansi yang memperlambat
   kenaikan sinyal ECHO, membuat ESP32 mendeteksi sedikit terlambat.

Solusi B: Kurangi panjang kabel atau tambahkan buffer transistor (74HC04).
```

### 4. Mengapa Tidak Bisa Mengukur Jarak < 2 cm?
```text
Keterbatasan fisik HC-SR04: Setelah menembak pulsa 10µs, sensor butuh
waktu ~600µs untuk "pulih" dari ledakan ultrasonik sendiri.
Selama 600µs itu, pin ECHO bisa berisi sisa getaran palsu dari letupan sendiri!

Jarak minimum = (0 + 600µs recovery) × 0.0343/2 = ~1 cm "blind zone".
Desain HC-SR04 secara konservatif menetapkan minimum 2 cm.

Untuk jarak sangat dekat (< 4cm): Gunakan sensor VL53L0X (Time-of-Flight laser)
yang mampu mengukur presisi hingga 1mm!
```

### 5. Pengukuran Berbeda Jauh Saat Dua HC-SR04 Dipakai Bersamaan?
```text
INTERFERENSI LINTAS SENSOR!

Jika dua HC-SR04 ditempatkan berdekatan dan keduanya aktif bersamaan,
gelombang 40kHz dari Sensor A bisa ditangkap oleh Sensor B sebagai echo palsu!

Solusi: Aktifkan secara BERGANTIAN (Multiplexed Trigger):
  1. Tembak TRIG Sensor A → Baca ECHO A → Tunggu 60ms
  2. Tembak TRIG Sensor B → Baca ECHO B → Tunggu 60ms
  3. Ulangi

JANGAN pernah memicu kedua sensor pada saat bersamaan!
```

### 6. Jarak Tiba-Tiba Melompat Ngawur Saat Digabung FastLED / OLED?
```text
Symptom: Di Program 4 (Interupsi), jarak berkedip liar menjadi ratusan meter
padahal halangan ada di depan mata.

Penyebab "Missed Interrupt": Library berat (seperti FastLED.show atau layar OLED besar)
seringkali 'menggembok' pendengaran CPU (Global Interrupt Masking) selama bertugas.
Jika pin ECHO **naik** persis saat CPU digembok, interupsi Naiknya akan TERLEWAT!

Dampak: Saat ECHO **turun**, CPU baru sadar dan mengurangi waktu saat ini dengan 
waktu `tEchoNaik` dari tembakan KEMARIN (yang mana nilainya sangat lawas)!

Solusi Masterpiece: Program 4 kita sudah kebal terhadap ini! Terdapat "Ghost Echo Guard" 
yakni kode `if (tEchoNaik != 0)` yang akan mendelete paksa segala rekam jejak
ECHO turun apabila ECHO naiknya absen. Pengukuran liar pun hangus seketika!
```

### 7. Jarak Menjadi Kacau/Pendek Saat Dinamo Berputar? (Voltage Sag)
```text
Symptom: Saat robot diam, sensor akurat. Begitu roda (Motor DC L298N) atau Relay aktif,
jarak ultrasonik mendadak menjadi 0 cm atau bergemuruh ngawur.

Penyebab Fisika (Brownout): HC-SR04 SANGAT sensitif terhadap kestabilan "Rel Darah" (Tegangan 5V). 
Saat dinamo ditarik gas mendadak, ia menyedot arus ekstrem (Inrush Current) yang menyebabkan 
pasokan tegangan 5V terjun bebas (Voltage Sag) sesaat hingga 4.2V! 
Turunnya tegangan sesaat ini membuat Penguat Operasional (Op-Amp) di dalam otak sensor HC-SR04
kelaparan listrik dan mengeluarkan "Noise Hantu".

Solusi Industri Robotik:
1. Pasang Kapasitor Elektrolit (misal 470µF) melintang tepat menempel di kaki VCC dan GND sensor.
2. PISAHKAN Rel Power: Jangan pernah merantai VCC sensor di satu titik kabel paralel
   dengan VCC milik Dinamo DC.
3. Gunakan baterai Li-Ion bertenaga tinggi untuk sistem mekanik!
```

---

## 30.9 Ringkasan

```
┌─────────────────────────────────────────────────────────────────────┐
│               RINGKASAN BAB 30 — ESP32 + HC-SR04                    │
├─────────────────────────────────────────────────────────────────────┤
│ Wiring      = TRIG→IO25, ECHO→IO26 (via Custom pins + jumper)       │
│ Tegangan    = VCC=5V, ECHO butuh pembagi tegangan 5V→3.3V!          │
│ Library     = Tidak diperlukan (murni digitalWrite + pulseIn)        │
│                                                                     │
│ Protokol Pengukuran:                                                │
│   1. TRIG LOW → 2µs (bersihkan)                                    │
│   2. TRIG HIGH → 10µs (min!) → TRIG LOW                            │
│   3. Ukur lebar pulsa ECHO HIGH (timeout 30ms)                      │
│   4. Jarak = (durasi × kecepatan_suara) / 2                        │
│                                                                     │
│ Kecepatan Suara:                                                    │
│   Standard (20°C)  : 0.0343 cm/µs  (pembagi ~58)                   │
│   Dengan koreksi T : (331.3 + 0.606×T) / 10000 cm/µs              │
│                                                                     │
│ PERINGATAN KRITIS:                                                  │
│   pulseIn() = MEMBLOKIR hingga 30ms!                                │
│   Gunakan Hardware Interrupts (Prog. 4) untuk sistem multi-tasking! │
│                                                                     │
│ Intervalsan Aman Minimal = 60ms antar pengukuran (16Hz max)         │
│ Jangkauan Valid = 2cm – 400cm, Sudut Kerucut < 15°                 │
│ Blind Zone = < 2 cm (keterbatasan fisik sensor)                    │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 30.10 Latihan

1. **Penggaris Digital Serial Plotter:** Ubah Program 1 agar Serial hanya mencetak satu angka jarak per baris (tanpa teks lain). Buka Serial Plotter Arduino IDE untuk melihat grafik jarak bergerak real-time saat tangan Anda mendekati dan menjauh dari sensor!

2. **Alarm Parkir Bertahap:** Modifikasi Program 3 dengan tambahan 5 zona (bukan 3): Aman (>80cm), Siaga (60-80cm), Waspada (40-60cm), Kritis (20-40cm), Bahaya (<20cm). Setiap zona memiliki pola beep buzzer yang berbeda (interval berbeda). Tambahkan LED yang menyala sesuai jumlah zona yang dilewati.

3. **Pengukuran Rata-Rata Bergerak (Moving Average):** Di Program 2, implementasikan *Moving Average Filter* dengan N=5 sampel. Simpan 5 nilai jarak terbaru dalam array, lalu tampilkan rata-ratanya. Bandingkan stabilitas bacaan `jarakMentah` vs `jarakFilter` saat sensor diguncang pelan.

4. **Level Air / Kedalaman Benda:** Tempatkan sensor HC-SR04 menghadap ke bawah di atas wadah container dengan ketinggian tetap (misal 30cm dari dasar). Saat benda/air ditambahkan ke wadah, level naik dan jarak yang terbaca akan berkurang. Hitung tinggi benda/air melalui rumus: `tinggi = jarak_kosong - jarak_terukur`. Tampilkan dalam bentuk persentase level penuh!

5. **Pengukuran Non-Blocking Tertanam:** Integrasikan Program 4 (Interupsi HC-SR04) dengan Program 4 BAB 29 (State Machine MPU-6050). Jalankan kedua sensor secara **bersamaan** dalam satu `loop()` tanpa saling memblokir. Tampilkan `[Jarak: xx.x cm | Roll: xx.x° | Pitch: xx.x°]` setiap 500ms menggunakan data register global dari masing-masing!

---

*Selanjutnya → [BAB 31: PIR AM312 — Deteksi Gerakan](../BAB-31/README.md)*
