# BAB 37: Relay — Kontrol AC/DC

> ✅ **Prasyarat:** Selesaikan **BAB 08 (Digital Output — LED & Buzzer)**, **BAB 09 (Digital Input — Push Button)**, **BAB 10 (Interrupt & ISR)**, dan **BAB 16 (Non-Blocking Programming)**. Relay pada Bluino Kit terhubung ke pin **Custom** — hubungkan kabel jumper dari pin relay ke GPIO ESP32 sesuai panduan bab ini.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **prinsip kerja relay elektromekanik** sebagai saklar yang dikendalikan sinyal listrik lemah.
- Menjelaskan **mengapa relay membutuhkan transistor driver dan dioda flyback** pada sirkuit kontrol.
- Membedakan kontak relay **NO (Normally Open)**, **NC (Normally Closed)**, dan **COM (Common)**.
- Mengontrol relay secara **non-blocking** menggunakan millis() agar program tetap responsif.
- Membangun **sistem timer relay** untuk otomatisasi jadwal on/off beban.
- Mengintegrasikan relay dengan **sensor PIR** untuk membuat sistem otomatisasi berbasis deteksi gerak.
- Merancang **Finite State Machine (FSM)** untuk mengelola multi-mode operasi relay secara aman.
- Memahami **protokol keselamatan dan batasan** saat menangani tegangan AC/DC tinggi.

---

## 37.1 Relay Elektromekanik — Prinsip Kerja

### Mengapa Relay Diperlukan?

ESP32 beroperasi pada logika 3.3V dan hanya mampu mengalirkan arus **maksimum 12mA per pin GPIO**. Sementara itu, perangkat rumah tangga seperti lampu, kipas angin, atau pompa air membutuhkan tegangan 220V AC atau arus DC yang jauh lebih tinggi. Di sinilah relay berperan sebagai **isolator listrik sekaligus saklar daya tinggi**.

```
MASALAH TANPA RELAY:
  ESP32 IO (3.3V, 12mA) ──X──▶ Lampu 220V/100W?  ❌ TIDAK MUNGKIN!
                              Akan merusak ESP32!

SOLUSI DENGAN RELAY:
  ESP32 IO (3.3V) ─▶ [Driver BC547] ─▶ [Koil Relay 5V] ─▶ [Kontak] ─▶ Lampu 220V
       ↑                                      ↑                 ↑
  Sinyal LEMAH                          Medan Magnet       Saklar KUAT
  (kontrol)                             (aktuasi)          (isolasi galvanik)
```

### Anatomi Relay 5V DC

```
STRUKTUR FISIK RELAY (SRD-05VDC-SL-C — Umum di kit):

          ┌─────────────────────────────────────────────┐
          │             KOIL (Elektromagnet)             │
          │  Coil+  ●────────────────────── ● Coil-     │
          │                                              │
          │  COM  ●─ ─ ─ ─ (anak kontak)  ─ ─ ─        │
          │  NC   ●────────(kontak diam) ─ ─ ─ ─        │
          │  NO   ●                        ─── ●         │
          └─────────────────────────────────────────────┘

  KOIL TIDAK AKTIF (LOW pada pin ESP32):
    COM ──────────── NC  (Normally Closed = TERHUBUNG default)
    COM  - - - - -  NO   (Normally Open = TERPUTUS default)

  KOIL AKTIF (HIGH pada pin ESP32):
    COM  - - - - -  NC   (NC kini TERPUTUS)
    COM ────────── NO    (NO kini TERHUBUNG!)
```

### Glossary Kontak Relay

| Terminal | Nama | Kondisi Koil OFF | Kondisi Koil ON |
|----------|------|------------------|-----------------|
| **COM** | Common | Pivot (titik tengah) | Pivot (titik tengah) |
| **NC** | Normally Closed | ✅ Terhubung ke COM | ❌ Terputus dari COM |
| **NO** | Normally Open | ❌ Terputus dari COM | ✅ Terhubung ke COM |

> 🔑 **Tip Pemula:** Gunakan **NO** untuk beban yang ingin dinyalakan saat relay aktif (normal use case). Gunakan **NC** untuk sistem fail-safe — jika listrik mati atau ESP32 hang, beban tetap ON (contoh: sistem sirkulasi udara pendingin server).

---

## 37.2 Sirkuit Driver Relay di Bluino Kit

Relay tidak dapat langsung dikendalikan oleh GPIO ESP32 karena koil 5V DC membutuhkan arus ~70-100mA — jauh melebihi batas safe 12mA GPIO. Bluino Kit menyelesaikan ini dengan **rangkaian driver BC547**.

```
SKEMATIK DRIVER RELAY BLUINO KIT (NPN Low-Side Switch):

                             5V Supply (VCC)
                                │
       ┌────────────────────────┴────────────────────────┐
       │                                                 │
  [Katoda 1N4148]                                 [Koil Relay]
  [Anoda  1N4148] (Flyback)                              │
       │                                                 │
       └────────────────────────┬────────────────────────┘
                                │
                          Collector (C)
  ESP32 GPIO ──[R 1KΩ]──── Base (B)   Transistor BC547 (NPN)
                          Emitter (E)
                                │
                               GND
  
  ALIRAN ARUS SAAT GPIO HIGH:
    GPIO → R1KΩ → Base BC547 → Transistor SATURASI → Collector SHORT ke GND
    → Arus mengalir: 5V → Koil Relay → Collector BC547 → GND
    → Medan magnet pada koil menarik anak kontak relay

  LED INDIKATOR (Built-in pada Bluino Kit):
    5V ──[R 1KΩ]──[LED] ──── Collector BC547
    Saat relay aktif (Collector = LOW), LED ikut dialiri arus dan menyala!
```

### Mengapa Dioda Flyback 1N4148 Wajib Ada?

```
BAHAYA VOLTAGE SPIKE TANPA DIODA FLYBACK:

  Saat GPIO: HIGH → LOW (Relay DIMATIKAN):
  
  Energi medan magnet di koil yang runtuh seketika akan melepaskan
  LONJAKAN TEGANGAN (spike) ─▶ hingga ratusan volt!
  
  Tanpa dioda: Spike memukul mundur pin Collector BC547
               → BC547 langsung jebol / ESP32 ikut rusak!

  Dengan 1N4148 (Flyback/Freewheeling diode):
  5V──────[Koil]───Collector
     ↑               │
     └───[1N4148]────┘
     (dipasang reverse-bias: Anoda ke Collector, Katoda ke 5V)
  
  Saat koil dimatikan, lonjakan tegangan dialihkan berputar
  di dalam "loop tertutup" koil-dioda hingga habis, 
  sehingga aman dan tidak mencapai transistor.
```

> ⚠️ **Peringatan Desain:** Jangan pernah menghubungkan koil relay **langsung** ke GPIO ESP32 tanpa driver transistor! Ini akan merusak ESP32 secara permanen. Bluino Kit sudah menyediakan driver ini — kamu hanya perlu menghubungkan pin kontrol ke GPIO yang kamu pilih.

---

## 37.3 Koneksi Hardware

### Wiring Relay ke ESP32 (Bluino Kit)

```
KONEKSI KONTROL RELAY (Sisi Logika — AMAN 3.3V/5V):
┌──────────────────────────────────────────────────────────┐
│  Bluino Kit Relay Module      ESP32 DEVKIT V1            │
│  ────────────────────         ──────────────────────     │
│  VCC (5V)         ────────── 5V (Dari pin header kit)   │
│  GND              ────────── GND                         │
│  IN (Kontrol)     ────────── GPIO (rek: IO32 atau IO26) │
└──────────────────────────────────────────────────────────┘

KONEKSI BEBAN (Sisi Daya — HATI-HATI!):
┌──────────────────────────────────────────────────────────┐
│  Skenario: Lampu DC 12V (AMAN untuk latihan)             │
│  ────────────────────────────────────────────────────    │
│  Power Supply 12V(+) ─── COM relay                       │
│  NO relay            ─── Lampu 12V (+)                   │
│  Lampu 12V (-)       ─── Power Supply 12V(-)             │
│                                                           │
│  ⚠️  UNTUK LATIHAN: Gunakan beban DC tegangan rendah     │
│     (LED 5V, lampu DC, kipas DC) — BUKAN listrik AC!     │
│     Pengenalan AC dijelaskan di sub-bab 37.4             │
└──────────────────────────────────────────────────────────┘
```

### Pin yang Direkomendasikan untuk Relay

| Pin GPIO | Status | Catatan |
|----------|--------|---------|
| **IO26** | ✅ Aman | DAC2 — tidak ada konflik, direkomendasikan |
| **IO32** | ✅ Aman | Input/Output biasa, aman |
| **IO33** | ✅ Aman | Input/Output biasa, aman |
| IO0 | ⚠️ Hindari | Strapping pin — ganggu boot |
| IO12 | ⚠️ Hindari | Sudah dipakai WS2812 |
| IO1, IO3 | ❌ Larang | TX0/RX0 UART — jangan dipakai |

> 💡 **Untuk seluruh contoh program di bab ini**, kita akan menggunakan **IO26** sebagai pin kontrol relay.

---

## 37.4 ⚠️ Keselamatan Listrik AC — Baca Sebelum Praktikum!

Jika kamu ingin mengontrol **beban AC (listrik rumah 220V)**, ada protokol keselamatan yang **WAJIB** dipatuhi:

```
PROTOKOL KESELAMATAN LISTRIK AC:

  ✅ DO (LAKUKAN):
     • Gunakan relay yang tertera rating AC yang sesuai (min: 10A/250VAC)
     • Pastikan semua terminal AC terisolasi — tutup dengan heat-shrink tube
     • Selalu matikan listrik SEBELUM memasang/melepas kabel AC
     • Gunakan kabel dengan diameter sesuai rating arus beban
     • Tempatkan relay module dalam wadah/kotak plastik tertutup
     • Uji dengan beban DC lebih dulu sebelum ke AC

  ❌ DON'T (JANGAN):
     • Menyentuh terminal relay yang sedang dialiri listrik AC
     • Menggunakan relay modul yang basah atau berkarat
     • Melebihi rating arus relay (lihat label: 10A/250VAC atau 10A/30VDC)
     • Meninggalkan kabel AC exposed (telanjang tanpa isolasi)
     • Menjalankan kode tanpa pengujian logika sisi DC terlebih dahulu

  ⚡ RATING RELAY UMUM (SRD-05VDC-SL-C):
     Coil: 5V DC, ~80mA
     Contact: 10A @ 250V AC  /  10A @ 30V DC
     → JANGAN lewati batas ini! Relay akan hangus/kebakaran.
```

> 🔥 **Rekomendasi Praktikum:** Lakukan semua pengembangan dan pengujian kode menggunakan **beban DC tegangan rendah** terlebih dahulu (LED, kipas 5V, lampu DC). Setelah logika kontrol terbukti benar, baru hubungkan ke beban AC di bawah pengawasan instruktur berpengalaman.

---

## 37.5 Program 1: Blink Relay — Hello World Relay

Program paling fundamental: mengedipkan relay secara non-blocking, seperti LED Blink tapi untuk beban daya tinggi.

```cpp
/*
 * BAB 37 - Program 1: Relay Blink — Hello World Relay
 *
 * Tujuan:
 *   Memahami cara mengontrol relay secara non-blocking menggunakan millis().
 *   Relay aktif 2 detik, mati 2 detik, berulang terus.
 *
 * Hardware:
 *   Relay Module   → IO26 (pin kontrol, via jumper ke header Custom)
 *   Beban uji      → COM + NO relay (disarankan: LED 5V atau kipas DC kecil)
 *
 * Logika:
 *   IO26 HIGH → Transistor BC547 jenuh → Koil relay aktif → NO terhubung ke COM
 *   IO26 LOW  → Transistor BC547 off   → Koil relay mati  → NO terputus
 */

// ── Konfigurasi ──────────────────────────────────────────────────────────
#define RELAY_PIN   26   // Pin kontrol relay (hubungkan via kabel jumper)

// Durasi on/off relay
const unsigned long RELAY_ON_MS  = 2000UL;  // Relay ON selama 2 detik
const unsigned long RELAY_OFF_MS = 2000UL;  // Relay OFF selama 2 detik

// ── State Non-Blocking ───────────────────────────────────────────────────
bool relayOn      = false;
unsigned long tRelay = 0;

// ── Helper Functions ─────────────────────────────────────────────────────
void relaySetState(bool aktif) {
  relayOn = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  Serial.printf("[RELAY] %s  (millis=%lu)\n", aktif ? "ON  ✔" : "OFF ✘", millis());
}

void setup() {
  Serial.begin(115200);
  digitalWrite(RELAY_PIN, LOW); // Set state aman sebelum pinMode untuk mencegah glitch
  pinMode(RELAY_PIN, OUTPUT);

  // WAJIB: Pastikan relay mati saat startup!
  // Beban tidak boleh aktif saat ESP32 baru dinyalakan/di-reset
  relaySetState(false);

  Serial.println("=== BAB 37 Program 1: Relay Blink ===");
  Serial.println("Relay akan ON 2 detik, OFF 2 detik, berulang...");
  Serial.println("---------------------------------------------");
}

void loop() {
  unsigned long now = millis();

  if (relayOn) {
    // Relay sedang ON — tunggu RELAY_ON_MS lalu matikan
    if (now - tRelay >= RELAY_ON_MS) {
      tRelay = now;
      relaySetState(false);
    }
  } else {
    // Relay sedang OFF — tunggu RELAY_OFF_MS lalu nyalakan
    if (now - tRelay >= RELAY_OFF_MS) {
      tRelay = now;
      relaySetState(true);
    }
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 37 Program 1: Relay Blink ===
Relay akan ON 2 detik, OFF 2 detik, berulang...
---------------------------------------------
[RELAY] OFF ✘  (millis=0)
[RELAY] ON  ✔  (millis=2001)
[RELAY] OFF ✘  (millis=4003)
[RELAY] ON  ✔  (millis=6004)
```

> 💡 **Mengapa `relaySetState(false)` di setup() WAJIB?** Saat ESP32 baru di-reset atau dinyalakan, kondisi pin GPIO adalah **undefined** (bisa HIGH atau LOW tergantung kapasitor internal). Jika relay terhubung ke pompa air atau alat berbahaya, kondisi ON yang tidak disengaja bisa fatal. **Selalu inisialisasi relay ke OFF di awal program!**

---

## 37.6 Program 2: Relay Timer — Sistem Penjadwal Otomatis

Program ini mensimulasikan sistem otomatisasi rumah sederhana: relay otomatis ON dan OFF sesuai jadwal yang telah ditentukan, seperti timer lampu taman atau sistem irigasi otomatis.

```cpp
/*
 * BAB 37 - Program 2: Relay Timer — Sistem Penjadwal Otomatis
 *
 * Fitur:
 *   ▶ Relay ON selama durasi tertentu (misalnya: pompa air 10 detik)
 *   ▶ Relay OFF selama periode istirahat (misalnya: 30 detik)
 *   ▶ Penghitung siklus tersisa di Serial Monitor
 *   ▶ Batas maksimum siklus — relay berhenti otomatis (safety)
 *   ▶ Sepenuhnya non-blocking berbasis millis()
 *
 * Hardware:
 *   Relay Module → IO26
 *   Beban        → COM + NO relay
 */

#define RELAY_PIN   26

// ── Konfigurasi Jadwal ───────────────────────────────────────────────────
const unsigned long DURASI_ON_MS   =  5000UL;  // ON selama 5 detik
const unsigned long DURASI_OFF_MS  = 10000UL;  // OFF selama 10 detik (istirahat)
const int           MAX_SIKLUS     =     5;    // Berhenti setelah 5 siklus ON

// ── State ────────────────────────────────────────────────────────────────
enum TimerState { TIMER_IDLE, TIMER_ON, TIMER_OFF, TIMER_DONE };
TimerState timerState = TIMER_IDLE;
unsigned long tTimer  = 0;
int  siklusKe    = 0;

// ── Antarmuka Serial Monitor ─────────────────────────────────────────────
void printStatus(const char* pesan, bool relayStatus) {
  unsigned long detik = millis() / 1000;
  Serial.printf("[%4lus] Siklus %d/%d | Relay: %-4s | %s\n",
    detik, siklusKe, MAX_SIKLUS,
    relayStatus ? "ON" : "OFF",
    pesan);
}

// ── Kontrol Relay ────────────────────────────────────────────────────────
void relayOn_() {
  digitalWrite(RELAY_PIN, HIGH);
}
void relayOff_() {
  digitalWrite(RELAY_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  relayOff_(); // Set state relay mati sebelum pinMode untuk mencegah glitch
  pinMode(RELAY_PIN, OUTPUT);

  Serial.println("=== BAB 37 Program 2: Relay Timer Otomatis ===");
  Serial.printf("Jadwal: ON %lu dtk | OFF %lu dtk | Maks %d siklus\n",
    DURASI_ON_MS/1000, DURASI_OFF_MS/1000, MAX_SIKLUS);
  Serial.println("-----------------------------------------------");

  // Langsung mulai siklus pertama
  timerState = TIMER_ON;
  siklusKe   = 1;
  tTimer     = millis();
  relayOn_();
  printStatus("Memulai siklus pertama", true);
}

void loop() {
  unsigned long now = millis();

  switch (timerState) {

    case TIMER_ON:
      // Relay sedang ON — tunggu durasi ON selesai
      if (now - tTimer >= DURASI_ON_MS) {
        tTimer = now;
        relayOff_();
        printStatus("Durasi ON selesai → beralih ke OFF", false);

        if (siklusKe >= MAX_SIKLUS) {
          timerState = TIMER_DONE;
          Serial.println("\n✅ Semua siklus selesai. Program selesai.");
        } else {
          timerState = TIMER_OFF;
        }
      }
      break;

    case TIMER_OFF:
      // Relay sedang OFF — tunggu durasi OFF (jeda/istirahat)
      if (now - tTimer >= DURASI_OFF_MS) {
        tTimer = now;
        siklusKe++;
        timerState = TIMER_ON;
        relayOn_();
        printStatus("Istirahat selesai → memulai siklus berikutnya", true);
      }
      break;

    case TIMER_DONE:
      // Program selesai — tidak melakukan apa-apa, relay tetap OFF
      // (Bisa ditambah: masuk sleep mode, kirim notifikasi, dll)
      break;

    case TIMER_IDLE:
    default:
      break;
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 37 Program 2: Relay Timer Otomatis ===
Jadwal: ON 5 dtk | OFF 10 dtk | Maks 5 siklus
-----------------------------------------------
[   0s] Siklus 1/5 | Relay: ON   | Memulai siklus pertama
[   5s] Siklus 1/5 | Relay: OFF  | Durasi ON selesai → beralih ke OFF
[  15s] Siklus 2/5 | Relay: ON   | Istirahat selesai → memulai siklus berikutnya
[  20s] Siklus 2/5 | Relay: OFF  | Durasi ON selesai → beralih ke OFF
...
[  65s] Siklus 5/5 | Relay: OFF  | Durasi ON selesai → beralih ke OFF

✅ Semua siklus selesai. Program selesai.
```

---

## 37.7 Program 3: Relay Dikendalikan Tombol dengan Debounce

Program ini membangun sistem toggle relay yang responsif: satu klik tombol = relayOn, klik lagi = relayOff. Implementasi debounce yang kokoh mencegah bouncing sinyal mekanikal tombol menyebabkan toggling yang tidak diinginkan.

```cpp
/*
 * BAB 37 - Program 3: Toggle Relay via Push Button (Debounce Profesional)
 *
 * Fitur:
 *   ▶ Klik tombol → Toggle relay (ON/OFF bergantian)
 *   ▶ Debounce software berbasis millis() (tidak ada delay!)
 *   ▶ Proteksi: Batas waktu relay ON maksimum (safety timeout)
 *   ▶ Status lengkap di Serial Monitor setiap perubahan state
 *
 * Hardware:
 *   Relay Module   → IO26 (via jumper Custom)
 *   Push Button    → IO32 (via jumper Custom, resistor pull-down sudah ada di kit)
 *
 * Logika Tombol:
 *   Tombol push-down pada Bluino Kit: LOW saat dilepas, HIGH saat ditekan.
 */

#define RELAY_PIN    26
#define BTN_PIN      32

// ── Konfigurasi ──────────────────────────────────────────────────────────
const unsigned long DEBOUNCE_MS     =  50UL;    // Debounce 50ms (standar)
const unsigned long SAFETY_TIMEOUT  = 60000UL;  // Relay auto-OFF setelah 60 detik

// ── State Relay ──────────────────────────────────────────────────────────
bool relayState   = false;
unsigned long tRelayOn = 0; // Waktu relay terakhir dinyalakan

// ── State Tombol (Non-Blocking Debounce) ─────────────────────────────────
bool btnLastPhysical = false; // Bacaan fisik terakhir
bool btnStable       = false; // State yang sudah terdebounce/stabil
bool btnJustPressed  = false; // Flag: TRUE satu kali saat rising-edge terdeteksi
unsigned long tBtnEdge = 0;   // Waktu terdeteksi perubahan fisik

void updateButton(unsigned long now) {
  btnJustPressed = false;
  bool bacaanSekarang = (digitalRead(BTN_PIN) == HIGH);

  // Deteksi perubahan fisik
  if (bacaanSekarang != btnLastPhysical) {
    tBtnEdge = now; // Catat waktu mulai bouncing
  }

  // Konfirmasi state baru hanya jika sudah stabil selama DEBOUNCE_MS
  if ((now - tBtnEdge) >= DEBOUNCE_MS) {
    if (bacaanSekarang != btnStable) {
      btnStable = bacaanSekarang;
      // Rising edge: tombol baru saja ditekan (LOW → HIGH)
      if (btnStable == true) {
        btnJustPressed = true;
      }
    }
  }
  btnLastPhysical = bacaanSekarang;
}

// ── Kontrol Relay ────────────────────────────────────────────────────────
void setRelay(bool aktif, unsigned long now) {
  relayState = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);

  if (aktif) {
    tRelayOn = now; // Catat kapan relay mulai ON (untuk safety timeout)
    Serial.printf("[RELAY] ON  | Safety timeout: %lu dtk\n", SAFETY_TIMEOUT/1000);
  } else {
    unsigned long durasi = (now - tRelayOn) / 1000;
    Serial.printf("[RELAY] OFF | Relay aktif selama: %lu dtk\n", durasi);
  }
}

void setup() {
  Serial.begin(115200);
  digitalWrite(RELAY_PIN, LOW); // Inisialisasi aman (selalu sebelum pinMode)
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);
  relayState = false;

  Serial.println("=== BAB 37 Program 3: Toggle Relay via Tombol ===");
  Serial.println("Tekan tombol IO32 untuk ON/OFF relay");
  Serial.printf("Safety timeout: relay auto-OFF setelah %lu detik\n",
    SAFETY_TIMEOUT / 1000);
  Serial.println("--------------------------------------------------");
}

void loop() {
  unsigned long now = millis();

  // ── Update state tombol ───────────────────────────────────────────────
  updateButton(now);

  // ── Toggle relay saat tombol ditekan ─────────────────────────────────
  if (btnJustPressed) {
    setRelay(!relayState, now); // Balik state: ON→OFF atau OFF→ON
  }

  // ── Safety Timeout: Auto-OFF jika relay terlalu lama ON ──────────────
  if (relayState && (now - tRelayOn >= SAFETY_TIMEOUT)) {
    Serial.println("[SAFETY] Timeout! Relay di-OFF paksa oleh safety system.");
    setRelay(false, now);
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 37 Program 3: Toggle Relay via Tombol ===
Tekan tombol IO32 untuk ON/OFF relay
Safety timeout: relay auto-OFF setelah 60 detik
--------------------------------------------------
[RELAY] ON  | Safety timeout: 60 dtk
[RELAY] OFF | Relay aktif selama: 3 dtk
[RELAY] ON  | Safety timeout: 60 dtk
[SAFETY] Timeout! Relay di-OFF paksa oleh safety system.
[RELAY] OFF | Relay aktif selama: 60 dtk
```

> 💡 **Safety Timeout adalah Praktik Industri:** Pada sistem otomasi nyata — terutama yang mengontrol pompa, heater, atau motor — selalu tambahkan batas waktu maksimum relay ON. Ini mencegah kerusakan perangkat jika software hang atau tombol fisik macet dalam kondisi ditekan.

---

## 37.8 Program 4: Automasasi Sensor PIR — Lampu Otomatis Gerak

Program ini adalah implementasi IoT klasik yang sangat berguna: lampu menyala otomatis saat ada gerakan, dan mati otomatis setelah beberapa detik tidak ada gerakan — persis seperti sistem lampu koridor gedung.

```cpp
/*
 * BAB 37 - Program 4: Otomasi PIR — Lampu Menyala saat Gerak Terdeteksi
 *
 * Alur kerja:
 *   1. PIR mendeteksi gerakan → Relay ON (lampu nyala)
 *   2. Timer mulai menghitung mundur
 *   3. Jika tidak ada gerakan baru sebelum timer habis → Relay OFF
 *   4. Jika gerakan terdeteksi lagi → Timer direset (lampu tetap nyala)
 *
 * Hardware:
 *   PIR AM312     → IO33 (via jumper Custom, output digital HIGH saat deteksi)
 *   Relay Module  → IO26 (via jumper Custom)
 *   Beban         → COM + NO relay (lampu/LED)
 *
 * Catatan:
 *   PIR AM312 memiliki waktu warm-up ~30 detik setelah dinyalakan.
 *   Biarkan beberapa saat sebelum mulai menggunakannya.
 */

#define RELAY_PIN  26
#define PIR_PIN    33

// ── Konfigurasi ──────────────────────────────────────────────────────────
const unsigned long HOLD_TIME_MS = 10000UL;  // Relay tetap ON 10 dtk setelah gerakan
const unsigned long PIR_READ_MS  =   200UL;  // Baca PIR setiap 200ms (cukup responsif)
const unsigned long WARMUP_MS    = 30000UL;  // Waktu warm-up sensor PIR AM312

// ── State ────────────────────────────────────────────────────────────────
enum PirState { PIR_WARMUP, PIR_STANDBY, PIR_TRIGGERED };
PirState pirState   = PIR_WARMUP;

bool  relayActive   = false;
unsigned long tLastMotion = 0;  // Waktu terakhir gerakan terdeteksi
unsigned long tPirRead    = 0;  // Timer baca sensor
unsigned long tWarmup     = 0;  // Timer warm-up

// ── Kontrol Relay ────────────────────────────────────────────────────────
void setRelay(bool aktif) {
  if (relayActive == aktif) return; // Tidak ada perubahan, skip
  relayActive = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  Serial.printf("[RELAY] %s\n", aktif ? "ON  — Gerakan terdeteksi!" : "OFF — Tidak ada gerakan.");
}

void setup() {
  Serial.begin(115200);
  digitalWrite(RELAY_PIN, LOW); // Kunci relay OFF sebelum menjadi output
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  tWarmup = millis();

  Serial.println("=== BAB 37 Program 4: Lampu Otomatis PIR ===");
  Serial.println("Menunggu PIR warm-up (30 detik)...");
  Serial.println("---------------------------------------------");
}

void loop() {
  unsigned long now = millis();

  // ── Fase 1: Warm-up PIR setelah power-on ─────────────────────────────
  if (pirState == PIR_WARMUP) {
    if (now - tWarmup >= WARMUP_MS) {
      pirState = PIR_STANDBY;
      Serial.println("✅ PIR siap! Sistem monitoring gerak AKTIF.");
      Serial.printf("   Hold time: %.1f detik setelah gerakan terakhir\n",
        HOLD_TIME_MS / 1000.0);
    } else {
      // Tampilkan countdown setiap 5 detik
      static unsigned long tPrintWarmup = 0;
      if (now - tPrintWarmup >= 5000UL) {
        tPrintWarmup = now;
        unsigned long sisaDetik = (WARMUP_MS - (now - tWarmup)) / 1000;
        Serial.printf("   Warm-up: %lu detik tersisa...\n", sisaDetik);
      }
    }
    return; // Jangan baca PIR selama warm-up
  }

  // ── Fase 2: Baca PIR secara periodik ─────────────────────────────────
  if (now - tPirRead >= PIR_READ_MS) {
    tPirRead = now;

    bool adaGerak = (digitalRead(PIR_PIN) == HIGH);

    if (adaGerak) {
      // Gerakan terdeteksi: nyalakan relay dan reset timer hold
      if (!relayActive) {
        setRelay(true);
      }
      tLastMotion = now; // Perbarui waktu gerakan terakhir
    }
  }

  // ── Fase 3: Timer hold — matikan relay jika tidak ada gerakan ─────────
  if (relayActive && (now - tLastMotion >= HOLD_TIME_MS)) {
    setRelay(false);
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 37 Program 4: Lampu Otomatis PIR ===
Menunggu PIR warm-up (30 detik)...
---------------------------------------------
   Warm-up: 25 detik tersisa...
   Warm-up: 20 detik tersisa...
   ...
✅ PIR siap! Sistem monitoring gerak AKTIF.
   Hold time: 10.0 detik setelah gerakan terakhir
[RELAY] ON  — Gerakan terdeteksi!
[RELAY] OFF — Tidak ada gerakan.
[RELAY] ON  — Gerakan terdeteksi!
[RELAY] OFF — Tidak ada gerakan.
```

> 💡 **Optimasi Hold Time:** Nilai `HOLD_TIME_MS = 10000UL` bisa disesuaikan sesuai kebutuhan. Untuk lorong pendek: 5 detik. Untuk ruang kerja: 5 menit (300000UL). Untuk gudang: 30 menit. Ubah satu konstanta dan seluruh perilaku sistem berubah — inilah keindahan desain berbasis konstanta.

---

## 37.9 Program 5: FSM Multi-Mode Relay

Program puncak BAB 37 ini membangun **Finite State Machine (FSM) berlapis empat mode operasi** yang bisa dinavigasi via tombol: Manual Toggle, Auto Timer, Auto PIR, dan Safe Lock (relay terkunci OFF). Ini adalah pola desain industri nyata untuk sistem kontrol aktuator.

```cpp
/*
 * BAB 37 - Program 5: FSM Multi-Mode Relay
 *
 * Empat mode operasi relay:
 *   MODE_MANUAL  → Toggle relay via tombol (ON/OFF manual)
 *   MODE_TIMER   → Relay ON selama 5 dtk, OFF 10 dtk, berulang
 *   MODE_PIR     → Relay ON saat PIR deteksi gerak, auto-OFF 10 dtk
 *   MODE_LOCK    → Relay terkunci OFF (maintenance/safety lock)
 *
 * Navigasi mode:
 *   Tekan tombol panjang (>1 detik) → Ganti mode (maju)
 *   Tekan tombol pendek (<1 detik)  → Aksi dalam mode aktif
 *
 * Hardware:
 *   Relay Module → IO26 (Custom)
 *   PIR AM312    → IO33 (Custom)
 *   Push Button  → IO32 (Custom, pull-down sudah ada di kit)
 */

#include <Arduino.h>

#define RELAY_PIN  26
#define PIR_PIN    33
#define BTN_PIN    32

// ── Konfigurasi ──────────────────────────────────────────────────────────
const unsigned long DEBOUNCE_MS       =    50UL;
const unsigned long LONG_PRESS_MS     =  1000UL; // Tekan > 1 detik = long press
const unsigned long TIMER_ON_MS       =  5000UL;
const unsigned long TIMER_OFF_MS      = 10000UL;
const unsigned long PIR_HOLD_MS       = 10000UL;
const unsigned long RELAY_SAFETY_MS   = 300000UL; // 5 menit safety timeout global

// ── Definisi Mode FSM ────────────────────────────────────────────────────
enum RelayMode {
  MODE_MANUAL,
  MODE_TIMER,
  MODE_PIR,
  MODE_LOCK,
  MODE_COUNT
};

const char* namaMode[] = {
  "MANUAL   ", "TIMER    ", "PIR AUTO ", "SAFE LOCK"
};

RelayMode currentMode = MODE_MANUAL;

// ── State Relay ──────────────────────────────────────────────────────────
bool relayState  = false;
unsigned long tRelayOn = 0;

void setRelay(bool aktif, unsigned long now) {
  if (relayState == aktif) return;
  relayState = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  if (aktif) tRelayOn = now;
  Serial.printf("[%s] RELAY %s\n", namaMode[currentMode],
    aktif ? "ON" : "OFF");
}

// ── Tombol Non-Blocking dengan Deteksi Long Press ─────────────────────────
bool  btnPhysical     = false;
bool  btnStable       = false;
bool  btnShortPress   = false;
bool  btnLongPress    = false;
bool  longPressFired  = false;
unsigned long tBtnEdge    = 0;
unsigned long tBtnPressed = 0;

void updateButton(unsigned long now) {
  btnShortPress = false;
  btnLongPress  = false;

  bool bacaan = (digitalRead(BTN_PIN) == HIGH);

  // Debounce
  if (bacaan != btnPhysical) tBtnEdge = now;
  if ((now - tBtnEdge) >= DEBOUNCE_MS) {
    if (bacaan != btnStable) {
      btnStable = bacaan;
      if (btnStable) {
        // Rising edge: tombol mulai ditekan
        tBtnPressed  = now;
        longPressFired = false;
      } else {
        // Falling edge: tombol dilepas
        unsigned long durasi = now - tBtnPressed;
        if (!longPressFired) {
          // Long press belum difire, ini short press
          if (durasi >= DEBOUNCE_MS && durasi < LONG_PRESS_MS) {
            btnShortPress = true;
          }
        }
      }
    }
  }

  // Deteksi long press saat tombol masih ditekan
  if (btnStable && !longPressFired) {
    if ((now - tBtnPressed) >= LONG_PRESS_MS) {
      btnLongPress   = true;
      longPressFired = true;
    }
  }

  btnPhysical = bacaan;
}

// ── Timer State (untuk MODE_TIMER) ───────────────────────────────────────
enum TimerPhase { TP_ON, TP_OFF };
TimerPhase timerPhase = TP_OFF;
unsigned long tTimer = 0;

void resetTimerMode(unsigned long now) {
  // Mulai selalu dengan fase OFF untuk memberi jeda aman pada koil relay
  // setelah dimatikan secara paksa oleh switchMode()
  timerPhase = TP_OFF;
  tTimer = now;
}

void updateTimerMode(unsigned long now) {
  if (timerPhase == TP_ON) {
    if (now - tTimer >= TIMER_ON_MS) {
      tTimer = now;
      timerPhase = TP_OFF;
      setRelay(false, now);
    }
  } else {
    if (now - tTimer >= TIMER_OFF_MS) {
      tTimer = now;
      timerPhase = TP_ON;
      setRelay(true, now);
    }
  }
}

// ── PIR State (untuk MODE_PIR) ───────────────────────────────────────────
unsigned long tLastMotion = 0;
unsigned long tPirRead    = 0;

void updatePirMode(unsigned long now) {
  if (now - tPirRead >= 200UL) {
    tPirRead = now;
    if (digitalRead(PIR_PIN) == HIGH) {
      if (!relayState) setRelay(true, now);
      tLastMotion = now;
    }
  }
  if (relayState && (now - tLastMotion >= PIR_HOLD_MS)) {
    setRelay(false, now);
  }
}

// ── Transisi Mode ─────────────────────────────────────────────────────────
void switchMode(RelayMode newMode, unsigned long now) {
  // Matikan relay terlebih dahulu saat ganti mode (safety)
  setRelay(false, now);

  currentMode = newMode;
  Serial.printf("\n[MODE] Berganti ke: %s\n", namaMode[newMode]);

  switch (newMode) {
    case MODE_MANUAL:
      Serial.println("  Tekan pendek = Toggle relay ON/OFF");
      break;
    case MODE_TIMER:
      Serial.printf("  Auto: ON %lus / OFF %lus berulang\n",
        TIMER_ON_MS/1000, TIMER_OFF_MS/1000);
      resetTimerMode(now);
      // Dihapus: setRelay(true, now) - berbahaya secara induktif jika dipanggil tepat setelah setRelay(false)
      break;
    case MODE_PIR:
      Serial.printf("  PIR: Relay ON saat gerak, OFF %lus kemudian\n",
        PIR_HOLD_MS/1000);
      tLastMotion = 0;
      break;
    case MODE_LOCK:
      Serial.println("  LOCKED: Relay dikunci OFF. Tekan panjang untuk keluar.");
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  digitalWrite(RELAY_PIN, LOW); // Pre-init state untuk mencegah trigger relay
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BTN_PIN, INPUT);
  relayState = false;

  Serial.println("=== BAB 37 Program 5: FSM Multi-Mode Relay ===");
  Serial.println("TEKAN LAMA (>1 dtk): Ganti mode");
  Serial.println("TEKAN PENDEK       : Aksi dalam mode");
  Serial.println("Mode awal: MANUAL");
  Serial.println("----------------------------------------------");
}

void loop() {
  unsigned long now = millis();

  updateButton(now);

  // ── Navigasi Mode via Long Press ──────────────────────────────────────
  if (btnLongPress) {
    RelayMode modeBerikut = (RelayMode)((currentMode + 1) % MODE_COUNT);
    switchMode(modeBerikut, now);
  }

  // ── Eksekusi Mode Aktif ───────────────────────────────────────────────
  switch (currentMode) {

    case MODE_MANUAL:
      if (btnShortPress) {
        setRelay(!relayState, now);
      }
      break;

    case MODE_TIMER:
      updateTimerMode(now);
      break;

    case MODE_PIR:
      updatePirMode(now);
      break;

    case MODE_LOCK:
      // Paksa relay mati terus-menerus saat di mode LOCK
      if (relayState) {
        setRelay(false, now);
        Serial.println("[LOCK] Percobaan aktivasi ditolak!");
      }
      break;
  }

  // ── Global Safety Timeout (semua mode) ───────────────────────────────
  if (relayState && (now - tRelayOn >= RELAY_SAFETY_MS)) {
    Serial.println("[SAFETY] Global timeout! Relay auto-OFF.");
    setRelay(false, now);
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 37 Program 5: FSM Multi-Mode Relay ===
TEKAN LAMA (>1 dtk): Ganti mode
TEKAN PENDEK       : Aksi dalam mode
Mode awal: MANUAL
----------------------------------------------
[MANUAL   ] RELAY ON
[MANUAL   ] RELAY OFF

[MODE] Berganti ke: TIMER    
  Auto: ON 5s / OFF 10s berulang
[TIMER    ] RELAY ON
[TIMER    ] RELAY OFF
...

[MODE] Berganti ke: PIR AUTO 
  PIR: Relay ON saat gerak, OFF 10s kemudian
[PIR AUTO ] RELAY ON
[PIR AUTO ] RELAY OFF

[MODE] Berganti ke: SAFE LOCK
  LOCKED: Relay dikunci OFF. Tekan panjang untuk keluar.
```

---

## 37.10 Tips & Panduan Troubleshooting

### 1. Relay berbunyi klik tapi beban tidak menyala?
```text
Penyebab paling umum:

A. Kabel beban salah terminal
   → Pastikan kabel beban terhubung ke COM + NO (bukan NC)
     jika ingin beban menyala saat relay aktif
   → Atau ke COM + NC jika ingin kebalikannya

B. Rating relay terlampaui
   → Cek label relay: 10A/250VAC atau yang tertera
   → Jika beban melebihi rating, relay akan overheating
     dan kontak gosong

C. Kabel longgar
   → Periksa semua sambungan terminal sekrup pada relay
   → Kencangkan dengan obeng minus kecil

D. Beban DC polaritas terbalik
   → Cek (+) dan (-) pada beban DC Anda
```

### 2. Relay aktif/tidak aktif sendiri tanpa perintah?
```text
Penyebab:

A. GPIO floating (tidak ada pull-down/pull-up)
   → Tambahkan initState yang jelas di setup():
     digitalWrite(RELAY_PIN, LOW);  // WAJIB!

B. Noise listrik dari beban AC atau motor DC
   → Pasang kapasitor 100nF (0.1µF) antara pin kontrol 
     dan GND di dekat driver relay

C. Kode tidak menginisialisasi relay
   → Selalu panggil digitalWrite(RELAY_PIN, LOW) di setup()
     sebelum pinMode() — atau tepat setelah pinMode()

D. Ground terpisah
   → Pastikan GND ESP32 dan GND modul relay terhubung!
     Sinyal kontrol tidak akan berfungsi jika GND berbeda.
```

### 3. ESP32 reset saat relay diaktifkan?
```text
Penyebab: Voltage drop pada jalur 5V karena koil relay
menarik arus besar secara tiba-tiba.

Solusi:
  A. Tambahkan kapasitor 100µF (elco) di pin 5V modul relay
     → Menstabilkan tegangan saat lonjakan arus koil

  B. Pastikan power supply USB atau adaptor mampu menyuplai
     arus: ESP32 (~200mA) + Relay (~80mA) + Beban = total

  C. Gunakan power supply terpisah untuk relay jika arus
     total mendekati batas adaptor

  D. Jika menggunakan baterai: lonjakan arus menyebabkan
     undervoltage → tambahkan kapasitor bulk 1000µF
```

### 4. Relay ON terus padahal kode sudah benar?
```text
Kemungkinan penyebab:

A. Relay type Active-LOW (beberapa modul relay bekerja terbalik!)
   → Ciri: HIGH = OFF, LOW = ON
   → Solusi: Balik logika atau ganti ke:
     #define RELAY_ON  LOW
     #define RELAY_OFF HIGH
     digitalWrite(RELAY_PIN, RELAY_ON); // untuk menyalakan

B. Debounce tombol tidak bekerja
   → Periksa resistor pull-down 10KΩ di kit sudah terhubung

C. Loop kode membaca ulang nilai yang benar tapi hardcoded
   → Cari bagian kode yang secara eksplisit set HIGH terus
```

### 5. Bagaimana cara mengontrol beban AC dengan aman?
```text
Prosedur keselamatan AC step-by-step:

  1. Matikan MCB/saklar utama sebelum memasang kabel
  2. Pastikan modul relay dalam kotak plastik tertutup
  3. Gunakan kabel NYM 2.5mm² untuk beban >1000W
  4. Isolasi semua terminal terbuka dengan heat-shrink tube
  5. Uji logika kontrol dulu dengan beban DC kecil
  6. Baru setelah logika terbukti benar → sambung ke AC
  7. Selalu gunakan fuse/sekering sesuai rating beban

  KATA-KATA KUNCI: Jika ragu, JANGAN sentuh kabel AC yang
  sedang bertegangan. Nyawa lebih berharga dari proyek IoT!
```

---

## 37.11 Ringkasan

```
┌──────────────────────────────────────────────────────────────────────┐
│                 RINGKASAN BAB 37 — Relay Kontrol AC/DC               │
├──────────────────────────────────────────────────────────────────────┤
│ HARDWARE & PRINSIP KERJA:                                             │
│   Relay = Saklar elektromekanik — koil magnet menggerakkan kontak    │
│   NC = Normally Closed: terhubung saat relay OFF (fail-safe)         │
│   NO = Normally Open: terhubung saat relay ON (normal use case)      │
│   Driver BC547 → Mengalirkan arus koil 80mA tanpa membebani GPIO     │
│   Flyback 1N4148 → Melindungi BC547 dari voltage spike koil          │
│   IO26 rekomendasi pin kontrol relay di Bluino Kit                   │
│                                                                       │
│ KESELAMATAN (WAJIB DIPATUHI):                                        │
│   ✅ Selalu inisialisasi relay OFF di setup() sebelum digunakan      │
│   ✅ Implementasikan safety timeout untuk relay yang aktif lama      │
│   ✅ Uji dengan beban DC rendah sebelum ke beban AC 220V             │
│   ✅ Jangan sentuh terminal AC saat bertegangan                      │
│   ✅ Perhatikan rating relay: 10A/250VAC — jangan dilampaui!         │
│                                                                       │
│ POLA DESAIN:                                                          │
│   ✅ Non-blocking dengan millis() — tidak ada delay()!               │
│   ✅ FSM untuk multi-mode relay yang bersih & mudah dikembangkan     │
│   ✅ Debounce tombol via millis() dengan deteksi short/long press    │
│   ✅ Safety timeout global sebagai lapisan proteksi terakhir         │
│   ✅ Fungsi setRelay() terpusat — satu titik kontrol, mudah debug    │
│                                                                       │
│ CATATAN SENSOR PIR AM312:                                             │
│   ▶ Warm-up ~30 detik setelah power-on                               │
│   ▶ Output HIGH saat deteksi (aktif HIGH)                            │
│   ▶ Baca setiap 200ms sudah cukup — tidak perlu lebih sering         │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 37.12 Latihan

1. **Relay Flasher Variabel:** Modifikasi Program 1 agar durasi ON dan OFF relay dikontrol oleh **dua buah potensiometer** (ADC). Satu pot mengontrol durasi ON (0.5–10 detik), pot lain mengontrol durasi OFF (1–30 detik). Tampilkan nilai durasi di Serial Monitor setiap kali relay berganti state.

2. **Scheduler 24-Jam Simulasi:** Kembangkan Program 2 agar relay mengikuti jadwal berbasis `millis()` yang mensimulasikan waktu nyata. Misalnya: relay ON dari "jam 06:00" hingga "jam 07:00" dan "jam 18:00" hingga "jam 23:00". Gunakan `millis() / 1000` sebagai detik, kalibrasi jadwal sesuai kecepatan simulasi kamu. (Petunjuk: 1 jam simulasi = 60 detik real time, 24 jam simulasi = 24 menit)

3. **Alarm Relay Bertingkat:** Integrasikan sensor **DHT11** (IO27) dengan relay. Buat tiga level respons: Suhu <28°C = relay OFF, 28-33°C = relay berkedip 3 detik ON/3 detik OFF (mode waspada), >33°C = relay ON terus-menerus (mode darurat). Tampilkan level alarm aktif di Serial Monitor beserta nilai suhu real-time.

4. **Relay dengan Konfirmasi Tombol:** Modifikasi Program 3 agar sebelum relay ON, sistem menampilkan pesan "Konfirmasi? Tekan lagi dalam 5 detik." Relay baru aktif jika tombol ditekan kedua kali dalam window 5 detik. Jika tidak ada konfirmasi, batalkan dan kembali ke standby. Ini adalah pola **confirmation dialog** untuk mencegah aktivasi tidak disengaja.

5. **Mode Relay Berbasis LDR (Sensor Cahaya):** Buat sistem **lampu malam otomatis**: jika ADC LDR menunjukkan nilai di atas threshold gelap (misal >2500), relay ON (lampu menyala). Jika nilai LDR di bawah threshold (ruangan terang, <800), relay OFF. Tambahkan histeresis 200 unit pada threshold untuk mencegah relay berkedip saat nilai ADC di sekitar batas.

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 36: RGB LED WS2812 (NeoPixel)](../BAB-36/README.md)*

*Selanjutnya → [BAB 38: Servo Motor](../BAB-38/README.md)*
