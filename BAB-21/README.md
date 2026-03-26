# BAB 21: Debugging & Troubleshooting

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menggunakan Serial Monitor secara efektif untuk debugging
- Membuat debug logging system yang bisa dimatikan
- Mendiagnosis error umum ESP32 (crash, brownout, boot loop)
- Memahami dan membaca Exception Decoder
- Menggunakan watchdog timer untuk mendeteksi hang
- Menerapkan teknik debugging sistematis

---

## 21.1 Serial Debugging — Teknik Lanjut

### Debug Macro — Bisa Dimatikan

Jangan biarkan `Serial.println()` berserakan di produksi. Gunakan macro yang bisa dimatikan:

```cpp
// ============================================================
// DEBUG SYSTEM — comment #define ini untuk mematikan semua debug
// ============================================================
#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
  #define DEBUG_BEGIN(baud)   Serial.begin(baud)
  #define DEBUG_PRINT(x)     Serial.print(x)
  #define DEBUG_PRINTLN(x)   Serial.println(x)
  #define DEBUG_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_BEGIN(baud)   // Kosong — tidak ada kode
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

void setup() {
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("Debug mode aktif");
  DEBUG_PRINTF("Free heap: %u bytes\n", ESP.getFreeHeap());
}
```

> **Keuntungan:** Saat siap produksi, cukup comment `#define DEBUG_ENABLED` — semua debug print hilang dan **tidak menggunakan memori atau CPU**.

---

## 21.2 Program: Multi-Level Debug Logger

```cpp
/*
 * BAB 21 - Program 1: Debug Logger dengan Level
 * Log dengan level: ERROR, WARN, INFO, DEBUG
 */

// ============================================================
// LOG LEVEL — set level minimum yang ditampilkan
// ============================================================
#define LOG_LEVEL_ERROR  0
#define LOG_LEVEL_WARN   1
#define LOG_LEVEL_INFO   2
#define LOG_LEVEL_DEBUG  3

#define CURRENT_LOG_LEVEL  LOG_LEVEL_INFO  // Ubah ini untuk filter

// Macro logging
#define LOG_ERROR(msg)  if (CURRENT_LOG_LEVEL >= LOG_LEVEL_ERROR) { \
  Serial.print("[ERROR] "); Serial.print(millis()); \
  Serial.print("ms "); Serial.println(msg); }

#define LOG_WARN(msg)   if (CURRENT_LOG_LEVEL >= LOG_LEVEL_WARN) { \
  Serial.print("[WARN]  "); Serial.print(millis()); \
  Serial.print("ms "); Serial.println(msg); }

#define LOG_INFO(msg)   if (CURRENT_LOG_LEVEL >= LOG_LEVEL_INFO) { \
  Serial.print("[INFO]  "); Serial.print(millis()); \
  Serial.print("ms "); Serial.println(msg); }

#define LOG_DEBUG(msg)  if (CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG) { \
  Serial.print("[DEBUG] "); Serial.print(millis()); \
  Serial.print("ms "); Serial.println(msg); }

// ============================================================
// PIN
// ============================================================
#define LED_PIN 4
#define POT_PIN 34

void setup() {
  Serial.begin(115200);
  delay(1000);

  LOG_INFO("=== System Startup ===");
  LOG_INFO("Initializing GPIO...");

  pinMode(LED_PIN, OUTPUT);
  analogSetAttenuation(ADC_11db);

  LOG_INFO("GPIO initialized");
  LOG_DEBUG("LED_PIN = 4, POT_PIN = 34");

  // Cek memori
  if (ESP.getFreeHeap() < 20000) {
    LOG_WARN("Free heap rendah!");
  } else {
    LOG_INFO("Memory OK");
  }

  LOG_INFO("System ready");
}

void loop() {
  int pot = analogRead(POT_PIN);

  LOG_DEBUG("Pot raw: ");  // Hanya tampil jika level = DEBUG

  if (pot > 4000) {
    LOG_WARN("Pot mendekati maximum!");
  }

  if (pot < 10) {
    LOG_ERROR("Pot reading abnormal — kemungkinan disconnected!");
  }

  delay(1000);
}
```

**Output (level = INFO):**
```
[INFO]  1002ms === System Startup ===
[INFO]  1003ms Initializing GPIO...
[INFO]  1005ms GPIO initialized
[INFO]  1008ms Memory OK
[INFO]  1010ms System ready
[WARN]  5012ms Pot mendekati maximum!
```

Pesan `[DEBUG]` **tidak muncul** karena level diset ke INFO. Ubah `CURRENT_LOG_LEVEL` ke `LOG_LEVEL_DEBUG` untuk melihat semua pesan.

---

## 21.3 Error Umum ESP32 & Solusi

### 1. Brownout Detector

```
Brownout detector was triggered
```

| Penyebab | Solusi |
|----------|--------|
| Power supply lemah | Gunakan kabel USB pendek dan langsung ke port USB (bukan hub) |
| Beban terlalu besar | Jangan drive beban besar langsung dari GPIO |
| WiFi consume banyak arus | Tambahkan kapasitor 100µF di VIN-GND |

### 2. Boot Loop

```
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
(terus berulang)
```

| Penyebab | Solusi |
|----------|--------|
| IO12 ditarik HIGH saat boot | Pastikan WS2812 tidak mengirim sinyal HIGH ke IO12 saat boot |
| IO0 ditarik LOW | Periksa bahwa tombol BOOT tidak terjebak |
| IO2 HIGH saat boot (beberapa board) | Lepas koneksi IO2 saat upload |
| Crash di setup() | Upload sketch kosong via BOOT mode |

### 3. Guru Meditation Error

```
Guru Meditation Error: Core  1 panic'ed (LoadProhibited)
```

| Penyebab | Solusi |
|----------|--------|
| Null pointer dereference | Cek pointer sebelum akses |
| Stack overflow | Kurangi variabel lokal besar, hindari rekursi dalam |
| Buffer overflow | Pastikan array index tidak melebihi ukuran |
| ISR terlalu panjang | Pindahkan logika dari ISR ke loop() |

### 4. WiFi Connect Gagal

```
WiFi: STA disconnected, reason: 201
```

| Penyebab | Solusi |
|----------|--------|
| SSID/password salah | Cek ulang credentials |
| ADC2 dipakai saat WiFi | Pindah ke ADC1 (IO32-IO39) |
| Power lemah saat TX | Tambahkan kapasitor, gunakan USB power yang baik |
| Jarak terlalu jauh | Dekatkan ke router, atau gunakan antena eksternal |

---

## 21.4 Program: Self-Diagnostic System

```cpp
/*
 * BAB 21 - Program 2: Self-Diagnostic
 * ESP32 memeriksa kesehatannya sendiri saat startup
 */

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("╔═══════════════════════════════════════╗");
  Serial.println("║        ESP32 Self-Diagnostic          ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.println();

  int issues = 0;

  // ── Check 1: Reset Reason ──
  Serial.print("Reset Reason Core 0: ");
  printResetReason(rtc_get_reset_reason(0));
  Serial.print("Reset Reason Core 1: ");
  printResetReason(rtc_get_reset_reason(1));

  // ── Check 2: Memory ──
  uint32_t freeHeap = ESP.getFreeHeap();
  Serial.print("Free Heap: ");
  Serial.print(freeHeap / 1024);
  Serial.print(" KB");
  if (freeHeap < 20000) {
    Serial.println(" ← ⚠️ RENDAH!");
    issues++;
  } else {
    Serial.println(" ✅");
  }

  uint32_t maxBlock = ESP.getMaxAllocHeap();
  Serial.print("Max Alloc Block: ");
  Serial.print(maxBlock / 1024);
  Serial.print(" KB");
  if (maxBlock < 10000) {
    Serial.println(" ← ⚠️ Fragmentasi!");
    issues++;
  } else {
    Serial.println(" ✅");
  }

  // ── Check 3: Flash ──
  Serial.print("Flash Size: ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.print(" MB");
  if (ESP.getFlashChipSize() < 4 * 1024 * 1024) {
    Serial.println(" ← ⚠️ Kurang dari 4MB!");
    issues++;
  } else {
    Serial.println(" ✅");
  }

  // ── Check 4: Chip Temperature (jika tersedia) ──
  // Catatan: temperatureRead() hanya untuk indikasi, tidak presisi
  float temp = temperatureRead();
  Serial.print("Chip Temp: ");
  Serial.print(temp, 1);
  Serial.print(" °C");
  if (temp > 80) {
    Serial.println(" ← 🔴 OVERHEATING!");
    issues++;
  } else if (temp > 60) {
    Serial.println(" ← ⚠️ Panas");
  } else {
    Serial.println(" ✅");
  }

  // ── Check 5: Sketch Size ──
  uint32_t sketchSize = ESP.getSketchSize();
  uint32_t freeSketch = ESP.getFreeSketchSpace();
  Serial.print("Sketch: ");
  Serial.print(sketchSize / 1024);
  Serial.print(" KB / ");
  Serial.print((sketchSize + freeSketch) / 1024);
  Serial.print(" KB (");
  Serial.print(sketchSize * 100 / (sketchSize + freeSketch));
  Serial.println("% used) ✅");

  // ── Hasil ──
  Serial.println();
  if (issues == 0) {
    Serial.println("✅ Semua diagnostik LULUS — sistem siap!");
  } else {
    Serial.print("⚠️ Ditemukan ");
    Serial.print(issues);
    Serial.println(" issue — periksa log di atas.");
  }
  Serial.println();
}

void printResetReason(int reason) {
  switch (reason) {
    case 1:  Serial.println("Power-on Reset"); break;
    case 3:  Serial.println("Software Reset"); break;
    case 4:  Serial.println("Legacy Watch Dog Reset"); break;
    case 5:  Serial.println("Deep Sleep Reset"); break;
    case 6:  Serial.println("Reset by SLC"); break;
    case 7:  Serial.println("Timer Group 0 WDT Reset"); break;
    case 8:  Serial.println("Timer Group 1 WDT Reset"); break;
    case 9:  Serial.println("RTC WDT Reset"); break;
    case 10: Serial.println("Intrusion Test Reset"); break;
    case 11: Serial.println("Timer Group Reset CPU"); break;
    case 12: Serial.println("Software CPU Reset"); break;
    case 13: Serial.println("RTC WDT Reset CPU"); break;
    case 14: Serial.println("External CPU Reset"); break;
    case 15: Serial.println("Brownout Reset"); break;
    case 16: Serial.println("RTC WDT CPU Reset"); break;
    default: Serial.println("Unknown"); break;
  }
}

void loop() {
  delay(10000);
}
```

---

## 21.5 Watchdog Timer (WDT)

Watchdog timer adalah **timer keamanan** yang me-reset ESP32 jika program hang (tidak merespons dalam waktu tertentu).

```cpp
/*
 * BAB 21 - Program 3: Watchdog Timer
 * Demonstrasi watchdog — ESP32 reset otomatis jika hang
 */

#include <esp_task_wdt.h>

#define WDT_TIMEOUT 5  // Timeout 5 detik

void setup() {
  Serial.begin(115200);

  // Konfigurasi Task Watchdog Timer
  esp_task_wdt_config_t config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = 0,          // Tidak monitor idle tasks
    .trigger_panic = true         // Reset ESP32 saat timeout
  };
  esp_task_wdt_init(&config);

  // Tambahkan task saat ini ke watchdog
  esp_task_wdt_add(NULL);

  Serial.println("BAB 21: Watchdog Timer");
  Serial.println("Program harus 'feed' WDT setiap <5 detik");
  Serial.println("Jika tidak, ESP32 akan reset otomatis");
}

int counter = 0;

void loop() {
  counter++;

  // Feed (reset) watchdog — memberitahu bahwa sistem masih hidup
  esp_task_wdt_reset();

  Serial.print("Loop #");
  Serial.print(counter);
  Serial.println(" — WDT fed ✅");

  // Simulasi: setelah 10 iterasi, program "hang"
  if (counter >= 10) {
    Serial.println("⚠️ Simulasi hang — WDT tidak di-feed...");
    while (true) {
      // Program macet di sini
      // WDT akan timeout setelah 5 detik dan RESET ESP32
    }
  }

  delay(1000);
}
```

---

## 21.6 Teknik Debugging Sistematis

### Metode "Binary Search" untuk Bug

```
Program crash di suatu tempat?

1. Comment SETENGAH kode di loop()
2. Apakah masih crash?
   ├─ Ya  → Bug di setengah yang tersisa
   └─ Tidak → Bug di setengah yang di-comment
3. Ulangi sampai menemukan baris penyebab
```

### Metode "Breadcrumb" (Jejak Roti)

```cpp
void loop() {
  Serial.println(">>> Checkpoint A");
  bacaSensor();

  Serial.println(">>> Checkpoint B");
  prosesData();

  Serial.println(">>> Checkpoint C");   // Jika ini tidak muncul,
  updateDisplay();                       // crash terjadi di prosesData()

  Serial.println(">>> Checkpoint D");
}
```

### Checklist Troubleshooting

```
┌──────────────────────────────────────────────────────────┐
│              CHECKLIST DEBUGGING                          │
│                                                           │
│  □ 1. Apakah board terdeteksi? (Device Manager → Port)   │
│  □ 2. Apakah baud rate Serial Monitor = 115200?          │
│  □ 3. Apakah board setting = ESP32 Dev Module?           │
│  □ 4. Apakah Pin yang dipakai benar? (cek config.h)     │
│  □ 5. Apakah ada error saat compile? (baca pesan error)  │
│  □ 6. Apakah ada reset loop? (cek Reset Reason)          │
│  □ 7. Berapa free heap? (cek memory)                     │
│  □ 8. Apakah kabel jumper terhubung dengan baik?         │
│  □ 9. Apakah komponen benar secara polaritas? (LED±)     │
│  □ 10. Coba program minimal (hanya blink) — berfungsi?   │
└──────────────────────────────────────────────────────────┘
```

---

## 21.7 Tip: Assert untuk Catch Bug Lebih Awal

```cpp
// Fungsi assert sederhana untuk ESP32
#define ASSERT(condition, message) \
  if (!(condition)) { \
    Serial.print("ASSERT FAILED: "); \
    Serial.print(message); \
    Serial.print(" at "); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.println(__LINE__); \
    while(true) { delay(1000); }  \
  }

// Penggunaan:
void setup() {
  Serial.begin(115200);
  int pin = -1;

  ASSERT(pin >= 0, "Pin tidak valid!");  // Program berhenti di sini dengan pesan jelas
}
```

---

## 📝 Latihan

1. **Debug logger:** Tambahkan log level system (Program 1) ke salah satu program dari BAB sebelumnya. Pastikan di level INFO hanya pesan penting yang muncul, dan di level DEBUG semua detail muncul.

2. **Self-diagnostic:** Modifikasi Program 2 agar juga memeriksa:
   - Apakah GPIO dapat diakses (coba `pinMode`, baca nilainya)
   - Apakah ADC berfungsi (baca pin yang dipasang potensiometer)
   - Tampilkan laporan dalam format tabel yang rapi

3. **Watchdog real:** Buat program sensor yang membaca DHT11 setiap 2 detik. Tambahkan watchdog 10 detik. Jika DHT gagal dibaca 5 kali berturut-turut, matikan WDT feed agar ESP32 reset otomatis.

4. **Error recovery:** Buat program yang menghitung jumlah error (sensor gagal dibaca) dan menyimpannya di RTC memory. Setelah reset, tampilkan: "Error sebelumnya: X kali, total reset: Y."

---

## 📚 Referensi

- [ESP32 Exception Decoder — GitHub](https://github.com/me-no-dev/EspExceptionDecoder)
- [ESP32 Watchdog Timer — Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html)
- [ESP32 Troubleshooting Guide — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-troubleshooting-guide/)
- [Reset Reasons — Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/misc_system_api.html)

---

[⬅️ BAB 20: Memory Management](../BAB-20/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 22 — Serial Communication ➡️](../BAB-22/README.md)
