# BAB 16: Non-Blocking Programming

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Mendesain program yang sepenuhnya non-blocking
- Membuat kelas Task sederhana untuk mengorganisir tugas periodik
- Menangani input Serial tanpa blocking
- Menerapkan pola non-blocking untuk sekuens operasi (LED pattern, alarm)
- Memahami kapan blocking diperbolehkan dan kapan tidak

---

## 16.1 Filosofi Non-Blocking

Pada BAB 15 kita belajar `millis()` sebagai pengganti `delay()`. BAB ini akan memperdalam **pola pikir non-blocking** — bagaimana mendesain seluruh program agar tidak pernah "macet".

### Aturan Emas

```
┌──────────────────────────────────────────────────────────┐
│              ATURAN NON-BLOCKING                          │
│                                                           │
│  1. JANGAN pernah pakai delay() di loop utama             │
│  2. Setiap fungsi harus KEMBALI secepat mungkin           │
│  3. Simpan STATE, bukan menunggu                          │
│  4. Biarkan loop() berputar dengan cepat                  │
│  5. Gunakan millis() untuk menentukan KAPAN bertindak     │
└──────────────────────────────────────────────────────────┘
```

### Blocking vs Non-Blocking

```
BLOCKING (❌):                    NON-BLOCKING (✅):
                                  
void bunyiAlarm() {               void bunyiAlarm() {
  for (int i=0; i<5; i++) {         if (millis() - prev >= 200) {
    buzzerON();                        prev = millis();
    delay(200);  // MACET!             buzzerState = !buzzerState;
    buzzerOFF();                       digitalWrite(BUZZER, buzzerState);
    delay(200);  // MACET lagi!        count++;
  }                                  }
}                                    if (count >= 10) alarmDone = true;
                                   }
→ 2 detik tidak responsif         → Kembali dalam µs, tombol tetap responsif
```

---

## 16.2 Pattern: Kelas Task Sederhana

Daripada membuat banyak variabel `prevMillis` terpisah, kita bisa membungkusnya dalam struct:

```cpp
/*
 * BAB 16 - Program 1: Task Scheduler Sederhana
 * Menjalankan beberapa tugas periodik secara terstruktur
 *
 * Wiring:
 *   LED Merah Custom → IO4
 *   Buzzer Custom    → IO13
 */

#define LED_PIN    4
#define BUZZER_PIN 13
#define POT_PIN    34

// ============================================================
// STRUCT TASK — menyimpan state setiap tugas
// ============================================================
struct Task {
  unsigned long interval;    // Interval eksekusi (ms)
  unsigned long lastRun;     // Waktu terakhir dijalankan
  const char* name;          // Nama task (untuk debug)

  bool isReady() {
    if (millis() - lastRun >= interval) {
      lastRun = millis();
      return true;
    }
    return false;
  }
};

// Definisi tasks
Task taskLED    = {500,  0, "LED Blink"};
Task taskSensor = {2000, 0, "Baca Sensor"};
Task taskStatus = {5000, 0, "Status Report"};

bool ledState = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  analogSetAttenuation(ADC_11db);

  Serial.println("BAB 16: Task Scheduler");
  Serial.println("3 task berjalan secara non-blocking");
}

void loop() {
  // Task 1: LED berkedip setiap 500ms
  if (taskLED.isReady()) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }

  // Task 2: Baca sensor setiap 2 detik
  if (taskSensor.isReady()) {
    int pot = analogRead(POT_PIN);
    Serial.print("[SENSOR] Pot: ");
    Serial.println(pot);
  }

  // Task 3: Laporan status setiap 5 detik
  if (taskStatus.isReady()) {
    Serial.print("[STATUS] Uptime: ");
    Serial.print(millis() / 1000);
    Serial.print("s | Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
  }
}
```

---

## 16.3 Non-Blocking Sequence (Sekuens)

Salah satu tantangan terbesar non-blocking adalah mengeksekusi **sekuens langkah** (step 1 → step 2 → step 3) tanpa `delay()`. Solusinya: simpan **current step** sebagai state.

```cpp
/*
 * BAB 16 - Program 2: Non-Blocking LED Sequence
 * 4 LED menyala bergantian TANPA delay()
 *
 * Wiring:
 *   LED 1 Custom → IO4
 *   LED 2 Custom → IO16
 *   LED 3 Custom → IO17
 *   LED 4 Custom → IO2
 */

const int LED_PINS[] = {4, 16, 17, 2};
const int JUMLAH_LED = 4;

int currentLED = 0;                // LED mana yang sedang menyala
unsigned long prevSwitch = 0;
const long SWITCH_INTERVAL = 200;  // Ganti LED setiap 200ms
bool goingForward = true;          // Arah running

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  Serial.println("BAB 16: Non-Blocking LED Running");
}

void loop() {
  // Hanya bertindak jika waktunya tiba
  if (millis() - prevSwitch >= SWITCH_INTERVAL) {
    prevSwitch = millis();

    // Matikan semua LED
    for (int i = 0; i < JUMLAH_LED; i++) {
      digitalWrite(LED_PINS[i], LOW);
    }

    // Nyalakan LED saat ini
    digitalWrite(LED_PINS[currentLED], HIGH);

    // Pindah ke LED berikutnya (maju-mundur)
    if (goingForward) {
      currentLED++;
      if (currentLED >= JUMLAH_LED - 1) goingForward = false;
    } else {
      currentLED--;
      if (currentLED <= 0) goingForward = true;
    }
  }

  // Loop tetap bisa menangani tugas lain di sini!
  // Misalnya: baca tombol, update sensor, dll.
}
```

---

## 16.4 Non-Blocking Serial Input

Membaca perintah dari Serial Monitor juga harus non-blocking:

```cpp
/*
 * BAB 16 - Program 3: Non-Blocking Serial Command
 * Menerima perintah dari Serial Monitor tanpa blocking
 *
 * Perintah: "on", "off", "blink", "status"
 *
 * Wiring: LED Custom → IO4
 */

#define LED_PIN 4

String inputBuffer = "";  // Buffer karakter yang masuk
bool blinkMode = false;
bool ledState = false;
unsigned long prevBlink = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("BAB 16: Non-Blocking Serial Command");
  Serial.println("Ketik: on / off / blink / status");
}

void loop() {
  // ====== Non-blocking Serial read ======
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      // Perintah lengkap — proses
      if (inputBuffer.length() > 0) {
        prosesPerintah(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;  // Tambah ke buffer
    }
  }

  // ====== Blink mode (non-blocking) ======
  if (blinkMode && (millis() - prevBlink >= 300)) {
    prevBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}

void prosesPerintah(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "on") {
    blinkMode = false;
    ledState = true;
    digitalWrite(LED_PIN, HIGH);
    Serial.println("→ LED ON");

  } else if (cmd == "off") {
    blinkMode = false;
    ledState = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println("→ LED OFF");

  } else if (cmd == "blink") {
    blinkMode = true;
    Serial.println("→ LED Blink Mode");

  } else if (cmd == "status") {
    Serial.print("→ LED: ");
    Serial.print(ledState ? "ON" : "OFF");
    Serial.print(" | Mode: ");
    Serial.print(blinkMode ? "Blink" : "Manual");
    Serial.print(" | Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println("s");

  } else {
    Serial.print("→ Perintah tidak dikenal: ");
    Serial.println(cmd);
  }
}
```

---

## 16.5 Non-Blocking Buzzer Alarm

Contoh alarm yang berbunyi **tanpa** memblokir tombol OFF:

```cpp
/*
 * BAB 16 - Program 4: Non-Blocking Alarm
 * Buzzer alarm yang bisa dimatikan kapan saja dengan tombol
 *
 * Wiring:
 *   Button S1 Custom → IO15
 *   Buzzer Custom    → IO13
 *   LED Merah Custom → IO4
 */

#define BUTTON_PIN 15
#define BUZZER_PIN 13
#define LED_PIN    4

// State alarm
bool alarmActive = false;
bool buzzerState = false;
unsigned long prevBuzz = 0;
const long BUZZ_INTERVAL = 150;

// Debounce
unsigned long lastDebounce = 0;
int lastBtnState = LOW;
int btnState = LOW;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  alarmActive = true;  // Mulai dengan alarm aktif
  Serial.println("BAB 16: Non-Blocking Alarm");
  Serial.println("Tekan tombol untuk MATIKAN alarm");
}

void loop() {
  // ====== Debounce tombol (non-blocking) ======
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastBtnState) lastDebounce = millis();

  if ((millis() - lastDebounce) > 50) {
    if (reading != btnState) {
      btnState = reading;
      if (btnState == HIGH) {
        alarmActive = !alarmActive;
        if (!alarmActive) {
          digitalWrite(BUZZER_PIN, LOW);
          digitalWrite(LED_PIN, LOW);
          Serial.println("Alarm DIMATIKAN");
        } else {
          Serial.println("Alarm DINYALAKAN");
        }
      }
    }
  }
  lastBtnState = reading;

  // ====== Buzzer alarm (non-blocking) ======
  if (alarmActive && (millis() - prevBuzz >= BUZZ_INTERVAL)) {
    prevBuzz = millis();
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    digitalWrite(LED_PIN, buzzerState ? HIGH : LOW);
  }
}
```

---

## 16.6 Kapan Blocking Diperbolehkan?

Tidak semua `delay()` itu jahat. Blocking diperbolehkan dalam situasi tertentu:

| Konteks | Blocking OK? | Alasan |
|---------|-------------|--------|
| `setup()` — inisialisasi | ✅ OK | Hanya berjalan sekali, tidak ada task lain |
| Waiting for boot (1-2 detik) | ✅ OK | Menunggu hardware siap |
| Baca sensor lambat (DHT11) | ⚠️ Hati-hati | ~250ms blocking — bisa diterima jika jarang |
| Animasi singkat (<100ms) | ⚠️ Tergantung | Jika tidak ada input kritis |
| Loop utama multi-task | ❌ Tidak | Semua task harus non-blocking |
| Saat WiFi aktif | ❌ Tidak | WiFi stack butuh CPU time secara periodik |

> **Aturan praktis:** Jika program kamu hanya melakukan **satu hal**, `delay()` tidak masalah. Begitu kamu punya **dua atau lebih task** yang harus responsif, gunakan `millis()`.

---

## 16.7 Anti-Pattern yang Harus Dihindari

```cpp
// ❌ ANTI-PATTERN 1: delay() di loop utama
void loop() {
  bacaSensor();
  delay(1000);    // Tombol tidak responsif selama 1 detik!
  updateDisplay();
}

// ❌ ANTI-PATTERN 2: while loop menunggu kondisi
void loop() {
  while (digitalRead(BUTTON) == LOW) {
    // Program macet di sini sampai tombol ditekan!
  }
}

// ❌ ANTI-PATTERN 3: for loop dengan delay
void nyalakanSemuaLED() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(500);   // 2 detik total blocking!
  }
}

// ✅ SOLUSI: Simpan state, biarkan loop berputar
int ledIndex = 0;
unsigned long prevLED = 0;

void loop() {
  if (millis() - prevLED >= 500 && ledIndex < 4) {
    prevLED = millis();
    digitalWrite(leds[ledIndex], HIGH);
    ledIndex++;
  }
  // Tombol dan task lain tetap responsif
}
```

---

## 📝 Latihan

1. **Multi-blink refactored:** Gunakan struct `Task` dari Program 1 untuk membuat 4 LED berkedip dengan interval berbeda (200ms, 500ms, 1000ms, 2000ms) secara bersamaan.

2. **Serial dimmer:** Buat program yang menerima angka 0–255 dari Serial Monitor (non-blocking) dan mengatur brightness LED menggunakan PWM.

3. **Alarm + countdown:** Buat alarm yang berbunyi 10 kali lalu berhenti otomatis, tapi bisa dimatikan kapan saja dengan tombol. Tampilkan hitungan mundur di Serial Monitor.

4. **Event logger:** Buat sistem yang mencatat timestamp setiap kali tombol ditekan (non-blocking) dan menampilkan log lengkap saat ketik "log" di Serial Monitor.

---

## 📚 Referensi

- [Blink Without Delay — Arduino](https://docs.arduino.cc/built-in-examples/digital/BlinkWithoutDelay/)
- [How to write non-blocking code — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-multitasking-arduino/)
- [Cooperative Multitasking — Embedded Artistry](https://embeddedartistry.com/blog/2017/02/01/improving-our-simple-scheduler/)

---

[⬅️ BAB 15: Timer Hardware](../BAB-15/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 17 — State Machine ➡️](../BAB-17/README.md)
