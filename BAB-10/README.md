# BAB 10: Interrupt & ISR

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami konsep interrupt dan mengapa dibutuhkan
- Membuat Interrupt Service Routine (ISR)
- Menggunakan external interrupt untuk push button
- Memahami IRAM_ATTR dan aturan ISR pada ESP32
- Membedakan RISING, FALLING, dan CHANGE trigger

---

## 10.1 Masalah dengan Polling

Pada BAB 9, kita membaca tombol di dalam `loop()` secara terus-menerus. Teknik ini disebut **polling** — CPU terus mengecek status pin setiap waktu.

```cpp
void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    // Tombol ditekan
  }
  // CPU sibuk mengecek terus-menerus...
}
```

**Masalah polling:**
- CPU "sibuk" meskipun tidak ada yang perlu dikerjakan
- Bisa **melewatkan** event singkat jika `loop()` lambat
- Tidak efisien untuk program yang mengerjakan banyak hal

### Solusi: Interrupt

**Interrupt** adalah mekanisme hardware yang **menghentikan** program yang sedang berjalan saat event tertentu terjadi, menjalankan fungsi khusus (ISR), lalu kembali ke program utama.

```
Program utama berjalan...
          │
          │ ← Event terjadi (tombol ditekan)
          │
          ▼
   ┌─────────────┐
   │     ISR      │ ← Fungsi interrupt dijalankan
   │  (singkat!)  │
   └──────┬──────┘
          │
          ▼
Program utama lanjut dari titik terakhir
```

---

## 10.2 External Interrupt pada ESP32

### GPIO yang Mendukung Interrupt

Semua GPIO ESP32 yang bisa di-input **mendukung interrupt** — termasuk GPIO input-only (IO34–IO39).

### Trigger Mode

| Mode | Kapan ISR Dipanggil | Use Case |
|------|---------------------|----------|
| `RISING` | Saat sinyal berubah LOW → HIGH | Tombol ditekan (pull-down) |
| `FALLING` | Saat sinyal berubah HIGH → LOW | Tombol dilepas (pull-down) |
| `CHANGE` | Saat sinyal berubah ke arah apapun | Encoder, sensor cepat |
| `HIGH` | Selama sinyal bernilai HIGH | Jarang dipakai |
| `LOW` | Selama sinyal bernilai LOW | Jarang dipakai |

### Sintaks

```cpp
// Di setup():
attachInterrupt(digitalPinToInterrupt(pin), fungsiISR, mode);

// Untuk melepas interrupt:
detachInterrupt(digitalPinToInterrupt(pin));
```

---

## 10.3 Program: LED Toggle dengan Interrupt

```cpp
/*
 * BAB 10 - Program 1: Interrupt Push Button
 * LED toggle menggunakan interrupt — tidak perlu polling
 *
 * Wiring:
 *   Button S1 Custom → IO15
 *   LED Merah Custom  → IO4
 */

#define BUTTON_PIN 15
#define LED_PIN    4

// Variabel volatile — bisa diubah oleh ISR
volatile bool ledState = false;
volatile unsigned long lastInterruptTime = 0;

// ISR harus singkat dan di-mark IRAM_ATTR
void IRAM_ATTR handleButtonPress() {
  unsigned long interruptTime = millis();

  // Debounce: abaikan jika kurang dari 200ms sejak interrupt terakhir
  if (interruptTime - lastInterruptTime > 200) {
    ledState = !ledState;
    lastInterruptTime = interruptTime;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Pasang interrupt pada RISING edge (pull-down: LOW→HIGH saat ditekan)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, RISING);

  Serial.println("BAB 10: LED Toggle dengan Interrupt");
  Serial.println("Tekan tombol untuk toggle LED...");
}

void loop() {
  // LED dikontrol oleh ISR, loop() bebas melakukan hal lain
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);

  // Loop bisa mengerjakan tugas lain tanpa khawatir melewatkan tombol
  Serial.print("LED: ");
  Serial.println(ledState ? "ON" : "OFF");
  delay(500);
}
```

---

## 10.4 Aturan Penting ISR pada ESP32

### 1. `IRAM_ATTR` Wajib

```cpp
void IRAM_ATTR namaISR() {
  // Kode ISR
}
```

ISR harus disimpan di **IRAM** (Internal RAM) agar bisa diakses cepat. Tanpa `IRAM_ATTR`, ESP32 bisa **crash** karena ISR disimpan di flash yang lambat.

### 2. ISR Harus SINGKAT

| ✅ Boleh di ISR | ❌ DILARANG di ISR |
|-----------------|-------------------|
| Ubah variabel `volatile` | `Serial.print()` |
| Set flag boolean | `delay()` |
| Increment counter | Membaca sensor (I2C, SPI) |
| `digitalWrite()` (singkat) | Alokasi memori (`malloc`, `new`) |
| `digitalRead()` | WiFi/Bluetooth operations |
| `millis()` (aman untuk debounce) | `String` operations |

### 3. Variabel Shared Harus `volatile`

```cpp
volatile int counter = 0;  // Bisa diubah oleh ISR dan loop()
```

`volatile` memberitahu compiler bahwa variabel ini **bisa berubah kapan saja** (oleh interrupt), sehingga tidak di-optimize.

### 4. Race Condition

Jika ISR dan `loop()` mengakses variabel yang sama, gunakan **critical section**:

```cpp
volatile int sharedVar = 0;

void loop() {
  // Nonaktifkan interrupt sementara saat baca variabel shared
  noInterrupts();
  int safeVar = sharedVar;
  interrupts();

  // Gunakan safeVar dengan aman
  Serial.println(safeVar);
}
```

---

## 10.5 Program: Counter dengan Interrupt

```cpp
/*
 * BAB 10 - Program 2: Counter Interrupt
 * Menghitung berapa kali tombol ditekan menggunakan interrupt
 *
 * Wiring: Button S1 Custom → IO15
 */

#define BUTTON_PIN 15

volatile int pressCount = 0;
volatile unsigned long lastPress = 0;

void IRAM_ATTR onButtonPress() {
  unsigned long now = millis();
  if (now - lastPress > 200) {  // Debounce 200ms
    pressCount++;
    lastPress = now;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, RISING);

  Serial.println("BAB 10: Interrupt Counter");
  Serial.println("Tekan tombol — jumlah dihitung otomatis");
}

void loop() {
  // Baca counter dengan aman
  noInterrupts();
  int count = pressCount;
  interrupts();

  Serial.print("Jumlah tekanan: ");
  Serial.println(count);

  delay(1000);  // Update tampilan setiap 1 detik
}
```

---

## 10.6 Program: Dua Interrupt — PIR + Button

```cpp
/*
 * BAB 10 - Program 3: Multi-Interrupt
 * Tombol → toggle alarm ON/OFF
 * PIR    → trigger alarm jika aktif
 *
 * Wiring:
 *   Button S1 Custom → IO15
 *   PIR AM312 Custom  → IO33
 *   Buzzer Custom     → IO13
 *   LED Merah Custom  → IO4
 */

#define BUTTON_PIN 15
#define PIR_PIN    33
#define BUZZER_PIN 13
#define LED_PIN    4

volatile bool alarmEnabled = false;
volatile bool motionDetected = false;
volatile unsigned long lastBtnTime = 0;

void IRAM_ATTR onButton() {
  unsigned long now = millis();
  if (now - lastBtnTime > 300) {
    alarmEnabled = !alarmEnabled;
    lastBtnTime = now;
  }
}

void IRAM_ATTR onMotion() {
  if (alarmEnabled) {
    motionDetected = true;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButton, RISING);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), onMotion, RISING);

  Serial.println("BAB 10: Sistem Alarm Sederhana");
  Serial.println("Tombol = ON/OFF alarm | PIR = Trigger");
}

void loop() {
  // Indikator alarm aktif
  digitalWrite(LED_PIN, alarmEnabled ? HIGH : LOW);

  if (motionDetected) {
    Serial.println("🚨 GERAKAN TERDETEKSI! ALARM!");

    // Bunyi alarm 3 detik
    for (int i = 0; i < 15; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
    }

    motionDetected = false;
  }

  Serial.print("Alarm: ");
  Serial.println(alarmEnabled ? "AKTIF" : "NONAKTIF");
  delay(500);
}
```

---

## 10.7 Kapan Pakai Polling vs Interrupt?

| Kriteria | Polling | Interrupt |
|----------|---------|-----------|
| Event jarang terjadi | ❌ CPU boros | ✅ Efisien |
| Event sangat cepat (<1ms) | ❌ Bisa terlewat | ✅ Langsung ditangkap |
| Butuh respon cepat | ❌ Tergantung loop speed | ✅ Respon mikro-detik |
| Logika kompleks | ✅ Bebas coding | ❌ ISR harus singkat |
| Baca sensor slow (I2C) | ✅ Bisa langsung | ❌ Tidak boleh di ISR |
| Debugging | ✅ Mudah | ❌ Lebih sulit |

**Rekomendasi:** Gunakan interrupt untuk **event capture** (set flag di ISR), lalu proses di `loop()`.

---

## 📝 Latihan

1. **Stopwatch:** Buat stopwatch sederhana:
   - Tombol 1: Start/Stop (interrupt)
   - Tampilkan waktu elapsed di Serial Monitor setiap 100ms

2. **Interrupt FALLING:** Modifikasi Program 1 agar interrupt terjadi saat tombol **dilepas** (FALLING). Apa perbedaan perilakunya?

3. **Dual counter:** Buat 2 counter terpisah untuk 2 tombol. Tampilkan total kedua counter di Serial Monitor.

4. **Critical section:** Buat program yang membuktikan perlunya `noInterrupts()`. Coba baca variabel `long` (4 byte) tanpa critical section — apakah nilainya pernah salah?

---

## 📚 Referensi

- [attachInterrupt() — Arduino Reference](https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/)
- [ESP32 Interrupts — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-external-interrupts-arduino/)
- [IRAM_ATTR — ESP-IDF API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/memory-types.html)

---

[⬅️ BAB 9: Digital Input](../BAB-09/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 11 — Analog Input & ADC ➡️](../BAB-11/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

