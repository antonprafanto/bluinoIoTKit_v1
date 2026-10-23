/*
 * BAB 38 - Program 5: Servo Multi-Mode dengan State Machine
 * Tekan LAMA S1 (minimal 1 detik) untuk berganti mode:
 *   PRESET → SAPU → POTENSIOMETER → PARKIR → PRESET → ...
 *   PRESET        : S1/S2 singkat = preset berikutnya/sebelumnya
 *   SAPU          : bolak-balik 0°–180°; S2 singkat = ganti kecepatan
 *   POTENSIOMETER : servo mengikuti potensiometer
 *   PARKIR        : servo ke 90°, lalu sinyalnya dilepas (detach)
 *
 * Rangkaian:
 *   SRV (header hijau, paling bawah) → IO32, servo di header servo kit
 *   SW1 (header kuning, pin ke-3)     → IO15
 *   SW2 (header kuning, pin ke-4)     → IO25
 *   VR10K (header kuning, pin ke-5)   → IO34
 */

#include <ESP32Servo.h>

#define PIN_SERVO 32  // SRV → IO32
#define PIN_S1 15     // SW1 → IO15 (ditekan = HIGH)
#define PIN_S2 25     // SW2 → IO25 (ditekan = HIGH)
#define PIN_POT 34    // VR10K → IO34

const int PULSA_MIN_US = 500;   // ← ganti dengan hasil kalibrasi Program 2
const int PULSA_MAX_US = 2400;  // ← ganti dengan hasil kalibrasi Program 2

const unsigned long DEBOUNCE_MS = 50;
const unsigned long TEKAN_LAMA_MS = 1000;  // tahan minimal 1 detik = tekan lama
const unsigned long INTERVAL_POT_MS = 20;

const int PRESET[] = {0, 45, 90, 135, 180};
const int JUMLAH_PRESET = sizeof(PRESET) / sizeof(PRESET[0]);

// ── Tipe data (ditulis sebelum fungsi mana pun) ──
enum Mode { MODE_PRESET, MODE_SAPU, MODE_POT, MODE_PARKIR, JUMLAH_MODE };
enum KejadianTombol { TIDAK_ADA, TEKAN_SINGKAT, TEKAN_LAMA };

struct Tombol {
  uint8_t pin;
  bool bacaanLalu;             // bacaan mentah sebelumnya
  bool stabil;                 // keadaan yang sudah pasti (true = ditekan)
  unsigned long waktuBerubah;  // kapan bacaan mentah terakhir berubah
  unsigned long waktuDitekan;  // kapan tombol mulai ditekan
  bool lamaSudahDilapor;       // tekan lama sudah dilaporkan untuk tekanan ini?
};

Tombol tombolS1 = {PIN_S1, false, false, 0, 0, false};
Tombol tombolS2 = {PIN_S2, false, false, 0, 0, false};

Servo servo;
Mode mode = MODE_PRESET;

int sudutSekarang = 90;          // sudut yang sedang dikirim ke servo
int sudutTarget = 90;            // sudut tujuan
unsigned long jedaGerakMs = 10;  // kecepatan: 1° setiap sekian ms
unsigned long waktuGerakLalu = 0;

int indeksPreset = 2;  // preset ketiga (90°)
float adcHalus = 0;
unsigned long waktuPotLalu = 0;

const char *namaMode(Mode m) {
  switch (m) {
    case MODE_PRESET: return "PRESET";
    case MODE_SAPU: return "SAPU";
    case MODE_POT: return "POTENSIOMETER";
    case MODE_PARKIR: return "PARKIR";
    default: return "?";
  }
}

// Debounce + membedakan tekan singkat dan tekan lama
KejadianTombol periksaTombol(Tombol &t, unsigned long sekarang) {
  bool bacaan = (digitalRead(t.pin) == HIGH);
  if (bacaan != t.bacaanLalu) {
    t.bacaanLalu = bacaan;
    t.waktuBerubah = sekarang;
  }
  if (sekarang - t.waktuBerubah >= DEBOUNCE_MS && bacaan != t.stabil) {
    t.stabil = bacaan;
    if (t.stabil) {  // baru ditekan: mulai menghitung lama tekan
      t.waktuDitekan = sekarang;
      t.lamaSudahDilapor = false;
    } else if (!t.lamaSudahDilapor) {
      return TEKAN_SINGKAT;  // dilepas sebelum 1 detik
    }
  }
  if (t.stabil && !t.lamaSudahDilapor && sekarang - t.waktuDitekan >= TEKAN_LAMA_MS) {
    t.lamaSudahDilapor = true;  // laporkan sekali saja per tekanan
    return TEKAN_LAMA;
  }
  return TIDAK_ADA;
}

void pastikanTerpasang() {
  if (!servo.attached()) {
    servo.attach(PIN_SERVO, PULSA_MIN_US, PULSA_MAX_US);
    servo.write(sudutSekarang);
    Serial.println("  Sinyal servo dipasang lagi");
  }
}

void pilihPreset(int indeks) {
  indeksPreset = indeks;
  sudutTarget = PRESET[indeksPreset];
  Serial.printf("  Preset %d dari %d -> target %d derajat\n",
                indeksPreset + 1, JUMLAH_PRESET, sudutTarget);
}

// Aksi masuk: dijalankan SEKALI setiap kali pindah mode
void pindahMode(Mode modeBaru) {
  mode = modeBaru;
  Serial.printf("[MODE] %s\n", namaMode(mode));
  pastikanTerpasang();
  jedaGerakMs = 10;

  switch (mode) {
    case MODE_PRESET:
      Serial.println("  S1/S2 singkat = preset berikutnya/sebelumnya");
      pilihPreset(indeksPreset);
      break;
    case MODE_SAPU:
      Serial.println("  S2 singkat = ganti kecepatan");
      jedaGerakMs = 15;
      sudutTarget = (sudutSekarang < 90) ? 180 : 0;  // menuju ujung yang lebih jauh
      break;
    case MODE_POT:
      Serial.println("  Putar potensiometer");
      jedaGerakMs = 3;  // cepat mengikuti tangan
      adcHalus = analogRead(PIN_POT);
      break;
    case MODE_PARKIR:
      Serial.println("  Menuju 90 derajat, lalu sinyal dilepas");
      sudutTarget = 90;
      break;
    default:
      break;
  }
}

// Gerakkan servo 1° menuju sudutTarget setiap jedaGerakMs
void gerakHalus(unsigned long sekarang) {
  if (sudutSekarang == sudutTarget) return;
  if (sekarang - waktuGerakLalu < jedaGerakMs) return;
  waktuGerakLalu = sekarang;
  if (sudutTarget > sudutSekarang) {
    sudutSekarang = sudutSekarang + 1;
  } else {
    sudutSekarang = sudutSekarang - 1;
  }
  servo.write(sudutSekarang);
}

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 38 - Program 5: Servo Multi-Mode");
  Serial.println("Tekan lama S1 (1 detik) untuk ganti mode");
  pinMode(PIN_S1, INPUT);
  pinMode(PIN_S2, INPUT);
  analogReadResolution(12);

  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, PULSA_MIN_US, PULSA_MAX_US);
  if (!servo.attached()) {
    Serial.println("Gagal memasang servo di IO32! Periksa nomor pin.");
    while (true) {
      delay(1000);  // program berhenti di sini
    }
  }
  servo.write(sudutSekarang);
  pindahMode(MODE_PRESET);
}

void loop() {
  unsigned long sekarang = millis();
  KejadianTombol s1 = periksaTombol(tombolS1, sekarang);
  KejadianTombol s2 = periksaTombol(tombolS2, sekarang);

  // Transisi: tekan lama S1 di mode mana pun → mode berikutnya
  if (s1 == TEKAN_LAMA) {
    pindahMode((Mode)((mode + 1) % JUMLAH_MODE));
  }

  // Aksi selama berada di mode tertentu
  switch (mode) {
    case MODE_PRESET:
      if (s1 == TEKAN_SINGKAT && indeksPreset < JUMLAH_PRESET - 1) {
        pilihPreset(indeksPreset + 1);
      }
      if (s2 == TEKAN_SINGKAT && indeksPreset > 0) {
        pilihPreset(indeksPreset - 1);
      }
      break;

    case MODE_SAPU:
      if (s2 == TEKAN_SINGKAT) {
        jedaGerakMs = (jedaGerakMs == 15) ? 5 : 15;
        Serial.printf("  Kecepatan: 1 derajat setiap %lu ms\n", jedaGerakMs);
      }
      if (sudutSekarang == sudutTarget) {  // sampai di ujung → berbalik
        sudutTarget = (sudutTarget == 180) ? 0 : 180;
      }
      break;

    case MODE_POT:
      if (sekarang - waktuPotLalu >= INTERVAL_POT_MS) {
        waktuPotLalu = sekarang;
        adcHalus = 0.2 * analogRead(PIN_POT) + 0.8 * adcHalus;  // EMA
        int sudutPot = map((int)(adcHalus + 0.5), 0, 4095, 0, 180);
        if (abs(sudutPot - sudutTarget) >= 2 || sudutPot == 0 || sudutPot == 180) {
          sudutTarget = sudutPot;
        }
      }
      break;

    case MODE_PARKIR:
      if (sudutSekarang == 90 && servo.attached()) {
        servo.detach();                // berhenti mengirim pulsa
        pinMode(PIN_SERVO, OUTPUT);    // tahan jalur sinyal LOW
        digitalWrite(PIN_SERVO, LOW);  // supaya servo tidak menangkap gangguan
        Serial.println("  Sudah di 90 derajat, sinyal dilepas (lengan bisa diputar dengan tangan)");
      }
      break;

    default:
      break;
  }

  gerakHalus(sekarang);
}
