/*
 * BAB 38 - Program 4: Preset Posisi dengan Tombol
 * S1 → preset berikutnya, S2 → preset sebelumnya.
 * Servo bergerak halus 1° setiap 10 ms menuju sudut preset,
 * dan tombol tetap bisa ditekan kapan saja selama servo bergerak.
 *
 * Rangkaian:
 *   SRV (header hijau, paling bawah) → IO32, servo di header servo kit
 *   SW1 (header kuning, pin ke-3)     → IO15
 *   SW2 (header kuning, pin ke-4)     → IO25
 */

#include <ESP32Servo.h>

#define PIN_SERVO 32  // SRV → IO32
#define PIN_S1 15     // SW1 → IO15 (ditekan = HIGH)
#define PIN_S2 25     // SW2 → IO25 (ditekan = HIGH)

const int PULSA_MIN_US = 500;   // ← ganti dengan hasil kalibrasi Program 2
const int PULSA_MAX_US = 2400;  // ← ganti dengan hasil kalibrasi Program 2

const int PRESET[] = {0, 45, 90, 135, 180};  // sudut-sudut preset
const int JUMLAH_PRESET = sizeof(PRESET) / sizeof(PRESET[0]);

const unsigned long JEDA_GERAK_MS = 10;  // 1° setiap 10 ms → 180° dalam ±1,8 detik
const unsigned long DEBOUNCE_MS = 50;

// Data satu tombol untuk debounce (ditulis sebelum fungsi mana pun)
struct Tombol {
  uint8_t pin;
  bool bacaanLalu;             // bacaan mentah sebelumnya
  bool stabil;                 // keadaan yang sudah pasti (true = ditekan)
  unsigned long waktuBerubah;  // kapan bacaan mentah terakhir berubah
};

Tombol tombolS1 = {PIN_S1, false, false, 0};
Tombol tombolS2 = {PIN_S2, false, false, 0};

Servo servo;

int indeksPreset = 2;    // mulai dari preset ketiga (90°)
int sudutSekarang = 90;  // sudut yang sedang dikirim ke servo
int sudutTarget = 90;    // sudut tujuan
unsigned long waktuGerakLalu = 0;

// Mengembalikan true SEKALI saat tombol baru ditekan (sudah di-debounce)
bool baruDitekan(Tombol &t, unsigned long sekarang) {
  bool bacaan = (digitalRead(t.pin) == HIGH);
  if (bacaan != t.bacaanLalu) {
    t.bacaanLalu = bacaan;
    t.waktuBerubah = sekarang;
  }
  if (sekarang - t.waktuBerubah >= DEBOUNCE_MS && bacaan != t.stabil) {
    t.stabil = bacaan;
    return t.stabil;  // true hanya saat berubah dari dilepas menjadi ditekan
  }
  return false;
}

void pilihPreset(int indeks) {
  indeksPreset = indeks;
  sudutTarget = PRESET[indeksPreset];
  Serial.printf("Preset %d dari %d -> target %d derajat\n",
                indeksPreset + 1, JUMLAH_PRESET, sudutTarget);
}

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 38 - Program 4: Preset Posisi dengan Tombol");
  pinMode(PIN_S1, INPUT);  // kit sudah punya resistor pull-down 10 kΩ
  pinMode(PIN_S2, INPUT);

  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, PULSA_MIN_US, PULSA_MAX_US);
  if (!servo.attached()) {
    Serial.println("Gagal memasang servo di IO32! Periksa nomor pin.");
    while (true) {
      delay(1000);  // program berhenti di sini
    }
  }
  servo.write(sudutSekarang);

  Serial.println("S1 = preset berikutnya, S2 = preset sebelumnya");
  pilihPreset(indeksPreset);
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: baca tombol dan pilih preset
  if (baruDitekan(tombolS1, sekarang)) {
    if (indeksPreset < JUMLAH_PRESET - 1) {
      pilihPreset(indeksPreset + 1);
    } else {
      Serial.println("Sudah di preset terakhir");
    }
  }
  if (baruDitekan(tombolS2, sekarang)) {
    if (indeksPreset > 0) {
      pilihPreset(indeksPreset - 1);
    } else {
      Serial.println("Sudah di preset pertama");
    }
  }

  // Tugas 2: gerakkan servo 1° menuju target setiap JEDA_GERAK_MS
  if (sudutSekarang != sudutTarget && sekarang - waktuGerakLalu >= JEDA_GERAK_MS) {
    waktuGerakLalu = sekarang;
    if (sudutTarget > sudutSekarang) {
      sudutSekarang = sudutSekarang + 1;
    } else {
      sudutSekarang = sudutSekarang - 1;
    }
    servo.write(sudutSekarang);

    if (sudutSekarang == sudutTarget) {
      Serial.printf("Sampai di %d derajat\n", sudutSekarang);
    }
  }
}
