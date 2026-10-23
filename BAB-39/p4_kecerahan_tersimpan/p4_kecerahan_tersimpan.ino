/*
 * BAB 39 - Program 4: Kecerahan LED Tersimpan
 * Putar potensiometer untuk mengatur kecerahan LED merah, lalu tekan S1
 * untuk menyimpannya ke NVS. Saat ESP32 menyala lagi, LED memakai
 * kecerahan tersimpan sampai kenop diputar.
 *
 * Rangkaian (header kit → pin ESP32):
 *   VR10K (potensiometer) → IO34
 *   SW1 (tombol S1)       → IO15
 *   LED1 (merah)          → IO4
 * Alat uji: Serial Monitor, baud 115200
 */

#include <Preferences.h>

#define POT_PIN   34
#define TOMBOL_S1 15
#define LED_MERAH 4

const uint32_t FREKUENSI = 5000;  // PWM 5 kHz (BAB 12)
const uint8_t  RESOLUSI  = 8;     // 8 bit → duty 0-255

const int AMBANG_GESER = 5;       // kenop dianggap diputar kalau bergeser >= 5 %
const unsigned long WAKTU_DEBOUNCE = 50;

// Timer non-blocking (pola BAB 16)
struct Timer {
  unsigned long interval;
  unsigned long waktuLalu;

  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerKenop   = {50, 0};    // baca kenop setiap 50 ms
Timer timerLaporan = {1000, 0};  // laporan ke Serial Monitor setiap 1 detik

Preferences prefs;

int kecerahanTersimpan = 50;  // persen; diisi dari NVS di setup()
int persenAwal = 0;           // posisi kenop saat ESP32 menyala
bool ikutiKenop = false;      // false: LED masih memakai nilai tersimpan
int persenAktif = 0;          // kecerahan LED saat ini (persen)

// Variabel debounce tombol S1 (pola BAB 9)
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

// Posisi kenop dalam persen (0-100), dirata-rata 8 kali supaya lebih tenang
int bacaKenopPersen() {
  long total = 0;
  for (int i = 0; i < 8; i++) {
    total = total + analogRead(POT_PIN);
  }
  return map(total / 8, 0, 4095, 0, 100);
}

void aturLed(int persen) {
  ledcWrite(LED_MERAH, map(persen, 0, 100, 0, 255));
}

// Simpan atas perintah + tulis saat berubah
void simpanKecerahan() {
  if (persenAktif == kecerahanTersimpan) {
    Serial.printf("S1 ditekan: %d %% sudah tersimpan, tidak perlu menulis ke flash.\n", persenAktif);
    return;
  }
  prefs.begin("peredup", false);
  size_t tertulis = prefs.putUChar("kecerahan", (uint8_t)persenAktif);
  prefs.end();

  if (tertulis > 0) {
    kecerahanTersimpan = persenAktif;
    Serial.printf("S1 ditekan: kecerahan %d %% disimpan ke NVS.\n", persenAktif);
  } else {
    Serial.println("Gagal menyimpan ke NVS!");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);   // tombol kit punya pull-down 10 kΩ: ditekan = HIGH
  delay(1000);

  Serial.println();
  Serial.println("BAB 39 - Program 4: Kecerahan LED Tersimpan");

  if (!ledcAttach(LED_MERAH, FREKUENSI, RESOLUSI)) {
    Serial.println("Gagal mengatur PWM di IO4!");
    while (true) {
      delay(1000);
    }
  }

  prefs.begin("peredup", false);   // baca-tulis: namespace dibuat kalau belum ada
  kecerahanTersimpan = prefs.getUChar("kecerahan", 50);  // 50 % kalau belum pernah disimpan
  prefs.end();
  if (kecerahanTersimpan > 100) {
    kecerahanTersimpan = 100;      // jaga-jaga kalau isinya tidak masuk akal
  }

  persenAwal = bacaKenopPersen();
  persenAktif = kecerahanTersimpan;
  aturLed(persenAktif);

  Serial.printf("Kecerahan tersimpan: %d %%\n", kecerahanTersimpan);
  Serial.println("Putar potensiometer untuk mengubah, tekan S1 untuk menyimpan.");
}

void loop() {
  unsigned long sekarang = millis();

  // 1. Baca kenop setiap 50 ms
  if (timerKenop.sudahWaktunya(sekarang)) {
    int persenKenop = bacaKenopPersen();
    if (!ikutiKenop && abs(persenKenop - persenAwal) >= AMBANG_GESER) {
      ikutiKenop = true;           // kenop sudah diputar: mulai sekarang LED ikut kenop
      Serial.println("Kenop diputar: LED sekarang mengikuti potensiometer.");
    }
    if (ikutiKenop) {
      persenAktif = persenKenop;
      aturLed(persenAktif);
    }
  }

  // 2. Tombol S1 dengan debounce: simpan kecerahan saat ditekan
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = sekarang;
    bacaanSebelumnya = bacaan;
  }
  if ((sekarang - waktuBerubah) > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    if (statusStabil == HIGH) {
      simpanKecerahan();
    }
  }

  // 3. Laporan setiap 1 detik
  if (timerLaporan.sudahWaktunya(sekarang)) {
    Serial.printf("LED: %3d %% | tersimpan: %3d %%%s\n", persenAktif, kecerahanTersimpan,
                  (persenAktif != kecerahanTersimpan) ? "  (belum disimpan)" : "");
  }
}
