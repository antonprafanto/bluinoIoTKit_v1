# BAB 31: PIR AM312 — Deteksi Gerakan Inframerah

> ✅ **Prasyarat:** Selesaikan **BAB 10 (Interupsi & ISR)** dan **BAB 16 (Non-Blocking Programming)**. PIR AM312 di Kit Bluino menggunakan pin **Custom** — hubungkan dengan kabel jumper ke GPIO pilihan kamu secara manual!

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menjelaskan prinsip kerja sensor PIR (*Passive Infrared*) menggunakan efek piroelektrik dan lensa Fresnel.
- Membedakan AM312 dari sensor PIR konvensional (HC-SR501) dan memilih yang tepat untuk setiap skenario.
- Menangani masalah *false positive*, *warm-up time*, dan *re-trigger* yang menjadi jebakan paling umum pemrogram.
- Membangun sistem alarm gerak berbasis **State Machine** yang responsif dan andal.
- Mengintegrasikan PIR dengan Hardware Interrupts untuk arsitektur *zero-latency* yang sesungguhnya.
- Merancang sistem keamanan terpadu yang menggabungkan PIR + Buzzer + LED dalam pola non-blocking.

---

## 31.1 Mengenal PIR AM312 — Mata Inframerah Mikro

### Apa itu Sensor PIR?

**PIR** (*Passive Infrared Sensor*) adalah detektor gerak yang bekerja dengan menangkap radiasi inframerah panas dari tubuh manusia atau hewan. Kata **"Passive"** adalah kunci paling penting di sini — sensor ini **tidak memancarkan energi apapun** ke lingkungan. Ia hanya diam dan *mendengarkan* energi panas yang sudah ada di ruangan!

Ini berbeda total dari HC-SR04 (Bab 30) yang aktif memancarkan gelombang suara. PIR ibarat seorang pengintai yang duduk diam di sudut gelap tanpa senter, hanya mengandalkan panas yang dipancarkan mangsa.

### Fisika Piroelektrik

Inti dari sensor PIR adalah **elemen piroelektrik** — material kristal khusus (biasanya LiTaO₃ atau keramik PZT) yang memiliki sifat luar biasa: **menghasilkan tegangan listrik ketika suhunya berubah**.

```
PRINSIP KERJA INTERNAL PIR:

Tubuh manusia pada suhu 37°C memancarkan:
  → Radiasi inframerah dengan panjang gelombang 8–14 µm (mid-infrared)
  → Ini adalah "tanda tangan panas" yang unik — sempurna untuk dideteksi!

Elemen Sensor (Dual Pyroelectric):
  ┌──────────────────────────────────────┐
  │   [ Elemen A ]    [ Elemen B ]       │
  │        ↑                ↑            │
  │   Panas masuk      Panas masuk       │
  │   dari zona 1     dari zona 2        │
  │        └────────────────┘            │
  │           Dipasang SALING BERLAWANAN │
  │           (differential / anti-fase) │
  └──────────────────────────────────────┘

Kondisi DIAM (tidak ada gerakan):
  → Elemen A dan B menerima panas lingkungan yang sama
  → Tegangan A – Tegangan B = 0V → Output: LOW

Kondisi BERGERAK (manusia melintas):
  → Manusia masuk zona Elemen A → Tegangan A naik!
  → B masih menerima panas ambient biasa
  → Tegangan A – Tegangan B ≠ 0V → Output: HIGH!
  → Manusia pindah ke zona B → Polaritas berbalik → Sinyal waveform terdeteksi!
```

### Peran Lensa Fresnel

Elemen piroelektrik telanjang hanya bisa mendeteksi perubahan panas dari jarak beberapa sentimeter. Untuk menjangkau 3–5 meter, digunakan **lensa Fresnel** — sebuah inovasi optik yang brilian.

```
LENSA FRESNEL vs LENSA KONVENSIONAL:

Lensa Biasa:               Lensa Fresnel:
   ╱  ╲                     ─┤├┤├┤├─
  ╱    ╲                    flat, ringan!
 ╱      ╲
─────────                   Prinsip: Dipecah menjadi
Tebal, berat, mahal!        cincin-cincin konsentris tipis.
                            Setiap cincin = satu "segmen lensa"
                            yang memfokuskan IR dari sudut berbeda!

POLA ZONA DETEKSI PIR (tampak atas):

             [SENSOR]
                │
    ┌───────────┼───────────┐
    │           │           │
   ███  zone  ███  zone  ███  ← zona AKTIF (sensitif)
   ___ insens ___  insens ___  ← zona MATI (tidak sensitif)
   ███  zone  ███  zone  ███
    │           │           │
    └───────────┴───────────┘

Manusia HARUS melintasi zona aktif→mati→aktif
agar deteksi terjadi! Gerakan LURUS MENUJU sensor = sulit terdeteksi!
```

### AM312 vs HC-SR501 — Kapan Pilih Yang Mana?

Kit Bluino menggunakan **AM312** — modul mini yang berbeda dari PIR konvensional HC-SR501. Berikut perbandingannya:

```
┌──────────────────────────────────────────────────────────────────┐
│              AM312 vs HC-SR501 — Perbandingan                    │
├─────────────────────────┬─────────────────┬──────────────────────┤
│ Fitur                   │ AM312 (Kit kita) │ HC-SR501 (Umum)      │
├─────────────────────────┼─────────────────┼──────────────────────┤
│ Ukuran PCB              │ ~10×8 mm (MINI!) │ ~33×24 mm (besar)    │
│ Tegangan Operasi        │ 2.7V – 12V       │ 4.5V – 20V           │
│ Konsumsi Arus Idle      │ < 0.1 mA         │ ~50 µA (BICSIOS IC)  │
│ Kompatibel 3.3V langsung│ ✅ YA            │ ❌ Tidak (5V ideal)  │
│ Jangkauan Deteksi       │ 3 – 5 meter      │ 3 – 7 meter          │
│ Sudut Deteksi           │ ≤ 100°           │ ≤ 120° (adj.)        │
│ Delay Time              │ ~2 detik (tetap) │ 3–300 detik (adj.)   │
│ Trigger Mode            │ Repeatable       │ Single / Repeatable  │
│ Potensiometer Adj.      │ ❌ Tidak ada     │ ✅ 2 buah (Sx & Tx)  │
│ Aplikasi Ideal          │ Wearable, IoT    │ Home Security, Alarm │
│                         │ Battery project  │ AC-powered project   │
└─────────────────────────┴─────────────────┴──────────────────────┘
```

> 💡 **Keuntungan AM312 di Kit Bluino:** Karena beroperasi langsung pada 3.3V dan konsumsi arusnya sangat rendah (< 0.1 mA), AM312 adalah sensor yang sempurna untuk proyek IoT bertenaga baterai. ESP32 pun bisa langsung membacanya tanpa buffer tegangan!

---

## 31.2 Spesifikasi Hardware & Wiring Kit Bluino

### Spesifikasi AM312

```
┌─────────────────────────────────────────────────────────────────┐
│             SPESIFIKASI AM312 — KIT BLUINO                      │
├──────────────────────────┬──────────────────────────────────────┤
│ Parameter                │ Nilai                                │
├──────────────────────────┼──────────────────────────────────────┤
│ Koneksi ke Kit           │ Pin CUSTOM (kabel jumper manual)     │
│ Pin Output               │ 1 pin digital (Active HIGH)          │
│ Tegangan Operasi         │ 2.7V – 12V DC                        │
│ Level Output HIGH        │ ~3.0V–3.3V (kompatibel ESP32!)       │
│ Level Output LOW         │ 0V                                   │
│ Arus Idle                │ < 0.1 mA (sangat hemat daya!)        │
│ Jangkauan Deteksi        │ 3 – 5 meter                          │
│ Sudut Kerucut            │ ≤ 100° horizontal                    │
│ Delay Time (Hold)        │ ~2 detik setelah gerakan terakhir    │
│ Trigger Mode             │ Repeatable (auto-extend jika masih   │
│                          │ ada gerakan saat delay berlangsung)  │
│ Suhu Operasi             │ -20°C hingga +60°C                   │
│ Dimensi PCB              │ ~10mm × 8mm                          │
└──────────────────────────┴──────────────────────────────────────┘
```

### Diagram Wiring

```
┌──────────────────────────────────────────────────────────┐
│           WIRING AM312 ↔ ESP32 KIT BLUINO                │
├──────────────────────────────────────────────────────────┤
│                                                          │
│   AM312 PIN         KABEL JUMPER    ESP32 (rekomendasi)  │
│   ─────────         ────────────    ──────────────────── │
│   VCC  ────────────── Merah ──────── 3.3V (Header kit)   │
│   GND  ────────────── Hitam ──────── GND  (Header kit)   │
│   OUT  ────────────── Kuning ─────── IO34 (Input Only)   │
│                                                          │
│   💡 Mengapa IO34?                                       │
│   IO34-IO39 adalah pin "Input Only" ESP32 yang IDEAL     │
│   untuk sensor pasif seperti PIR karena:                 │
│   - Tidak bisa dikonfigurasi sebagai Output (aman!)      │
│   - Mendukung Hardware Interrupt (attachInterrupt)       │
│   - Tidak ada GPIOState konflik saat boot                │
│                                                          │
│   ⚠️  JANGAN gunakan IO0, IO2, IO12, IO15 untuk PIR!    │
│   Pin strapping tersebut bisa menyebabkan boot failure   │
│   jika kondisi HIGH/LOW saat ESP32 dinyalakan!           │
└──────────────────────────────────────────────────────────┘
```

> ⚠️ **Tidak Perlu Resistor Pull-up/Pull-down!** AM312 sudah memiliki rangkaian output terintegrasi yang menghasilkan sinyal digital bersih. Menambahkan resistor eksternal justru bisa mempengaruhi timing sinyal.

---

## 31.3 Instalasi Library

PIR AM312 **tidak memerlukan library eksternal sama sekali!** Ini adalah sensor paling sederhana dari sisi interfacing — hanya satu pin digital yang dibaca HIGH atau LOW.

```
Tidak perlu menginstal library apapun!
Cukup pastikan Board ESP32 sudah terpasang di Board Manager.

Fungsi yang digunakan:
  √ pinMode()        — Konfigurasi pin sebagai INPUT
  √ digitalRead()    — Membaca status H/L sensor
  √ attachInterrupt()— Mendaftarkan ISR (Program 4)
  √ millis()         — Timing non-blocking
```

---

## 31.4 Perangkap Terbesar #1: Warm-Up Time!

Sebelum masuk ke kode program, ada satu konsep yang **WAJIB** kamu pahami. Ini adalah jebakan yang mengecoh 99% pemrogram PIR pertama kali.

**Setelah dinyalakan, PIR AM312 membutuhkan waktu "pemanasan" (Warm-Up) selama 30–60 detik sebelum pembacaannya stabil!**

Mengapa? Karena elemen piroelektrik di dalamnya perlu waktu untuk menyesuaikan diri dengan suhu lingkungan ambient saat ini. Selama proses ini, sensor akan mengeluarkan sinyal HIGH palsu (*false positive*) secara acak seperti orang yang baru bangun tidur dan belum sepenuhnya sadar!

```
TIMELINE WARM-UP PIR:

Boot ESP32
    │
    ▼
[0 – 30 detik]: ZONA BAHAYA!
    │   PIR mengeluarkan sinyal HIGH/LOW acak
    │   ← Jangan percaya pembacaan di sini!
    │   ← Ini bukan BUG — ini FISIKA!
    │
    ▼
[~30 detik]: Kalibrasi Selesai
    │   Elemen piroelektrik sudah merekam "suhu dasar" ruangan
    │   Pembacaan kini dapat dipercaya!
    │
    ▼
[> 30 detik]: ZONA AMAN ✅
    │   HIGH = Ada gerakan (benar)
    │   LOW  = Tidak ada gerakan (benar)
```

> 💡 **Praktik Profesional:** Selalu tambahkan blokade `delay(30000)` atau timer warm-up non-blocking di `setup()` sebelum mulai memproses data PIR! Tampilkan pesan bertahitung mundur ke Serial Monitor untuk memberi tahu pengguna.

---

## 31.5 Program 1: Deteksi Gerak Dasar (Halo PIR!)

Program paling sederhana untuk membuktikan sensor bekerja, sekaligus mendemonstrasikan tantangan warm-up yang benar.

```cpp
/*
 * BAB 31 - Program 1: PIR AM312 Deteksi Gerak Dasar
 *
 * Memperkenalkan pembacaan sensor PIR digital yang paling fundamental.
 * Program ini menampilkan deteksi gerak real-time pada Serial Monitor,
 * TERMASUK fase warm-up yang benar — bukan "tutorial copas" yang langsung
 * baca sensor dan heran kenapa terus-menerus detect padahal tidak ada orang!
 *
 * Wiring (Kit Bluino — Koneksi MANUAL via Kabel Jumper):
 *   AM312 VCC  → 3.3V (Header daya kit)
 *   AM312 GND  → GND
 *   AM312 OUT  → IO34 (Input Only GPIO — ideal untuk sensor pasif)
 */

const int PIN_PIR         = 34;
const unsigned long WAKTU_WARMUP_MS = 30000UL; // 30 detik warm-up

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT); // TIDAK perlu pull-up/pull-down — AM312 sudah punya output driver!

  Serial.println("══════════════════════════════════════════════");
  Serial.println("  BAB 31: PIR AM312 — Deteksi Gerak Dasar");
  Serial.println("══════════════════════════════════════════════");
  Serial.println("  ⚠️  Fase WARM-UP dimulai. Harap diam & tunggu!");
  Serial.println("──────────────────────────────────────────────");

  // ── Fase Warm-Up Wajib! ─────────────────────────────────────
  // Countdown visual agar pengguna tidak panik saat melihat output
  // "GERAK!" yang palsu berulang-ulang pada detik-detik awal.
  for (int hitungan = 30; hitungan > 0; hitungan--) {
    Serial.printf("  🕐 Kalibrasi sensor... %2d detik lagi\n", hitungan);
    delay(1000);
  }

  Serial.println("──────────────────────────────────────────────");
  Serial.println("  ✅ Sensor siap! Gerakkan tangan di depannya.");
  Serial.println("══════════════════════════════════════════════\n");
}

void loop() {
  int statusPIR = digitalRead(PIN_PIR);

  // HIGH = Ada perubahan inframerah terdeteksi (gerakan!)
  if (statusPIR == HIGH) {
    Serial.println("🚨 GERAK TERDETEKSI!");
  } else {
    Serial.println("   Tidak ada gerakan...");
  }

  // Polling setiap 100ms — lebih dari cukup karena delay sensor ~2 detik
  delay(100);
}
```

**Contoh output:**
```text
══════════════════════════════════════════════
  BAB 31: PIR AM312 — Deteksi Gerak Dasar
══════════════════════════════════════════════
  ⚠️  Fase WARM-UP dimulai. Harap diam & tunggu!
──────────────────────────────────────────────
  🕐 Kalibrasi sensor... 30 detik lagi
  🕐 Kalibrasi sensor... 29 detik lagi
  ...
  🕐 Kalibrasi sensor...  1 detik lagi
──────────────────────────────────────────────
  ✅ Sensor siap! Gerakkan tangan di depannya.
══════════════════════════════════════════════

   Tidak ada gerakan...
   Tidak ada gerakan...
🚨 GERAK TERDETEKSI!
🚨 GERAK TERDETEKSI!        ← AM312 menahan HIGH selama ±2 detik
🚨 GERAK TERDETEKSI!
   Tidak ada gerakan...     ← PIN kembali LOW setelah 2 detik idle
```

---

## 31.6 Perangkap Terbesar #2: Memahami Re-Trigger & Hold Time

Sebelum ke Program 2, pahami dulu perilaku AM312 yang sering disalahpahami:

```
PERILAKU "REPEATABLE TRIGGER" AM312:

Skenario: Seseorang bergerak terus-menerus di depan sensor

Waktu →    0s   1s   2s   3s   4s   5s   6s   7s   8s
           │    │    │    │    │    │    │    │    │

Gerakan:   [──gerakan──] diam  [──gerakan──] [diam panjang]
OUT pin:   [────HIGH────────────────────────] [────LOW────]
                                                 ↑
                            Baru jadi LOW 2 detik setelah gerakan TERAKHIR!

Penjelasan:
  - Setiap gerakan baru "mereset" timer 2 detik dari awal
  - Selama ada gerakan, pin OUT TIDAK pernah turun ke LOW!
  - Ini adalah perilaku BENAR dan BERGUNA untuk sistem keamanan
  - Tapi ini juga berarti: event "MULAI BERGERAK" hanya terjadi sekali
    meskipun orang bergerak terus!
```

Ini adalah **perbedaan fundamental** antara mendeteksi **STATUS** (sedang bergerak?) vs mendeteksi **EVENT** (mulai bergerak? berhenti bergerak?). Program 2 akan mengajarkan cara membedakan keduanya.

---

## 31.7 Program 2: Deteksi Event (Edge Detection) Non-Blocking

Program ini menggunakan **teknik Edge Detection** — mendeteksi kapan tepatnya sinyal **naik** (rising edge = mulai bergerak) dan kapan **turun** (falling edge = berhenti bergerak). Tanpa `delay()` sama sekali!

```cpp
/*
 * BAB 31 - Program 2: PIR AM312 — Event Detection Non-Blocking
 *
 * Level naik dari: Polling status → Deteksi perubahan (edge detection)
 *
 * Perbedaan kunci:
 *   Program 1: Cetak setiap loop() — spam!
 *   Program 2: Cetak HANYA saat ada PERUBAHAN state — elegan!
 *
 * Juga menghitung statistik: berapa kali deteksi terjadi sejak boot.
 */

const int PIN_PIR              = 34;
const unsigned long WAKTU_WARMUP_MS = 30000UL;

// ── State Tracking ────────────────────────────────────────────────
int  statusPIRSebelumnya = LOW;  // Simpan state sebelumnya untuk edge detection
int  totalDeteksi        = 0;    // Penghitung statistik
unsigned long waktuDeteksiTerakhir = 0;

// Warm-up non-blocking
bool sudahWarmup = false;
unsigned long tWarmupMulai = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);

  tWarmupMulai = millis(); // Catat waktu mulai warm-up

  Serial.println("══════════════════════════════════════════════════");
  Serial.println("  BAB 31: PIR AM312 — Event Detection Non-Blocking");
  Serial.println("══════════════════════════════════════════════════");
  Serial.println("  ⚠️  Warm-up 30 detik dimulai — jangan bergerak!");
  Serial.println("──────────────────────────────────────────────────\n");
}

void loop() {
  unsigned long sekarang = millis();

  // ── FASE WARM-UP NON-BLOCKING ──────────────────────────────────
  // Gunakan millis() bukan delay() agar program tetap "hidup"
  if (!sudahWarmup) {
    unsigned long terlewat    = sekarang - tWarmupMulai;
    unsigned long sisaDetik   = (WAKTU_WARMUP_MS - terlewat) / 1000;

    // Tampilkan countdown setiap 1 detik
    static unsigned long tCetakTerakhir = 0;
    if (sekarang - tCetakTerakhir >= 1000UL) {
      tCetakTerakhir = sekarang;
      if (terlewat < WAKTU_WARMUP_MS) {
        Serial.printf("  🕐 Kalibrasi... %2lu detik lagi\n", sisaDetik + 1);
      }
    }

    if (terlewat >= WAKTU_WARMUP_MS) {
      sudahWarmup = true;
      Serial.println("\n  ✅ Sensor siap! Deteksi aktif.");
      Serial.println("──────────────────────────────────────────────────");
      Serial.println("  [Waktu ms]  [Event]          [Total Deteksi]");
      Serial.println("──────────────────────────────────────────────────");
    }
    return; // Jangan proses sensor selama warm-up
  }

  // ── BACA STATUS SENSOR ─────────────────────────────────────────
  int statusSekarang = digitalRead(PIN_PIR);

  // ── DETEKSI RISING EDGE (LOW → HIGH) ─────────────────────────
  // Terjadi hanya SEKALI saat gerakan pertama kali dimulai
  if (statusSekarang == HIGH && statusPIRSebelumnya == LOW) {
    totalDeteksi++;
    waktuDeteksiTerakhir = sekarang;

    Serial.printf("  [%7lu ms] 🚨 MULAI BERGERAK!  [Deteksi ke-%d]\n",
                  sekarang, totalDeteksi);
  }

  // ── DETEKSI FALLING EDGE (HIGH → LOW) ────────────────────────
  // Terjadi SETELAH periode hold 2 detik berakhir, saat semua gerak berhenti
  if (statusSekarang == LOW && statusPIRSebelumnya == HIGH) {
    unsigned long durasiAktif = sekarang - waktuDeteksiTerakhir;

    Serial.printf("  [%7lu ms] ✅ Berhenti.      [Durasi aktif: %lu ms]\n",
                  sekarang, durasiAktif);
  }

  // Simpan state saat ini untuk dibandingkan di iterasi berikutnya
  statusPIRSebelumnya = statusSekarang;

  delay(20); // Polling 50Hz — cukup cepat untuk PIR yang responnya 2 detik
}
```

**Contoh output:**
```text
══════════════════════════════════════════════════
  BAB 31: PIR AM312 — Event Detection Non-Blocking
══════════════════════════════════════════════════
  ⚠️  Warm-up 30 detik dimulai — jangan bergerak!
──────────────────────────────────────────────────
  🕐 Kalibrasi... 30 detik lagi
  ...
  🕐 Kalibrasi...  1 detik lagi

  ✅ Sensor siap! Deteksi aktif.
──────────────────────────────────────────────────
  [Waktu ms]  [Event]          [Total Deteksi]
──────────────────────────────────────────────────
  [ 31245 ms] 🚨 MULAI BERGERAK!  [Deteksi ke-1]
  [ 33298 ms] ✅ Berhenti.      [Durasi aktif: 2053 ms]
  [ 40112 ms] 🚨 MULAI BERGERAK!  [Deteksi ke-2]
  [ 42187 ms] ✅ Berhenti.      [Durasi aktif: 2075 ms]
```

---

## 31.8 Program 3: Sistem Alarm Gerak dengan State Machine

Sekarang kita bangun sesuatu yang nyata: **Sistem alarm gerak 4-state** dengan respons visual (LED) dan audio (Buzzer). Ini adalah arsitektur yang digunakan di sistem keamanan rumah sesungguhnya!

State Machine kita memiliki 4 keadaan:
- `WARMUP` — Fase pemanasan awal saat sensor baru dinyalakan
- `IDLE` — Tidak ada gerakan, sistem siap memantau
- `TERDETEKSI` — Gerakan terdeteksi! Alarm berbunyi & LED menyala
- `COOLDOWN` — Gerakan berhenti, alarm mati, tunggu jeda sebelum kembali ke IDLE

```cpp
/*
 * BAB 31 - Program 3: Alarm Gerak — Finite State Machine (FSM)
 *
 * Arsitektur State Machine menghilangkan semua logika kondisi if/else
 * yang tumpang tindih dan menggantinya dengan model yang jelas dan terstruktur.
 * Ini adalah standar industri dalam desain firmware embedded!
 *
 * States:
 *   IDLE       → Siap mendeteksi, LED hijau berkedip pelan tanda "on guard"
 *   TERDETEKSI → Alarm aktif: LED merah ON + buzzer berbunyi berpola
 *   COOLDOWN   → Jeda 3 detik setelah gerak berhenti sebelum kembali IDLE
 *
 * Konfigurasi Pin (sesuaikan dengan jumper kamu):
 *   PIR OUT → IO34
 *   LED (merah sebagai alarm)  → IO2  (built-in LED kit, active HIGH)
 *   Buzzer  → IO25 (Active Buzzer Kit Bluino)
 */

// ── Konfigurasi Pin ───────────────────────────────────────────────
const int PIN_PIR    = 34;
const int PIN_LED    = 2;   // Gunakan LED bawaan atau LED eksternal
const int PIN_BUZZER = 25;

// ── Definisi State Machine ────────────────────────────────────────
enum State {
  WARMUP,      // Fase pemanasan sensor — abaikan semua input
  IDLE,        // Siap dan memantau — tidak ada ancaman
  TERDETEKSI,  // Gerakan aktif terdeteksi — alarm berbunyi!
  COOLDOWN     // Jeda singkat setelah alarm — reset ke IDLE
};

State stateSekarang = WARMUP;
unsigned long tMasukState = 0; // Catat kapan masuk state saat ini

// ── Parameter Waktu ───────────────────────────────────────────────
const unsigned long DURASI_WARMUP_MS   = 30000UL; // 30 detik warm-up
const unsigned long DURASI_COOLDOWN_MS =  3000UL; //  3 detik cooldown
const unsigned long INTERVAL_BEEP_MS   =   200UL; // 200ms beep alarm (5Hz)
const unsigned long INTERVAL_IDLE_MS   =  1000UL; // LED idle berkedip 1 detik

// ── Variabel State ────────────────────────────────────────────────
int    statusPIRLama      = LOW;
bool   buzzerAktif        = false;
unsigned long tBuzzerToggle = 0;
int    totalAlarm         = 0;

// ── Fungsi Transisi State ─────────────────────────────────────────
void pindahState(State stateTarget) {
  stateSekarang = stateTarget;
  tMasukState   = millis();

  // Reset output saat pindah state
  digitalWrite(PIN_LED,    LOW);
  digitalWrite(PIN_BUZZER, LOW);
  buzzerAktif = false;
}

// ── Fungsi Non-Blocking Buzzer ────────────────────────────────────
void updateBuzzerAlarm() {
  unsigned long sekarang = millis();
  if (sekarang - tBuzzerToggle >= INTERVAL_BEEP_MS) {
    tBuzzerToggle = sekarang;
    buzzerAktif   = !buzzerAktif;
    digitalWrite(PIN_BUZZER, buzzerAktif ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_PIR,    INPUT);
  pinMode(PIN_LED,    OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LED,    LOW);
  digitalWrite(PIN_BUZZER, LOW);

  pindahState(WARMUP);

  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("  BAB 31: Sistem Alarm Gerak — State Machine");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("  Memulai Warm-Up PIR (30 detik)...");
  Serial.println("──────────────────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();
  int statusPIR = digitalRead(PIN_PIR);
  bool adaGerakan = (statusPIR == HIGH);

  // ================================================================
  // MESIN STATE — Setiap state punya LOGIKA dan TRANSISI masing-masing
  // ================================================================

  switch (stateSekarang) {

    // ── STATE: WARMUP ─────────────────────────────────────────────
    case WARMUP: {
      // Countdown non-blocking setiap 1 detik
      static unsigned long tCetakWarmup = 0;
      if (sekarang - tCetakWarmup >= 1000UL) {
        tCetakWarmup = sekarang;
        unsigned long sisa = (DURASI_WARMUP_MS - (sekarang - tMasukState)) / 1000;
        if (sekarang - tMasukState < DURASI_WARMUP_MS)
          Serial.printf("  🕐 Warm-Up: %2lu detik lagi...\n", sisa + 1);
      }
      // Kedipkan LED cepat sbg indikator warm-up
      digitalWrite(PIN_LED, ((sekarang / 200) % 2 == 0) ? HIGH : LOW);

      if (sekarang - tMasukState >= DURASI_WARMUP_MS) {
        Serial.println("  ✅ Sistem AKTIF — Memantau gerakan!\n");
        pindahState(IDLE);
      }
      break;
    }

    // ── STATE: IDLE ───────────────────────────────────────────────
    case IDLE: {
      // LED berkedip pelan sebagai tanda "on guard"
      digitalWrite(PIN_LED, ((sekarang / INTERVAL_IDLE_MS) % 2 == 0) ? HIGH : LOW);

      // TRANSISI → TERDETEKSI: Rising edge (gerakan baru muncul)
      if (adaGerakan && statusPIRLama == LOW) {
        totalAlarm++;
        Serial.printf("[%7lu ms] 🚨 ALARM #%d — GERAKAN TERDETEKSI!\n",
                      sekarang, totalAlarm);
        pindahState(TERDETEKSI);
      }
      break;
    }

    // ── STATE: TERDETEKSI ─────────────────────────────────────────
    case TERDETEKSI: {
      // LED ON terus + buzzer berpola cepat
      digitalWrite(PIN_LED, HIGH);
      updateBuzzerAlarm();

      // TRANSISI → COOLDOWN: Falling edge (gerakan berhenti = pin turun LOW)
      if (!adaGerakan && statusPIRLama == HIGH) {
        unsigned long durasiAktif = sekarang - tMasukState;
        Serial.printf("[%7lu ms] ✋ Gerakan berhenti. Durasi: %lu ms. Masuk Cooldown...\n",
                      sekarang, durasiAktif);
        pindahState(COOLDOWN);
      }
      break;
    }

    // ── STATE: COOLDOWN ───────────────────────────────────────────
    case COOLDOWN: {
      // LED berkedip super cepat, buzzer off — fase "absen"
      digitalWrite(PIN_LED, ((sekarang / 250) % 2 == 0) ? HIGH : LOW);
      digitalWrite(PIN_BUZZER, LOW);

      // TRANSISI → TERDETEKSI: Gerakan muncul lagi saat cooldown!
      if (adaGerakan) {
        Serial.printf("[%7lu ms] 🚨 Gerakan muncul kembali! Alarm ulang!\n", sekarang);
        pindahState(TERDETEKSI);
        break;
      }

      // TRANSISI → IDLE: Cooldown selesai, sistem siap kembali
      if (sekarang - tMasukState >= DURASI_COOLDOWN_MS) {
        Serial.printf("[%7lu ms] 💤 Sistem kembali ke mode IDLE — memantau.\n\n", sekarang);
        pindahState(IDLE);
      }
      break;
    }
  }

  statusPIRLama = statusPIR; // Update state PIR untuk edge detection berikutnya
  delay(20);
}
```

**Contoh output:**
```text
══════════════════════════════════════════════════════
  BAB 31: Sistem Alarm Gerak — State Machine
══════════════════════════════════════════════════════
  Memulai Warm-Up PIR (30 detik)...
──────────────────────────────────────────────────────
  🕐 Warm-Up: 30 detik lagi...
  ...
  ✅ Sistem AKTIF — Memantau gerakan!

[ 30122 ms] 🚨 ALARM #1 — GERAKAN TERDETEKSI!
[ 34210 ms] ✋ Gerakan berhenti. Durasi: 4088 ms. Masuk Cooldown...
[ 37211 ms] 💤 Sistem kembali ke mode IDLE — memantau.

[ 45503 ms] 🚨 ALARM #2 — GERAKAN TERDETEKSI!
[ 47605 ms] ✋ Gerakan berhenti. Durasi: 2102 ms. Masuk Cooldown...
[ 47982 ms] 🚨 Gerakan muncul kembali! Alarm ulang!
[ 50100 ms] ✋ Gerakan berhenti. Durasi: 2118 ms. Masuk Cooldown...
[ 53100 ms] 💤 Sistem kembali ke mode IDLE — memantau.
```

---

## 31.9 Program 4: Arsitektur Hardware Interrupt (Zero-Latency)

Program 3 masih menggunakan polling di dalam `loop()`. Meski non-blocking, ada potensi *latency* beberapa milidetik jika `loop()` sedang mengerjakan tugas lain. Untuk sistem keamanan kelas tinggi yang harus merespons dalam **mikrodetik**, kita gunakan **Hardware Interrupt**.

```cpp
/*
 * BAB 31 - Program 4: PIR AM312 — Hardware Interrupt (Zero-Latency)
 *
 * Arsitektur ini menjamin bahwa setiap perubahan state PIR (naik/turun)
 * SELALU direkam secara instan, bahkan jika loop() sedang mengerjakan
 * operasi berat seperti komunikasi WiFi, penulisan SD Card, atau
 * animasi NeoPixel yang memblokir sebentar.
 *
 * Perbedaan dengan Program 3:
 *   Program 3: loop() polling setiap 20ms — max latency 20ms
 *   Program 4: ISR dipanggil hardware dalam ~1µs — zero latency!
 *
 * Cocok untuk: Alarm keamanan, logging presisi waktu kejadian,
 *              sistem multi-sensor berbeban CPU tinggi.
 */

// ── Konfigurasi Pin ───────────────────────────────────────────────
const int PIN_PIR    = 34; // IO34 mendukung interrupt!
const int PIN_LED    = 2;
const int PIN_BUZZER = 25;

// ── Variabel Volatile untuk ISR ───────────────────────────────────
// WAJIB volatile: Memberitahu compiler bahwa variabel ini bisa berubah
// kapan saja di "belakang layar" oleh fungsi Interrupt, jangan di-cache!
volatile bool  eventGerakMulai   = false; // Flag: rising edge terjadi
volatile bool  eventGerakBerhenti = false; // Flag: falling edge terjadi
volatile unsigned long tRisingEdge = 0;   // Timestamp rising edge (µs)
volatile unsigned long tFallingEdge = 0;  // Timestamp falling edge (µs)

// ── ISR — Interupsi PIR (dipanggil otomatis hardware!) ────────────
// IRAM_ATTR: Tempatkan ISR di IRAM tercepat ESP32 untuk latensi minimal!
void IRAM_ATTR isrPIR() {
  if (digitalRead(PIN_PIR) == HIGH) {
    // ── RISING EDGE: Gerakan mulai ─────────────────────────────
    tRisingEdge      = micros();
    eventGerakMulai  = true;
  } else {
    // ── FALLING EDGE: Gerakan berhenti ─────────────────────────
    tFallingEdge      = micros();
    eventGerakBerhenti = true;
  }
  // ⚡ ISR selesai dalam < 1µs — langsung kembali ke program utama!
}

// ── Warm-up ───────────────────────────────────────────────────────
// (Menggunakan blocking delay karena dilakukan HANYA SEKALI saat boot)

// ── Statistik ─────────────────────────────────────────────────────
int totalEvent = 0;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_PIR,    INPUT);
  pinMode(PIN_LED,    OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LED,    LOW);
  digitalWrite(PIN_BUZZER, LOW);

  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("  BAB 31: PIR AM312 — Hardware Interrupt Architecture");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("  ⚠️  Warm-Up 30 detik — harap diam!");
  Serial.println("──────────────────────────────────────────────────────");

  // Warm-up blocking (ini satu-satunya delay yang diizinkan di program ini!)
  for (int i = 30; i > 0; i--) {
    Serial.printf("  🕐 Kalibrasi: %2d detik lagi\n", i);
    delay(1000);
  }

  // ── Daftarkan Interrupt SETELAH warm-up! ─────────────────────
  // Jangan daftarkan interrupt sebelum warm-up selesai —
  // false-positive selama warm-up akan men-spam ISR kita!
  attachInterrupt(digitalPinToInterrupt(PIN_PIR), isrPIR, CHANGE);

  Serial.println("  ✅ Interrupt terdaftar! Sistem memantau...\n");
  Serial.println("──────────────────────────────────────────────────────");
  Serial.println("  [Waktu µs]         [Event]");
  Serial.println("──────────────────────────────────────────────────────");
}

void loop() {
  // ── Proses event dari ISR (Critical Section) ──────────────────
  // Baca dan clear flag ISR dalam daerah aman (noInterrupts)
  // agar tidak terjadi race condition saat ISR menimpa flag yang sedang dibaca!
  
  bool prosesNaik   = false;
  bool prosesTurun  = false;
  
  // Gunakan 'static' agar tNaik bertahan antar iterasi loop()
  // Ini krusial agar durasi dihitung dari awal gerak, bukan dari 0!
  static unsigned long tNaik = 0; 
  unsigned long tTurun = 0;

  noInterrupts(); // Matikan interrupt sementara (kritis!)
  if (eventGerakMulai) {
    prosesNaik        = true;
    tNaik             = tRisingEdge;
    eventGerakMulai   = false; // Clear flag
  }
  if (eventGerakBerhenti) {
    prosesTurun       = true;
    tTurun            = tFallingEdge;
    eventGerakBerhenti = false; // Clear flag
  }
  interrupts(); // Nyalakan kembali interrupt

  // ── Tindak lanjut event di luar critical section ──────────────
  if (prosesNaik) {
    totalEvent++;
    digitalWrite(PIN_LED,    HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
    Serial.printf("  [%10lu µs] 🚨 GERAK MULAI!   [Event #%d]\n",
                  tNaik, totalEvent);
  }

  if (prosesTurun) {
    unsigned long durasiUs = tTurun - tNaik;
    digitalWrite(PIN_LED,    LOW);
    digitalWrite(PIN_BUZZER, LOW);
    Serial.printf("  [%10lu µs] ✅ GERAK BERHENTI [Durasi: %.2f detik]\n",
                  tTurun, durasiUs / 1000000.0f);
  }

  // ── Tugas lain bisa berjalan di sini tanpa mempengaruhi deteksi ─
  // Contoh: Komunikasi WiFi, update OLED, baca sensor lain, dsb.
  // Interrupt HC-SR04 dari Bab 30 pun bisa berjalan di sini bersamaan!
}
```

**Keunggulan Arsitektur Interrupt:**
```text
Skenario: ESP32 sedang mengirim data ke WiFi (operasi ~50ms)

Polling (Program 3):          Interrupt (Program 4):
  WiFi task (50ms)              WiFi task (50ms)
  → Gerakan terdeteksi         → [ISR memotong WiFi!]
  → TAPI loop() baru bisa        → tRisingEdge direkam
    memproses setelah 50ms!      → WiFi dilanjutkan
  → Latency: hingga 50ms!      → Loop() proses event
                                 → Latency: < 1µs!
```

---

## 31.10 Tips & Panduan Troubleshooting

### 1. PIR Terus-menerus HIGH Padahal Tidak Ada Orang?
```text
Kemungkinan penyebab (dari paling umum ke paling jarang):

A. WARM-UP belum selesai!
   → Paling umum! Tunggu 30 detik penuh setelah dinyalakan.
   → Selama warm-up, PIR NORMAL mengeluarkan sinyal random.

B. Sumber panas di jangkauan deteksi:
   → AC yang menyala/mati, kipas angin, bohlam lampu pijar,
     sinar matahari langsung yang bergerak, hewan peliharaan!
   → Pindahkan sensor atau tutupi objek pemicu panas.

C. Radio Frequency Interference (RFI):
   → Meletakkan PIR terlalu dekat dengan antena WiFi ESP32
     atau kabel power yang membawa arus besar (relay/motor).
   → Jauhkan minimal 5cm dari sumber EMI.

D. Getaran mekanik berlebihan:
   → Sensor bergetar di robot yang bergerak cepat →
     lensa Fresnel "melihat" sumber panas berbeda-beda.
   → Pasang sensor di platform yang anti-getar.
```

### 2. PIR Tidak Sama Sekali Mendeteksi (Selalu LOW)?
```text
Urutan diagnosis step-by-step:

Langkah 1: Cek tegangan VCC.
  → Ukur VCC ke AM312 dengan multimeter: harus 3.3V atau lebih.
  → Jika di bawah 2.7V → sensor tidak beroperasi!

Langkah 2: Cek koneksi OUT.
  → Hubungkan LED + resistor 220Ω antara pin OUT dan GND.
  → Lewati tangan di depan sensor (setelah warm-up) — LED harus menyala!
  → Jika ya: masalah di kode/pin ESP32. Jika tidak: cek wiring VCC/GND.

Langkah 3: Cek pemilihan PIN_PIR di kode.
  → Pastikan konstanta PIN_PIR cocok dengan jumper fisik kamu.
  → Lakukan Serial.println(digitalRead(34)) di loop untuk verifikasi.

Langkah 4: Pastikan menggunakan INPUT (bukan INPUT_PULLUP).
  → INPUT_PULLUP akan "melawan" output driver AM312 →
    pin selalu terbaca HIGH meskipun sensor mengeluarkan LOW!
```

### 3. Jarak Deteksi Sangat Pendek (< 1 meter)?
```text
AM312 bisa mendeteksi hingga 5 meter, tapi jangkauan aktual
bergantung pada BEBERAPA faktor fisika:

A. Orientasi sensor:
   → AM312 paling sensitif dengan sumbu optik menghadap horizontal.
   → Miringkan sensor ke bawah 45° → jangkauan berkurang drastis!

B. Ukuran target:
   → Tangan saja: 1-2 meter. Tubuh penuh tegak: 3-5 meter.
   → Hewan kecil: 30-50cm. Ini bukan bug, ini fisika piroelektrik!

C. Kecepatan gerakan:
   → Gerakan sangat lambat (< 5 cm/detik) bisa tidak terdeteksi!
   → Elemen piroelektrik butuh PERUBAHAN panas yang cukup cepat.
   → "Berdiri diam" = tidak terdeteksi meski ada di jangkauan!

D. Suhu lingkungan mendekati suhu tubuh (~35°C):
   → Di siang hari sangat panas, beda panas tubuh-lingkungan mengecil.
   → Sensitivitas deteksi berkurang. Ini keterbatasan fisik PIR.
```

### 4. Delay 2 Detik Terasa Terlalu Lama — Bisa Diperpendek?
```text
Sayangnya, AM312 menggunakan delay internal yang TETAP (~2 detik)
dan TIDAK bisa diatur dari luar — berbeda dengan HC-SR501 yang
memiliki potensiometer adjustment!

Strategi workaround profesional:

A. Desain logika aplikasi yang "sabar":
   → State COOLDOWN di Program 3 adalah solusinya.
   → Sistem tetap dianggap "aktif" selama delay berlangsung.
   → Baru kembali IDLE setelah pin LOW + cooldown tambahan.

B. Gunakan HC-SR501 jika WAJIB punya delay yang bisa diatur:
   → HC-SR501 punya potensiometer untuk mengatur 3-300 detik.
   → Trade-off: ukuran lebih besar, butuh 5V.

C. Software debounce untuk efek "lebih cepat":
   → Buat flag "gerak berhenti" yang dikirimkan segera
     saat kamu sudah yakin tidak ada gerakan lagi,
     meski pin OUT secara fisik masih HIGH.
   → Teknik lanjutan — tidak direkomendasikan untuk pemula.
```

### 5. Bagaimana Menghindari False Positive dari Hewan Peliharaan?
```text
Ini adalah pertanyaan umum di sistem keamanan rumah!

Teknik "Pet Immunity" (Kekebalan Hewan):

A. Memasang sensor menghadap KE BAWAH (angled down):
   → Sensor dipasang di sudut atas ruangan, menghadap ke lantai
     pada sudut 30-45°. Anjing/kucing yang tingginya < 50cm
     tidak akan "terlihat" oleh sensor. Tapi manusia tegak = terdeteksi!

B. Mengurangi zona deteksi:
   → Tutupi sebagian lensa Fresnel dengan selotip hitam untuk
     mengurangi sudut pandang vertikal ke bawah.

C. Kombinasi dengan sensor lain (Sensor Fusion):
   → Gabungkan PIR dengan HC-SR04 atau kamera:
     PIR yang terima False Positive DIVERIFIKASI oleh sensor jarak.
     Hewan kecil = jarak < 30cm dari sudut bawah.
     Manusia = jarak lebih jauh dari sudut yang lebih tinggi.
```

### 6. Pengukuran Jarak Kejadian Menggunakan `micros()`?
```text
Di Program 4, kita menggunakan micros() bukan millis() di dalam ISR.

Mengapa?
  → AM312 mengubah pin dalam hitungan mikrodetik setelah mendeteksi IR.
  → millis() hanya resolusi 1ms = tidak cukup presisi untuk cap waktu forensik.
  → micros() resolusi 1µs = cukup untuk rekap log waktu kejadian presisi tinggi.

Catatan penting micros() dalam ISR:
  → micros() AMAN dipanggil dari dalam ISR di ESP32.
  → Berbeda dengan Arduino Uno dimana micros() bisa bermasalah jika
    interrupt timer dimatikan saat ISR berjalan.
```

---

## 31.11 Ringkasan

```
┌─────────────────────────────────────────────────────────────────────┐
│                RINGKASAN BAB 31 — ESP32 + PIR AM312                 │
├─────────────────────────────────────────────────────────────────────┤
│ Wiring       = VCC→3.3V, GND→GND, OUT→IO34 (Input Only)            │
│ Library      = Tidak diperlukan (murni digitalRead + interrupt)      │
│ Tegangan     = 2.7V – 12V (Kompatibel 3.3V langsung!)              │
│ Output Level = HIGH = Ada gerakan, LOW = Tidak ada gerakan          │
│                                                                     │
│ PERINGATAN KRITIS:                                                  │
│   ⚠️  Warm-Up 30 detik WAJIB sebelum pembacaan valid!              │
│   ⚠️  Gunakan INPUT, bukan INPUT_PULLUP!                            │
│   ⚠️  Jangan pakai IO0, IO2, IO12, IO15 (strapping pins)!          │
│                                                                     │
│ Karakteristik AM312:                                                │
│   Hold Time  = ~2 detik setelah gerakan terakhir (tetap, fixed)    │
│   Trigger    = Repeatable (auto-restart timer jika ada gerakan)     │
│   Jangkauan  = 3–5 meter (bergantung kondisi lingkungan)           │
│   Sudut      = ≤ 100°                                               │
│   Arus Idle  = < 0.1 mA (sangat cocok untuk aplikasi baterai!)     │
│                                                                     │
│ Teknik Deteksi:                                                     │
│   Program 1: Polling status — mudah, cocok untuk belajar            │
│   Program 2: Edge detection — deteksi event mulai/berhenti          │
│   Program 3: State Machine — arsitektur alarm profesional           │
│   Program 4: Hardware Interrupt — zero-latency, siap produksi       │
│                                                                     │
│ Kecepatan Deteksi (Latency):                                        │
│   Polling (Prog 1-3): bergantung delay() / interval millis()       │
│   Interrupt (Prog 4): < 1 µs — deteksi instan!                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 31.12 Latihan

1. **Penghitung Pengunjung Ruangan:** Pasang sensor PIR di pintu menghadap masuk. Setiap kali rising edge terdeteksi, increment counter. Tampilkan ke Serial Monitor: `"Total pengunjung hari ini: X orang"`. Tambahkan fitur reset counter menggunakan Push Button (dari BAB 09).

2. **Lampu Otomatis Hemat Energi:** Gabungkan PIR AM312 dengan LED (atau Relay untuk lampu sungguhan). Saat gerakan terdeteksi → LED menyala. Setelah tidak ada gerakan selama 30 detik → LED mati otomatis. Implementasikan dengan State Machine yang bersih, tanpa delay apapun.

3. **Log Waktu Kehadiran:** Integrasikan Program 4 (Interrupt) dengan RTC (Real Time Clock) atau millis(). Setiap kali gerakan terdeteksi, simpan timestamp ke Serial. Format output: `"[HH:MM:SS] Gerakan terdeteksi — Durasi aktif: X detik"`. Hitung dan tampilkan total waktu ada orang di ruangan per sesi boot!

4. **Sensor Fusion PIR + HC-SR04:** Gabungkan BAB 30 (HC-SR04) dan BAB 31 (PIR AM312) dalam satu program menggunakan interrupt untuk keduanya. Logika: Alarm hanya berbunyi jika **KEDUA** sensor setuju ada objek (`PIR=HIGH` **DAN** `jarak < 150cm`). Ini mengurangi false positive secara dramatis — inilah cara sistem keamanan profesional bekerja!

5. **Sistem Anti-Pencuri Berlapis:** Buat sistem 2 zona dengan 2 sensor PIR (jika punya lebih dari satu, atau gunakan 1 PIR + 1 Push Button simulasi sensor kedua). Zona 1: PIR → Peringatan (LED kuning). Zona 2: Button → Alarm penuh (LED merah + Buzzer kencang + log serial). Implementasikan dengan State Machine yang menangani kombinasi state kedua zona!

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Selanjutnya → [BAB 32: LDR — Sensor Cahaya](../BAB-32/README.md)*
