# BAB 14: Capacitive Touch

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami cara kerja sensor sentuh kapasitif ESP32
- Membaca nilai touch sensor
- Mengkalibrasi threshold untuk deteksi sentuhan
- Membuat touch button, touch slider, dan touch lamp

---

## 14.1 Sensor Sentuh Kapasitif ESP32

ESP32 memiliki **10 pin touch sensor** bawaan yang bisa mendeteksi sentuhan jari manusia tanpa tombol fisik.

### Cara Kerja

Pin touch bekerja berdasarkan **kapasitansi**:
- Setiap pin terhubung ke kapasitor internal
- Saat jari menyentuh pin, kapasitansi berubah (tubuh manusia = konduktor)
- ESP32 mengukur perubahan ini dan menghasilkan nilai digital

```
Tidak disentuh: Nilai tinggi (contoh: 60-80)
Disentuh:       Nilai rendah (contoh: 10-20)
                ↓
                Perbedaan ini = deteksi sentuhan
```

> **Catatan:** Pada ESP32 versi lama, sentuhan **menurunkan** nilai. Pada ESP32-S2/S3, sentuhan **menaikkan** nilai. Kit Bluino menggunakan ESP32 classic (nilai turun saat disentuh).

### Pin Touch yang Tersedia

| Touch Channel | GPIO | Status pada Kit Bluino |
|---------------|------|----------------------|
| T0 | IO4 | ✅ Tersedia (custom) |
| T1 | IO0 | ❌ BOOT pin — jangan dipakai |
| T2 | IO2 | ✅ Tersedia (custom) |
| T3 | IO15 | ✅ Tersedia (custom) |
| T4 | IO13 | ✅ Tersedia (custom) |
| T5 | IO12 | ❌ Dipakai WS2812 |
| T6 | IO14 | ❌ Dipakai DS18B20 |
| T7 | IO27 | ❌ Dipakai DHT11 |
| T8 | IO33 | ✅ Tersedia (custom) |
| T9 | IO32 | ✅ Tersedia (custom) |

> **Kit Bluino** menggunakan pin **Custom** untuk modul Touch. Kamu bisa menghubungkan kabel jumper dari pad Touch ke salah satu pin Touch yang tersedia (contoh rekomendasi: **IO33 / T8**). Pastikan *tidak* menghubungkannya ke IO27 karena pin tersebut didedikasikan untuk DHT11.

---

## 14.2 Program: Baca Nilai Touch

```cpp
/*
 * BAB 14 - Program 1: Baca Touch Sensor
 * Membaca nilai raw dari touch pad IO27
 *
 * Wiring: Touch Custom → IO33
 */

#define TOUCH_PIN T8  // T8 = IO33 (touch pad via pin custom)

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BAB 14: Capacitive Touch Sensor");
  Serial.println("Sentuh pad touch (IO33) dan lihat nilainya...");
  Serial.println("Nilai TURUN saat disentuh (ESP32 classic)");
  Serial.println();
}

void loop() {
  int touchValue = touchRead(TOUCH_PIN);

  Serial.print("Touch Value: ");
  Serial.print(touchValue);

  if (touchValue < 30) {
    Serial.println(" → TERSENTUH! 🖐️");
  } else {
    Serial.println(" → Tidak tersentuh");
  }

  delay(200);
}
```

**Output contoh:**
```
Touch Value: 72 → Tidak tersentuh
Touch Value: 68 → Tidak tersentuh
Touch Value: 15 → TERSENTUH! 🖐️
Touch Value: 12 → TERSENTUH! 🖐️
Touch Value: 65 → Tidak tersentuh
```

---

## 14.3 Kalibrasi Threshold

Nilai touch bisa berbeda tergantung kondisi lingkungan (kelembaban, noise). Sebaiknya **kalibrasi otomatis** saat startup:

```cpp
/*
 * BAB 14 - Program 2: Kalibrasi Otomatis Touch
 * Membaca baseline saat startup, lalu deteksi sentuhan
 *
 * Wiring: Touch Custom → IO33
 */

#define TOUCH_PIN T8
#define LED_PIN   4

int baseline = 0;       // Nilai dasar tanpa sentuhan
int threshold = 0;      // Threshold deteksi

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("BAB 14: Touch dengan Kalibrasi Otomatis");
  Serial.println("Jangan sentuh pad selama kalibrasi...");
  delay(2000);

  // Kalibrasi: baca rata-rata 20 sampel
  long total = 0;
  for (int i = 0; i < 20; i++) {
    total += touchRead(TOUCH_PIN);
    delay(50);
  }
  baseline = total / 20;
  threshold = baseline * 60 / 100;  // 60% dari baseline

  Serial.print("Baseline: ");
  Serial.println(baseline);
  Serial.print("Threshold: ");
  Serial.println(threshold);
  Serial.println("Kalibrasi selesai! Silakan sentuh pad.");
}

void loop() {
  int touchValue = touchRead(TOUCH_PIN);
  bool touched = (touchValue < threshold);

  digitalWrite(LED_PIN, touched ? HIGH : LOW);

  if (touched) {
    Serial.print("SENTUH! (");
    Serial.print(touchValue);
    Serial.println(")");
  }

  delay(50);
}
```

---

## 14.4 Program: Touch Toggle LED

```cpp
/*
 * BAB 14 - Program 3: Touch Toggle
 * Sentuh sekali = LED ON, sentuh lagi = LED OFF
 *
 * Wiring: Touch Custom → IO33, LED Custom → IO4
 */

#define TOUCH_PIN T8
#define LED_PIN   4
#define THRESHOLD 30  // Sesuaikan setelah kalibrasi

bool ledState = false;
bool lastTouched = false;
unsigned long lastTouchTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("BAB 14: Touch Toggle LED");
}

void loop() {
  int touchValue = touchRead(TOUCH_PIN);
  bool touched = (touchValue < THRESHOLD);

  // Deteksi edge (baru menyentuh, bukan masih menyentuh)
  if (touched && !lastTouched && (millis() - lastTouchTime > 300)) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastTouchTime = millis();

    Serial.print("Toggle! LED: ");
    Serial.println(ledState ? "ON" : "OFF");
  }

  lastTouched = touched;
  delay(20);
}
```

---

## 14.5 Touch Interrupt

ESP32 juga mendukung **interrupt** pada touch pin:

```cpp
/*
 * BAB 14 - Program 4: Touch Interrupt
 * Menggunakan interrupt untuk deteksi sentuhan (hemat daya)
 *
 * Wiring: Touch Custom → IO33, LED Custom → IO4
 */

#define TOUCH_PIN T8
#define LED_PIN   4
#define THRESHOLD 30

volatile bool touchDetected = false;

void touchCallback() {
  touchDetected = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Pasang touch interrupt
  touchAttachInterrupt(TOUCH_PIN, touchCallback, THRESHOLD);

  Serial.println("BAB 14: Touch Interrupt");
  Serial.println("Sentuh pad untuk menyalakan LED...");
}

void loop() {
  if (touchDetected) {
    Serial.println("Touch interrupt triggered!");
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    touchDetected = false;
  }

  // CPU bisa melakukan hal lain atau tidur (deep sleep)
  delay(10);
}
```

> **Fitur penting:** Touch interrupt bisa **membangunkan ESP32 dari deep sleep** — berguna untuk membuat perangkat ultra-low power yang aktif saat disentuh.

---

## 14.6 Membuat Touch Pad Sendiri

Selain pad built-in di kit, kamu bisa membuat touch pad sendiri:

| Material | Cara | Sensitivitas |
|----------|------|--------------|
| Kabel jumper | Sentuh ujung kabel | Rendah |
| Aluminium foil | Tempel ke permukaan, hubungkan ke GPIO | Tinggi |
| PCB pad | Desain pad kuprum di PCB | Sangat tinggi |
| Konduktif tape | Tempel di meja/dinding | Tinggi |

**Tips:**
- Pad yang lebih besar → sensitivitas lebih tinggi
- Hindari kabel terlalu panjang (noise)
- Tambahkan resistor 1MΩ ke GND untuk stabilitas (opsional)

---

## 📝 Latihan

1. **Multi-touch:** Hubungkan kabel jumper ke IO4 (T0) dan IO32 (T9). Buat program yang mendeteksi sentuhan pada masing-masing pin dan menyalakan LED yang berbeda.

2. **Touch dimmer:** Buat program dimana menyentuh pad lama = LED makin terang (PWM naik), melepaskan = brightness berhenti di level tersebut.

3. **Touch piano:** Hubungkan 4 kabel jumper ke 4 touch pin berbeda. Setiap sentuhan menghasilkan bunyi buzzer dengan nada berbeda (Do, Re, Mi, Fa).

4. **Touch counter:** Setiap sentuhan menambah counter. Tampilkan counter di Serial Monitor. Reset ke 0 jika menyentuh lebih dari 3 detik (long press).

---

## 📚 Referensi

- [touchRead() — Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/touch.html)
- [ESP32 Touch Sensor — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-touch-pins-arduino-ide/)
- [ESP32 Touch Pad — Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/touch_pad.html)

---

[⬅️ BAB 13: DAC](../BAB-13/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 15 — Timer Hardware ➡️](../BAB-15/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

