# BAB 22: UART / Serial Communication

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami prinsip kerja komunikasi serial UART secara mendalam
- Menggunakan `Serial`, `Serial1`, dan `Serial2` pada ESP32 secara bersamaan
- Mengirim dan menerima data terstruktur melalui Serial
- Membangun protokol komunikasi antar perangkat berbasis UART
- Melakukan debug dan parsing data Serial dengan teknik profesional

---

## 22.1 Apa itu UART?

**UART** (Universal Asynchronous Receiver-Transmitter) adalah protokol komunikasi serial **paling tua dan paling sederhana** yang masih sangat luas digunakan hingga hari ini. Kata kunci penting: **Asynchronous** — artinya tidak ada sinyal clock bersama antara pengirim dan penerima; keduanya harus *sepakat dulu* pada kecepatan yang sama (baud rate).

### Komunikasi Serial vs Paralel

```
PARALEL (lama, mahal):          SERIAL (modern, hemat pin):

D0 ══════════════════           TX ═══════════════════
D1 ══════════════════           RX ═══════════════════
D2 ══════════════════
D3 ══════════════════           Cukup 2 kabel!
D4 ══════════════════
D5 ══════════════════
D6 ══════════════════
D7 ══════════════════

8 kabel data + 1 kabel clock
```

### Struktur Frame UART

Setiap karakter yang dikirim melalui UART dibungkus dalam sebuah **frame**:

```
IDLE  START  D0  D1  D2  D3  D4  D5  D6  D7  PARITY  STOP
──────┐     ┌───┬───┬───┬───┬───┬───┬───┬───┬───────┬──────
  1   │  0  │ x │ x │ x │ x │ x │ x │ x │ x │  opt  │  1
      └─────┴───┴───┴───┴───┴───┴───┴───┴───┴───────┘

• IDLE    : Line HIGH (diam)
• START   : 1 bit LOW — tanda awal frame
• D0–D7  : 8 bit data (LSB first)
• PARITY  : Opsional — deteksi error
• STOP    : 1 bit HIGH — tanda akhir frame
```

### Baud Rate

Baud rate adalah jumlah **bit per detik** yang dikirim. Pengirim dan penerima **harus identik**:

| Baud Rate | Kecepatan Nyata | Penggunaan Umum |
|-----------|-----------------|-----------------|
| 9600      | ~960 byte/det   | GPS, sensor tua |
| 19200     | ~1920 byte/det  | Modem serial |
| 38400     | ~3840 byte/det  | Komunikasi embedded |
| 57600     | ~5760 byte/det  | Bluetooth HC-05 |
| 115200    | ~11520 byte/det | Debug ESP32 (standar) |
| 230400    | ~23040 byte/det | Firmware flashing |
| 921600    | ~92160 byte/det | Download firmware cepat |

> ⚡ **Aturan Praktis:** Untuk komunikasi antar mikrokontroler di jarak pendek (<1 meter), gunakan **115200 bps**. Untuk jarak lebih jauh atau kabel panjang, turunkan ke **9600** untuk kestabilan.

---

## 22.2 UART di ESP32

ESP32 memiliki **3 port UART** hardware:

```
┌─────────────────────────────────────────────────────────┐
│                  ESP32 UART Hardware                     │
├──────────┬──────────┬──────────┬────────────────────────┤
│ Port     │ TX Pin   │ RX Pin   │ Fungsi Default          │
├──────────┼──────────┼──────────┼────────────────────────┤
│ UART0    │ IO1 (TX0)│ IO3 (RX0)│ Serial Monitor (USB)    │
│ UART1    │ IO10     │ IO9      │ (Flash SPI — hindari!)  │
│ UART2    │ IO17     │ IO16     │ Bebas dipakai           │
└──────────┴──────────┴──────────┴────────────────────────┘
```

> ⚠️ **Penting untuk Kit Bluino:**
> - **UART0** (`IO1`/`IO3`) sudah terpakai oleh Serial Monitor via USB. **Jangan sambungkan perangkat lain ke pin ini!**
> - **UART1** menabrak Flash SPI internal (`IO9`/`IO10`) — **dilarang keras** digunakan.
> - **UART2** (`IO16`/`IO17`) adalah pilihan aman untuk komunikasi dengan perangkat eksternal.

### Remapping Pin UART

Keistimewaan ESP32: pin UART bisa di-*remap* ke GPIO mana pun menggunakan `begin()` dengan parameter tambahan:

```cpp
// Syntax lengkap Serial2.begin():
Serial2.begin(baudRate, config, rxPin, txPin);

// Contoh: gunakan IO14 sebagai RX, IO12 sebagai TX untuk Serial2
Serial2.begin(9600, SERIAL_8N1, 14, 12);
```

---

## 22.3 Objek Serial di Arduino ESP32

### `Serial` (UART0 — Serial Monitor)

Ini yang paling sering kita pakai untuk debug:

```cpp
void setup() {
  Serial.begin(115200);          // Inisialisasi
  Serial.println("Halo!");       // Kirim string dengan newline
  Serial.print("Nilai: ");       // Kirim tanpa newline
  Serial.println(42);            // Integer
  Serial.println(3.14, 2);       // Float dengan 2 desimal
  Serial.printf("X=%d Y=%.1f\n", 10, 3.5); // Format printf
}
```

### Buffer Serial

Serial Arduino bekerja dengan **buffer FIFO** (antrian):

```
Perangkat Pengirim           ESP32 (Penerima)
─────────────────           ────────────────────────────
"Hello\n"    ────RX──►  [ H | e | l | l | o | \n |   ]
                               Buffer RX (128 byte default)
                               ↓
                         Serial.available()  → 6
                         Serial.read()       → 'H'
                         Serial.read()       → 'e'
                         ... dst
```

### Fungsi-Fungsi Penting

| Fungsi | Keterangan |
|--------|-----------|
| `Serial.available()` | Jumlah byte yang sudah masuk di buffer RX |
| `Serial.read()` | Baca 1 byte dari buffer (hapus dari buffer) |
| `Serial.peek()` | Lihat byte terdepan tanpa menghapusnya |
| `Serial.readString()` | Baca semua buffer menjadi String (blocking!) |
| `Serial.readStringUntil(c)` | Baca sampai karakter `c` |
| `Serial.readBytes(buf, n)` | Baca `n` byte ke array `buf` |
| `Serial.flush()` | Tunggu hingga semua TX selesai terkirim |
| `Serial.write(byte)` | Kirim 1 byte mentah |
| `Serial.print()` | Kirim data sebagai teks ASCII |
| `Serial.println()` | `print()` + karakter `\r\n` |
| `Serial.printf()` | Kirim dengan format gaya C printf |

---

## 22.4 Program 1: Komunikasi Dasar Serial Monitor

```cpp
/*
 * BAB 22 - Program 1: Echo Serial
 * Semua yang dikirim dari Serial Monitor
 * akan di-echo kembali dengan modifikasi.
 *
 * Wiring: Tidak ada — hanya via USB Serial
 */

String inputBuffer = "";
bool newCommand = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }  // Tunggu Serial siap (ESP32 bisa tanpa ini, tapi aman)

  Serial.println("========================================");
  Serial.println("  BAB 22: UART Serial Echo Demo");
  Serial.println("  Ketik sesuatu lalu tekan Enter!");
  Serial.println("========================================");
}

void loop() {
  // ── Baca Serial tanpa blocking ──
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        newCommand = true;
        break; // Berhenti sebentar agar loop() bisa memproses
      }
    } else {
      // Proteksi RAM (Cegah kehabisan Heap jika data masuk tanpa newline)
      if (inputBuffer.length() < 250) {
        inputBuffer += c;
      }
    }
  }

  // ── Proses command ──
  if (newCommand) {
    Serial.println("─────────────────────────────");
    Serial.print("Kamu ketik  : ");
    Serial.println(inputBuffer);
    Serial.print("Panjang     : ");
    Serial.print(inputBuffer.length());
    Serial.println(" karakter");
    Serial.print("Huruf besar : ");
    inputBuffer.toUpperCase();
    Serial.println(inputBuffer);
    Serial.println("─────────────────────────────");

    inputBuffer = "";
    newCommand = false;
  }
}
```

**Contoh output di Serial Monitor:**
```
========================================
  BAB 22: UART Serial Echo Demo
  Ketik sesuatu lalu tekan Enter!
========================================
─────────────────────────────
Kamu ketik  : halo dunia
Panjang     : 10 karakter
Huruf besar : HALO DUNIA
─────────────────────────────
```

---

## 22.5 Parsing Data Serial Terstruktur

Dalam proyek IoT nyata, kita sering menerima data dalam format terstruktur, misalnya dari GPS, modul Bluetooth, atau sensor cerdas. Teknik *parsing* ini adalah skill fundamental.

### Format Data Umum

```
Contoh format CSV (Comma-Separated Values):
"SENSOR,25.4,68.2,1013\n"
  │       │    │    └── Tekanan (hPa)
  │       │    └─────── Kelembaban (%)
  │       └──────────── Suhu (°C)
  └──────────────────── Header/tipe data

Contoh format Key-Value:
"T=25.4;H=68.2;P=1013\n"

Contoh format dengan marker:
"<S>25.4,68.2,1013</S>\n"
```

### Program 2: Parser Data CSV dari Serial

```cpp
/*
 * BAB 22 - Program 2: Parser Data CSV
 * Menerima data format: "SENSOR,suhu,kelembaban\n"
 * Contoh: "SENSOR,25.4,68.2\n"
 *
 * Wiring: Tidak ada — via Serial Monitor
 * Coba ketik: SENSOR,25.4,68.2
 */

#define MAX_BUF 64

char rxBuf[MAX_BUF];
uint8_t rxIdx = 0;
bool lineReady = false;

// Struct untuk menyimpan data terparsing
struct SensorData {
  float suhu;
  float kelembaban;
  bool valid;
};

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 22 - Parser CSV");
  Serial.println("Format input: SENSOR,<suhu>,<kelembaban>");
  Serial.println("Contoh: SENSOR,25.4,68.2");
}

// Baca Serial ke buffer char array (lebih hemat heap dari String)
void bacaSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      if (rxIdx > 0) {
        rxBuf[rxIdx] = '\0';  // Null-terminate
        lineReady = true;
        rxIdx = 0;
        break; // Hentikan baca jika 1 baris utuh sudah siap
      }
    } else if (rxIdx < MAX_BUF - 1) {
      rxBuf[rxIdx++] = c;
    }
    // Data melebihi buffer → abaikan (proteksi overflow)
  }
}

// Parse string CSV menjadi struct SensorData
SensorData parseCSV(const char* line) {
  SensorData data = {0.0f, 0.0f, false};

  // Salin ke buffer lokal agar strtok tidak merusak original
  char buf[MAX_BUF];
  strncpy(buf, line, MAX_BUF - 1);
  buf[MAX_BUF - 1] = '\0';

  char* token = strtok(buf, ",");

  // Token 1: Header harus "SENSOR"
  if (token == nullptr || strcmp(token, "SENSOR") != 0) {
    return data;  // Header tidak valid
  }

  // Token 2: Suhu
  token = strtok(nullptr, ",");
  if (token == nullptr) return data;
  data.suhu = atof(token);

  // Token 3: Kelembaban
  token = strtok(nullptr, ",");
  if (token == nullptr) return data;
  data.kelembaban = atof(token);

  data.valid = true;
  return data;
}

void loop() {
  bacaSerial();

  if (lineReady) {
    lineReady = false;

    Serial.print("Diterima: \"");
    Serial.print(rxBuf);
    Serial.println("\"");

    SensorData hasil = parseCSV(rxBuf);

    if (hasil.valid) {
      Serial.println("✅ Data valid!");
      Serial.printf("   Suhu       : %.1f °C\n", hasil.suhu);
      Serial.printf("   Kelembaban : %.1f %%\n", hasil.kelembaban);

      // Logika sederhana berdasarkan data
      if (hasil.suhu > 35.0f) {
        Serial.println("   ⚠️  PERINGATAN: Suhu terlalu tinggi!");
      }
      if (hasil.kelembaban > 80.0f) {
        Serial.println("   ⚠️  PERINGATAN: Kelembaban sangat tinggi!");
      }
    } else {
      Serial.println("❌ Format tidak valid. Gunakan: SENSOR,suhu,kelembaban");
    }

    Serial.println();
  }
}
```

---

## 22.6 UART2 — Komunikasi Antar Perangkat

Saat kita butuh berbicara dengan modul eksternal (GPS, Bluetooth HC-05, modul GSM, dsb.) sembari tetap memonitor via Serial Monitor, kita gunakan **Serial2 (UART2)**.

### Program 3: Bridge Serial Monitor ↔ Serial2

```cpp
/*
 * BAB 22 - Program 3: Serial Bridge (Passthrough)
 * Apa pun yang masuk dari Serial Monitor diteruskan ke Serial2,
 * dan sebaliknya. Berguna untuk:
 *   - Konfigurasi modul Bluetooth (HC-05 AT Commands)
 *   - Debug komunikasi GPS
 *   - Uji coba modul GSM/4G
 *
 * Wiring:
 *   Perangkat Eksternal TX → IO16 (RX2 ESP32)
 *   Perangkat Eksternal RX → IO17 (TX2 ESP32)
 *   GND perangkat         → GND ESP32
 *
 * ⚠️  Pastikan tegangan logika perangkat eksternal = 3.3V
 *    atau gunakan level shifter jika 5V!
 */

#define SERIAL2_BAUD  9600   // Sesuaikan dengan modul Anda
#define RX2_PIN       16
#define TX2_PIN       17

void setup() {
  Serial.begin(115200);
  Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, RX2_PIN, TX2_PIN);

  Serial.println("═══════════════════════════════════════");
  Serial.println("  BAB 22: Serial Bridge Aktif");
  Serial.printf("  Serial2: RX=IO%d, TX=IO%d, %d bps\n",
                RX2_PIN, TX2_PIN, SERIAL2_BAUD);
  Serial.println("  Ketik di Serial Monitor → dikirim ke Serial2");
  Serial.println("  Data dari Serial2 → tampil di Serial Monitor");
  Serial.println("═══════════════════════════════════════");
}

void loop() {
  // Serial Monitor → Serial2
  if (Serial.available()) {
    char c = (char)Serial.read();
    Serial2.write(c);
  }

  // Serial2 → Serial Monitor
  if (Serial2.available()) {
    char c = (char)Serial2.read();
    Serial.write(c);
  }
}
```

### Diagram Koneksi Serial2

```
┌─────────────────────────────────────────────────────────┐
│                    Kit Bluino (ESP32)                    │
│                                                          │
│  [USB / PC]                  [Modul Eksternal]           │
│      │                             │                     │
│   Serial0 ◄─── UART0 ───►  (debug only)                 │
│  (IO1/IO3)                                               │
│                                                          │
│   IO16 (RX2) ◄──────────── TX  [Modul GPS/BT/GSM]       │
│   IO17 (TX2) ──────────►   RX  [Modul GPS/BT/GSM]       │
│   GND        ───────────── GND [Modul GPS/BT/GSM]       │
└─────────────────────────────────────────────────────────┘
```

---

## 22.7 Protokol Komunikasi Sederhana

Dalam sistem nyata, kita perlu membuat aturan (*protokol*) agar komunikasi antara dua mikrokontroler tidak salah tafsir. Berikut desain protokol sederhana yang robust.

### Desain Protokol: `<CMD:VALUE>`

```
Format pesan:
  '<' + PERINTAH + ':' + NILAI + '>' + '\n'

Contoh:
  "<LED:ON>\n"       → Nyalakan LED
  "<LED:OFF>\n"      → Matikan LED
  "<BUZZER:3>\n"     → Bunyi buzzer 3 kali
  "<STATUS:?>\n"     → Minta laporan status
  "<SERVO:90>\n"     → Set servo ke 90 derajat

Respon (dari ESP32 kembali ke pengirim):
  "<OK:LED_ON>\n"    → Konfirmasi berhasil
  "<ERR:UNKNOWN>\n"  → Perintah tidak dikenal
```

### Program 4: Command Handler Protokol `<CMD:VALUE>`

```cpp
/*
 * BAB 22 - Program 4: Protocol Command Handler
 * Menerima perintah format <CMD:VALUE> dari Serial Monitor
 * dan mengeksekusi tindakan yang sesuai.
 *
 * Wiring:
 *   LED Merah Custom → IO4
 *   Buzzer Custom    → IO13
 */

#define LED_PIN    4
#define BUZZER_PIN 13

#define MAX_BUF    64

char rxBuf[MAX_BUF];
uint8_t rxIdx = 0;
bool    msgReady = false;

// ── Baca Serial, cari paket <...> ──────────────────────────
void bacaProtokol() {
  static bool inPacket = false;

  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '<') {
      inPacket = true;
      rxIdx = 0;
    } else if (c == '>' && inPacket) {
      rxBuf[rxIdx] = '\0';
      msgReady = true;
      inPacket = false;
      break; // Keluar loop agar pesan langsung diproses
    } else if (inPacket && rxIdx < MAX_BUF - 1) {
      rxBuf[rxIdx++] = c;
    }
  }
}

// ── Kirim respon berformat ──────────────────────────────────
void kirimRespon(const char* status, const char* nilai) {
  Serial.printf("<%s:%s>\n", status, nilai);
}

// ── Eksekusi perintah ───────────────────────────────────────
void eksekusiPerintah(const char* msg) {
  // Pisahkan CMD dan VALUE dengan delimiter ':'
  char buf[MAX_BUF];
  strncpy(buf, msg, MAX_BUF - 1);
  buf[MAX_BUF - 1] = '\0';

  char* cmd   = strtok(buf, ":");
  char* value = strtok(nullptr, ":");

  if (cmd == nullptr) {
    kirimRespon("ERR", "EMPTY_CMD");
    return;
  }

  // ── LED ──
  if (strcmp(cmd, "LED") == 0) {
    if (value == nullptr) { kirimRespon("ERR", "NO_VALUE"); return; }

    if (strcmp(value, "ON") == 0) {
      digitalWrite(LED_PIN, HIGH);
      kirimRespon("OK", "LED_ON");
    } else if (strcmp(value, "OFF") == 0) {
      digitalWrite(LED_PIN, LOW);
      kirimRespon("OK", "LED_OFF");
    } else {
      kirimRespon("ERR", "INVALID_VALUE");
    }
  }

  // ── BUZZER ──
  else if (strcmp(cmd, "BUZZER") == 0) {
    if (value == nullptr) { kirimRespon("ERR", "NO_VALUE"); return; }

    int kali = atoi(value);
    if (kali < 1 || kali > 10) {
      kirimRespon("ERR", "OUT_OF_RANGE"); return;
    }

    for (int i = 0; i < kali; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      if (i < kali - 1) delay(100);
    }
    kirimRespon("OK", "BUZZER_DONE");
  }

  // ── STATUS ──
  else if (strcmp(cmd, "STATUS") == 0) {
    char info[32];
    snprintf(info, sizeof(info), "LED=%s,UP=%lus",
             digitalRead(LED_PIN) ? "ON" : "OFF",
             millis() / 1000UL);
    kirimRespon("OK", info);
  }

  // ── Unknown ──
  else {
    kirimRespon("ERR", "UNKNOWN_CMD");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN,    OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("══════════════════════════════════════════");
  Serial.println("  BAB 22: Protocol Command Handler");
  Serial.println("  Format: <CMD:VALUE>");
  Serial.println("  Contoh: <LED:ON>  <BUZZER:3>  <STATUS:?>");
  Serial.println("══════════════════════════════════════════");
}

void loop() {
  bacaProtokol();

  if (msgReady) {
    msgReady = false;
    Serial.print("Terima: <");
    Serial.print(rxBuf);
    Serial.println(">");
    eksekusiPerintah(rxBuf);
  }
}
```

**Contoh sesi di Serial Monitor:**
```
Terima: <LED:ON>
<OK:LED_ON>
Terima: <BUZZER:3>
<OK:BUZZER_DONE>
Terima: <STATUS:?>
<OK:LED=ON,UP=42s>
Terima: <XYZ:123>
<ERR:UNKNOWN_CMD>
```

---

## 22.8 Pengiriman Data Biner (Binary Protocol)

Selain data teks ASCII, UART juga bisa mengirim data **biner mentah** — lebih efisien dan kompak.

```cpp
/*
 * BAB 22 - Program 5 (Konsep): Kirim Struct via Serial
 * Mengirimkan struct data sensor sebagai byte biner.
 *
 * Berguna saat bandwidth terbatas atau butuh efisiensi tinggi.
 */

// Paket data 6 byte (pastikan #pragma pack atau layout konsisten
// di kedua sisi pengirim & penerima!)
struct __attribute__((packed)) DataPacket {
  uint8_t  header;      // 0xAA — penanda awal paket
  int16_t  suhu10;      // Suhu × 10 (misal 254 = 25.4°C)
  uint16_t kelembaban10;// Kelembaban × 10
  uint8_t  checksum;    // XOR semua byte sebelumnya
};

uint8_t hitungChecksum(const uint8_t* data, size_t len) {
  uint8_t cs = 0;
  for (size_t i = 0; i < len; i++) cs ^= data[i];
  return cs;
}

void kirimPaket(float suhu, float kelembaban) {
  DataPacket pkt;
  pkt.header       = 0xAA;
  pkt.suhu10       = (int16_t)(suhu * 10);
  pkt.kelembaban10 = (uint16_t)(kelembaban * 10);

  // Hitung checksum dari semua field kecuali checksum itu sendiri
  pkt.checksum = hitungChecksum((uint8_t*)&pkt,
                                sizeof(pkt) - sizeof(pkt.checksum));

  Serial.write((uint8_t*)&pkt, sizeof(pkt));
}
```

> 💡 **Kapan pakai biner vs teks?**
> | | Teks ASCII | Biner |
> |---|---|---|
> | Keterbacaan | ✅ Mudah dibaca manusia | ❌ Tidak terbaca langsung |
> | Ukuran data | ❌ Lebih besar (3× lebih besar untuk angka) | ✅ Lebih kompak |
> | Parsing | ✅ Mudah di debug | ❌ Perlu parser khusus |
> | Kecepatan | ❌ Lebih lambat | ✅ Lebih cepat |
>
> **Rekomendasi:** Gunakan teks ASCII untuk debugging dan komunikasi dengan Python/PC. Gunakan biner untuk komunikasi antar mikrokontroler atau saat bandwidth sangat terbatas.

---

## 22.9 Tips & Teknik Debugging Serial

### 1. Selalu Periksa `Serial.available()` Sebelum `read()`

```cpp
// ❌ SALAH — bisa baca garbage jika buffer kosong
char c = Serial.read();

// ✅ BENAR
if (Serial.available() > 0) {
  char c = Serial.read();
}
```

### 2. Jangan Gunakan `readString()` di Produksi

```cpp
// ❌ Blocking! Menunggu timeout (default 1 detik)
String data = Serial.readString();

// ✅ Non-blocking — kumpulkan karakter satu per satu
while (Serial.available() > 0) {
  char c = Serial.read();
  // ... proses karakter
}
```

### 3. Hati-hati dengan `\r\n` vs `\n`

Serial Monitor Arduino mengirim `\r\n` (Carriage Return + Line Feed) secara default. Selalu filter keduanya:

```cpp
if (c == '\n' || c == '\r') {
  // Akhir baris
}
```

### 4. Buffer Overflow Protection

```cpp
#define MAX_BUF 64
char buf[MAX_BUF];
uint8_t idx = 0;

while (Serial.available() > 0) {
  char c = Serial.read();
  if (c == '\n' || c == '\r') {
    if (idx > 0) {
      buf[idx] = '\0';
      // proses buf
      idx = 0;
      break; // <--- Keluar loop agar diproses
    }
  } else if (idx < MAX_BUF - 1) {  // ← Proteksi overflow
    buf[idx++] = c;
  }
  // Jika idx >= MAX_BUF - 1, karakter dibuang → tidak crash
}
```

### 5. Gunakan `Serial.printf()` untuk Output Terformat

```cpp
// ❌ Verbose dan lambat
Serial.print("Suhu: ");
Serial.print(suhu, 1);
Serial.print(" C, Kelembaban: ");
Serial.print(kelembaban, 1);
Serial.println(" %");

// ✅ Ringkas dan cepat
Serial.printf("Suhu: %.1f C, Kelembaban: %.1f %%\n", suhu, kelembaban);
```

---

## 22.10 Ringkasan

```
┌─────────────────────────────────────────────────────────┐
│              RINGKASAN BAB 22 — UART/SERIAL              │
├─────────────────────────────────────────────────────────┤
│ UART: Asynchronous, 2 kabel (TX/RX), tidak ada clock    │
│                                                         │
│ ESP32 punya 3 UART hardware:                            │
│  • Serial  (UART0) → IO1/IO3  — Serial Monitor          │
│  • Serial1 (UART1) → IO9/IO10 — ⚠️ Hindari (Flash)     │
│  • Serial2 (UART2) → IO16/IO17 — Bebas dipakai         │
│                                                         │
│ Pola baca NON-BLOCKING:                                 │
│  while (Serial.available()) { char c = Serial.read(); } │
│                                                         │
│ Format Protokol disarankan:                             │
│  • Teks: <CMD:VALUE>\n (mudah debug)                    │
│  • Biner: Header + Data + Checksum (efisien)            │
│                                                         │
│ Pantangan:                                              │
│  ❌ Serial.readString() → blocking                      │
│  ❌ Serial.read() tanpa cek available()                 │
│  ❌ Buffer tanpa limit → stack overflow                 │
└─────────────────────────────────────────────────────────┘
```

---

## 22.11 Latihan

1. **Echo Terbalik**: Modifikasi Program 1 agar string yang di-echo dibalik urutannya (misal: "halo" → "olah").

2. **Parser Multi-Field**: Kembangkan Program 2 agar bisa menerima format `SENSOR,id,suhu,kelembaban,tekanan` dengan validasi bahwa `id` adalah angka antara 1–99.

3. **Command Handler Baru**: Tambahkan perintah `<BLINK:n>` pada Program 4 yang membuat LED berkedip sebanyak `n` kali (antara 1–10) secara **non-blocking** menggunakan pola `millis()` dari BAB 16.

4. **Dua Serial Sekaligus**: Buat program yang menerima data suhu dari Serial2 (simulasikan dengan mengetik di Serial Monitor kedua via port berbeda), lalu tampilkan di Serial. Perhatikan pengelolaan dua buffer terpisah.

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Selanjutnya → [BAB 23: I2C — Inter-Integrated Circuit](../BAB-23/README.md)*
