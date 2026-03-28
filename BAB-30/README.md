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

  // Baca suhu awal saat boot
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

Membangun sistem deteksi jarak bergaya kendali industri: **Aman → Waspada → Bahaya** dengan umpan balik buzzer dan Serial berbeda per zona.

```cpp
/*
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

  if (jarak < ZONA_BAHAYA) {
    intervalBeep = 100UL;  // Beep sangat cepat: 10 kali/detik!
  } else if (jarak < ZONA_WASPADA) {
    intervalBeep = 500UL;  // Beep lambat: 2 kali/detik
  } else {
    // Zona aman: matikan buzzer
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

void loop() {
  DataJarak ukuran = ukurJarak();

  if (!ukuran.valid) {
    Serial.println("⚠️  Di luar jangkauan!");
    digitalWrite(PIN_BUZZER, LOW);
    delay(200);
    return;
  }

  float jarak = ukuran.jarakCm;
  const char* zona;
  const char* ikon;

  if (jarak < ZONA_BAHAYA) {
    zona = "BAHAYA ";
    ikon = "🔴";
  } else if (jarak < ZONA_WASPADA) {
    zona = "WASPADA";
    ikon = "🟡";
  } else {
    zona = "AMAN   ";
    ikon = "🟢";
  }

  Serial.printf("%s %s %5.1f cm\n", ikon, zona, jarak);

  updateBuzzer(jarak);

  delay(100); // Loop 10Hz
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

## 30.7 Program 4: Non-Blocking State Machine (Tanpa `pulseIn` Memblokir!)

> ⚠️ **Kelemahan Fatal `pulseIn()`:** Perintah `pulseIn()` yang kita gunakan di Program 1–3 secara diam-diam **memblokir** seluruh `loop()` selama menunggu sinyal ECHO — bisa sampai 30.000 µs (30ms)! Untuk sistem yang menjalankan WiFi, OLED, atau sensor lain secara bersamaan, ini adalah **racun laten** yang sama berbahayanya dengan `delay()`.

Solusinya menggunakan **mesin keadaan (State Machine)** berbasis interupsi waktu yang membagi proses pengukuran menjadi beberapa fase tidak-memblokir.

```cpp
/*
 * BAB 30 - Program 4: HC-SR04 Non-Blocking State Machine
 *
 * Menggantikan pulseIn() yang memblokir dengan arsitektur State Machine
 * berbasis timer. Loop utama tidak pernah berhenti lebih dari ~12µs!
 *
 * State Machine:
 *   STATE_IDLE      → Menunggu interval pengukuran berikutnya
 *   STATE_TRIG_HIGH → Mengirim pulsa TRIG HIGH (10µs)
 *   STATE_TRIG_LOW  → Menurunkan TRIG ke LOW, tunggu ECHO naik
 *   STATE_WAIT_ECHO → Menunggu ECHO naik
 *   STATE_MEASURING → Menghitung durasi ECHO sampai turun
 *   STATE_DONE      → Proses selesai, simpan ke register
 */

// ── Konfigurasi ───────────────────────────────────────────────────
const int           PIN_TRIG        = 25;
const int           PIN_ECHO        = 26;
const unsigned long INTERVAL_UKUR   = 60UL;   // 60ms antar pengukuran (~16Hz)
const unsigned long TIMEOUT_ECHO_US = 30000UL; // 30ms = ~5 meter

// ── Definisi State Machine ────────────────────────────────────────
enum StateUltrasonik {
  STATE_IDLE,
  STATE_TRIG_HIGH,
  STATE_TRIG_LOW,
  STATE_WAIT_ECHO,
  STATE_MEASURING,
  STATE_DONE
};

StateUltrasonik stateSekarang = STATE_IDLE;

// ── Variabel State Internal ───────────────────────────────────────
unsigned long tStateStart   = 0; // Kapan state ini dimulai
unsigned long tEchoStart    = 0; // Kapan ECHO mulai HIGH

// ── Data Register Global ──────────────────────────────────────────
struct DataUltrasonik {
  float         jarakCm;
  unsigned long durasiUs;
  bool          valid;
  unsigned long tPengukuran; // Timestamp pengukuran terakhir
};

DataUltrasonik jarakTerakhir = {0.0f, 0UL, false, 0UL};

// ── Fungsi State Machine ──────────────────────────────────────────
void updateUltrasonik() {
  unsigned long sekarang = micros(); // Gunakan micros() untuk presisi µs!

  switch (stateSekarang) {
    
    case STATE_IDLE:
      // Tunggu hingga interval berikutnya (menggunakan millis untuk interval besar)
      if ((millis() - jarakTerakhir.tPengukuran) >= INTERVAL_UKUR) {
        digitalWrite(PIN_TRIG, LOW);
        delayMicroseconds(2); // Bersihkan TRIG — ini blocking 2µs, dapat diabaikan!
        stateSekarang = STATE_TRIG_HIGH;
        tStateStart   = micros();
      }
      break;

    case STATE_TRIG_HIGH:
      // Tembakkan TRIG HIGH
      digitalWrite(PIN_TRIG, HIGH);
      stateSekarang = STATE_TRIG_LOW;
      tStateStart   = micros();
      break;

    case STATE_TRIG_LOW:
      // Tunggu 10µs lalu turunkan TRIG
      if (micros() - tStateStart >= 10UL) {
        digitalWrite(PIN_TRIG, LOW);
        stateSekarang = STATE_WAIT_ECHO;
        tStateStart   = micros();
      }
      break;

    case STATE_WAIT_ECHO:
      // Tunggu pin ECHO naik ke HIGH (sensor sedang bersiap)
      if (digitalRead(PIN_ECHO) == HIGH) {
        tEchoStart    = micros();
        stateSekarang = STATE_MEASURING;
      }
      // Timeout: jika ECHO tak naik dalam 5ms, sensor macet!
      if (micros() - tStateStart > 5000UL) {
        jarakTerakhir.valid        = false;
        jarakTerakhir.tPengukuran  = millis();
        stateSekarang              = STATE_IDLE;
      }
      break;

    case STATE_MEASURING:
      // Ukur durasi sampai ECHO turun ke LOW
      if (digitalRead(PIN_ECHO) == LOW) {
        unsigned long durasi = micros() - tEchoStart;
        float jarak          = (durasi * 0.0343f) / 2.0f;

        jarakTerakhir.durasiUs    = durasi;
        jarakTerakhir.jarakCm     = jarak;
        jarakTerakhir.valid       = (jarak >= 2.0f && jarak <= 400.0f);
        jarakTerakhir.tPengukuran = millis();

        stateSekarang = STATE_IDLE;
      }
      // Timeout: jika ECHO tidak turun dalam 30ms, objek terlalu jauh
      if (micros() - tEchoStart > TIMEOUT_ECHO_US) {
        jarakTerakhir.valid       = false;
        jarakTerakhir.tPengukuran = millis();
        stateSekarang             = STATE_IDLE;
      }
      break;

    default:
      stateSekarang = STATE_IDLE;
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.println("══════════════════════════════════════════════════");
  Serial.println(" BAB 30: HC-SR04 Non-Blocking State Machine");
  Serial.println("══════════════════════════════════════════════════");
  Serial.println(" loop() TIDAK PERNAH diblokir > 12µs oleh sensor!");
  Serial.println("══════════════════════════════════════════════════\n");
}

void loop() {
  // State Machine HC-SR04 — berjalan di latar belakang
  updateUltrasonik();

  // Tampilkan hasil saat data baru tersedia
  static unsigned long tTampilTerakhir = 0;
  if (millis() - tTampilTerakhir >= 500UL) {
    tTampilTerakhir = millis();

    if (jarakTerakhir.valid) {
      Serial.printf("[%6lu ms] 📏 Jarak: %5.1f cm  (%4lu µs)\n",
                    jarakTerakhir.tPengukuran,
                    jarakTerakhir.jarakCm,
                    jarakTerakhir.durasiUs);
    } else {
      Serial.printf("[%6lu ms] ⚠️  Di luar jangkauan\n", millis());
    }
  }

  // Logika lain bebas berjalan di sini!
  // Contoh: update OLED, kirim MQTT, baca sensor lain...
}
```

**Contoh output:**
```text
══════════════════════════════════════════════════
 BAB 30: HC-SR04 Non-Blocking State Machine
══════════════════════════════════════════════════
 loop() TIDAK PERNAH diblokir > 12µs oleh sensor!
══════════════════════════════════════════════════

[   521 ms] 📏 Jarak:  28.4 cm  (1658 µs)
[  1021 ms] 📏 Jarak:  28.3 cm  (1651 µs)
[  1521 ms] 📏 Jarak:  65.2 cm  (3802 µs)
[  2021 ms] ⚠️  Di luar jangkauan
```

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
│   Gunakan State Machine (Program 4) untuk sistem IoT kompleks!      │
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

5. **Pengukuran Non-Blocking Tertanam:** Integrasikan Program 4 (State Machine HC-SR04) dengan BAB 29 (Non-Blocking MPU-6050). Jalankan kedua sensor secara **bersamaan** dalam satu `loop()` tanpa saling memblokir. Tampilkan `[Jarak: xx.x cm | Roll: xx.x° | Pitch: xx.x°]` setiap 500ms menggunakan data register global dari masing-masing sensor!

---

*Selanjutnya → [BAB 31: PIR AM312 — Deteksi Gerakan](../BAB-31/README.md)*
