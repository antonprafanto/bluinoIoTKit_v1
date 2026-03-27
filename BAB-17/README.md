# BAB 17: State Machine

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami konsep State Machine (Finite State Machine / FSM)
- Menggambar diagram transisi state
- Mengimplementasikan state machine dengan `switch/case`
- Membuat sistem lampu lalu lintas non-blocking
- Mendesain sistem alarm berlapis dengan state machine

---

## 17.1 Apa itu State Machine?

**State Machine** (mesin status) adalah model pemrograman dimana sistem berada dalam **satu state** pada satu waktu, dan berpindah ke state lain berdasarkan **event** atau **kondisi**.

### Analogi: Lampu Lalu Lintas

```
       ┌─────────┐  5 detik   ┌─────────┐  2 detik   ┌─────────┐
       │  HIJAU  │──────────→│  KUNING │──────────→│  MERAH  │
       └─────────┘            └─────────┘            └────┬────┘
            ↑                                              │
            └──────────────── 5 detik ─────────────────────┘
```

Setiap saat, lampu hanya ada di **satu state**: HIJAU, KUNING, atau MERAH. Transisi terjadi setelah waktu tertentu.

### Mengapa State Machine Penting?

| Tanpa State Machine | Dengan State Machine |
|--------------------|--------------------|
| Banyak variabel boolean (`isRunning`, `isWaiting`, `isDone`...) | Satu variabel `state` |
| Logika if-else bersarang dan rumit | `switch/case` yang bersih |
| Sulit ditambah fitur baru | Mudah ditambah state baru |
| Sulit di-debug | State jelas dan terprediksi |
| Bug "impossible state" | State selalu valid |

---

## 17.2 Struktur Dasar State Machine

```cpp
// 1. Definisikan state menggunakan enum
enum State {
  STATE_IDLE,
  STATE_RUNNING,
  STATE_PAUSED,
  STATE_DONE
};

// 2. Variabel state
State currentState = STATE_IDLE;
unsigned long stateStartTime = 0;

// 3. Fungsi bantu untuk transisi
void changeState(State newState) {
  Serial.print("State: ");
  Serial.print(currentState);
  Serial.print(" → ");
  Serial.println(newState);

  currentState = newState;
  stateStartTime = millis();  // Catat waktu masuk state baru
}

// 4. Loop dengan switch/case
void loop() {
  switch (currentState) {
    case STATE_IDLE:
      // Logika saat IDLE
      break;
    case STATE_RUNNING:
      // Logika saat RUNNING
      break;
    // ...
  }
}
```

---

## 17.3 Program: Lampu Lalu Lintas (Non-Blocking)

```cpp
/*
 * BAB 17 - Program 1: Traffic Light State Machine
 * Lampu lalu lintas non-blocking dengan state machine
 *
 * Wiring:
 *   LED 1 (Merah)  Custom → IO4
 *   LED 3 (Hijau)  Custom → IO17
 *   LED 2 (Kuning) Custom → IO16
 */

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17

// Definisi state
enum TrafficState {
  HIJAU,
  KUNING,
  MERAH
};

TrafficState state = HIJAU;
unsigned long stateStart = 0;

// Durasi setiap state (ms)
const long DURASI_HIJAU  = 5000;  // 5 detik
const long DURASI_KUNING = 2000;  // 2 detik
const long DURASI_MERAH  = 5000;  // 5 detik

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);

  stateStart = millis();
  Serial.println("BAB 17: Traffic Light State Machine");
}

void setLampu(bool merah, bool kuning, bool hijau) {
  digitalWrite(LED_MERAH, merah ? HIGH : LOW);
  digitalWrite(LED_KUNING, kuning ? HIGH : LOW);
  digitalWrite(LED_HIJAU, hijau ? HIGH : LOW);
}

void loop() {
  unsigned long elapsed = millis() - stateStart;

  switch (state) {
    case HIJAU:
      setLampu(false, false, true);  // Hijau ON
      if (elapsed >= DURASI_HIJAU) {
        state = KUNING;
        stateStart = millis();
        Serial.println("HIJAU → KUNING");
      }
      break;

    case KUNING:
      setLampu(false, true, false);  // Kuning ON
      if (elapsed >= DURASI_KUNING) {
        state = MERAH;
        stateStart = millis();
        Serial.println("KUNING → MERAH");
      }
      break;

    case MERAH:
      setLampu(true, false, false);  // Merah ON
      if (elapsed >= DURASI_MERAH) {
        state = HIJAU;
        stateStart = millis();
        Serial.println("MERAH → HIJAU");
      }
      break;
  }
}
```

### Diagram Transisi

```
┌──────────┐                ┌──────────┐                ┌──────────┐
│          │   5s elapsed   │          │   2s elapsed   │          │
│  HIJAU   │───────────────→│  KUNING  │───────────────→│  MERAH   │
│  🟢      │                │  🟡      │                │  🔴      │
│          │                │          │                │          │
└──────────┘                └──────────┘                └────┬─────┘
      ↑                                                      │
      └──────────────────── 5s elapsed ──────────────────────┘
```

---

## 17.4 Program: Security System (Alarm Berlapis)

Sistem alarm yang lebih kompleks dengan multiple state:

```cpp
/*
 * BAB 17 - Program 2: Security System
 * Sistem keamanan dengan state: DISARMED → ARMING → ARMED → TRIGGERED → ALARM
 *
 * Wiring:
 *   Button S1 Custom → IO15 (ARM/DISARM)
 *   PIR AM312 Custom → IO33
 *   Buzzer Custom    → IO13
 *   LED Merah Custom → IO4  (status)
 *   LED Hijau Custom → IO17 (armed indicator)
 */

#define BTN_PIN    15
#define PIR_PIN    33
#define BUZZER_PIN 13
#define LED_RED    4
#define LED_GREEN  17

// ============================================================
// STATE DEFINITION
// ============================================================
enum SecurityState {
  DISARMED,     // Tidak aktif — aman
  ARMING,       // Countdown sebelum aktif (10 detik)
  ARMED,        // Aktif — menunggu gerakan
  TRIGGERED,    // Gerakan terdeteksi — warning 5 detik
  ALARM         // Alarm berbunyi penuh
};

SecurityState state = DISARMED;
unsigned long stateStart = 0;

// Debounce
unsigned long lastDebounce = 0;
int lastBtnState = LOW;
int btnState = LOW;
bool buttonPressed = false;

// Non-blocking blink
unsigned long prevBlink = 0;
bool blinkState = false;

// Durasi
const long ARMING_TIME    = 10000;  // 10 detik countdown
const long TRIGGER_TIME   = 5000;   // 5 detik sebelum alarm penuh
const long BLINK_FAST     = 100;
const long BLINK_SLOW     = 500;

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  Serial.println("BAB 17: Security System State Machine");
  Serial.println("Tekan tombol untuk ARM/DISARM");
}

void changeState(SecurityState newState) {
  state = newState;
  stateStart = millis();

  // Reset output saat ganti state
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
}

// Debounce non-blocking — mengembalikan true sekali saat tombol ditekan
bool cekTombol() {
  buttonPressed = false;
  int reading = digitalRead(BTN_PIN);
  if (reading != lastBtnState) lastDebounce = millis();
  if ((millis() - lastDebounce) > 50) {
    if (reading != btnState) {
      btnState = reading;
      if (btnState == HIGH) buttonPressed = true;
    }
  }
  lastBtnState = reading;
  return buttonPressed;
}

void loop() {
  bool btnPress = cekTombol();
  unsigned long elapsed = millis() - stateStart;

  switch (state) {
    // ──────── DISARMED ────────
    case DISARMED:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);

      if (btnPress) {
        changeState(ARMING);
        Serial.println("→ ARMING: Keluar dalam 10 detik!");
      }
      break;

    // ──────── ARMING (countdown) ────────
    case ARMING:
      // LED hijau berkedip lambat
      if (millis() - prevBlink >= BLINK_SLOW) {
        prevBlink = millis();
        blinkState = !blinkState;
        digitalWrite(LED_GREEN, blinkState ? HIGH : LOW);
      }

      // Tampilkan countdown setiap detik
      {
        static int lastPrintedSecond = -1;
        int currentSecond = elapsed / 1000;
        if (currentSecond != lastPrintedSecond) {
          lastPrintedSecond = currentSecond;
          unsigned long sLeft = (ARMING_TIME >= elapsed) ? (ARMING_TIME - elapsed) / 1000 : 0;
          Serial.print("Arming: ");
          Serial.print(sLeft);
          Serial.println("s");
        }
      }

      if (btnPress) {
        changeState(DISARMED);
        Serial.println("→ DISARMED (dibatalkan)");
      } else if (elapsed >= ARMING_TIME) {
        changeState(ARMED);
        Serial.println("→ ARMED! Sistem aktif.");
      }
      break;

    // ──────── ARMED (menunggu gerakan) ────────
    case ARMED:
      digitalWrite(LED_GREEN, HIGH);  // Solid hijau = armed

      if (btnPress) {
        changeState(DISARMED);
        Serial.println("→ DISARMED");
      } else if (digitalRead(PIR_PIN) == HIGH) {
        changeState(TRIGGERED);
        Serial.println("→ TRIGGERED! Gerakan terdeteksi!");
      }
      break;

    // ──────── TRIGGERED (warning countdown) ────────
    case TRIGGERED:
      // LED merah berkedip cepat
      if (millis() - prevBlink >= BLINK_FAST) {
        prevBlink = millis();
        blinkState = !blinkState;
        digitalWrite(LED_RED, blinkState ? HIGH : LOW);
      }

      if (btnPress) {
        changeState(DISARMED);
        Serial.println("→ DISARMED (kode benar)");
      } else if (elapsed >= TRIGGER_TIME) {
        changeState(ALARM);
        Serial.println("→ ALARM PENUH!");
      }
      break;

    // ──────── ALARM (bunyi penuh) ────────
    case ALARM:
      // Buzzer + LED berkedip cepat
      if (millis() - prevBlink >= BLINK_FAST) {
        prevBlink = millis();
        blinkState = !blinkState;
        digitalWrite(BUZZER_PIN, blinkState ? HIGH : LOW);
        digitalWrite(LED_RED, blinkState ? HIGH : LOW);
      }

      if (btnPress) {
        changeState(DISARMED);
        Serial.println("→ DISARMED (alarm dimatikan)");
      }
      break;
  }
}
```

### Diagram Transisi Security System

```
                        Tombol
┌───────────┐  ────────────────→  ┌───────────┐
│ DISARMED  │                     │  ARMING   │
│ (idle)    │  ←──── Tombol ────  │ (10s cnt) │
└───────────┘                     └─────┬─────┘
      ↑                                 │ 10s elapsed
      │                                 ▼
      │ Tombol                    ┌───────────┐
      ├─────────────────────────  │   ARMED   │
      │                           │ (waiting) │
      │                           └─────┬─────┘
      │                                 │ PIR=HIGH
      │ Tombol                          ▼
      ├─────────────────────────  ┌───────────┐
      │                           │ TRIGGERED │
      │                           │ (5s warn) │
      │                           └─────┬─────┘
      │                                 │ 5s elapsed
      │ Tombol                          ▼
      └─────────────────────────  ┌───────────┐
                                  │   ALARM   │
                                  │ (buzzer!) │
                                  └───────────┘
```

---

## 17.5 State Machine dengan Enum dan String

Tips: buat fungsi yang mengubah state enum ke string untuk debugging:

```cpp
const char* stateToString(SecurityState s) {
  switch (s) {
    case DISARMED:  return "DISARMED";
    case ARMING:    return "ARMING";
    case ARMED:     return "ARMED";
    case TRIGGERED: return "TRIGGERED";
    case ALARM:     return "ALARM";
    default:        return "UNKNOWN";
  }
}

// Gunakan:
Serial.print("State: ");
Serial.println(stateToString(state));
```

---

## 17.6 Panduan Mendesain State Machine

### Langkah 1: Identifikasi State

Tanyakan: **dalam kondisi apa saja sistem bisa berada?**

### Langkah 2: Identifikasi Event/Trigger

Tanyakan: **apa saja yang bisa menyebabkan perubahan state?**

| Tipe Event | Contoh |
|-----------|--------|
| Waktu | Timer expired, countdown habis |
| Input pengguna | Tombol ditekan, Serial command |
| Sensor | PIR mendeteksi gerakan, suhu > 40°C |
| Internal | Counter mencapai limit, error detected |

### Langkah 3: Gambar Diagram Transisi

Gambar setiap state sebagai kotak, dan setiap transisi sebagai panah dengan label event.

### Langkah 4: Implementasi

```
enum → switch/case → transisi dengan kondisi → action di setiap state
```

---

## 📝 Latihan

1. **Mesin cuci:** Desain dan implementasi state machine untuk mesin cuci mini:
   - State: IDLE → WASH → RINSE → SPIN → DONE
   - Tombol untuk Start dan Cancel
   - LED berbeda menyala di setiap state
   - Setiap fase berdurasi 5 detik

2. **Mode LED:** Buat state machine dengan 4 mode LED yang berganti setiap kali tombol ditekan:
   - SOLID ON → BLINK SLOW → BLINK FAST → BREATHING → (kembali ke SOLID ON)

3. **Password system:** Buat state machine yang menerima password 4 digit dari Serial (contoh: "1234"). Jika benar → LED hijau. Jika salah 3 kali → alarm 10 detik.

4. **Diagram dulu:** Sebelum coding latihan 1-3, gambar diagram transisi state-nya terlebih dahulu di kertas. Bandingkan diagram dengan kode yang kamu buat.

---

## 📚 Referensi

- [Finite State Machine — Wikipedia](https://en.wikipedia.org/wiki/Finite-state_machine)
- [State Machine Design Pattern — Embedded Artistry](https://embeddedartistry.com/fieldmanual-terms/state-machine/)
- [Arduino State Machine Tutorial — Majenko](https://majenko.co.uk/blog/finite-state-machine-getting-started)

---

[⬅️ BAB 16: Non-Blocking Programming](../BAB-16/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 18 — Callback & Event-Driven ➡️](../BAB-18/README.md)
