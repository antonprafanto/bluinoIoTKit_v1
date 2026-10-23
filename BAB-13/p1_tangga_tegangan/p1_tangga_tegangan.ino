/*
 * BAB 13 - Program 1: Tangga Tegangan
 * DAC1 (IO25) mengeluarkan 5 tingkat tegangan, masing-masing ±3 detik.
 * Tegangannya dibaca balik oleh ADC di IO32 lewat kabel jumper 25 → 32.
 *
 * Rangkaian:
 *   Kabel jumper dari pin header 25 ke pin header 32 (deretan atas)
 *   Pin 25 dan 32 tidak boleh tersambung ke komponen lain (cabut SW2, Tr, SRV)
 */

#define DAC_PIN  25  // DAC1 → IO25
#define BACA_PIN 32  // ADC1 → IO32, membaca balik tegangan DAC

const int DAFTAR_NILAI[] = {0, 64, 128, 192, 255};
const int JUMLAH_LANGKAH = sizeof(DAFTAR_NILAI) / sizeof(DAFTAR_NILAI[0]);

// Baca tegangan 16 kali lalu dirata-rata supaya hasilnya stabil (lihat BAB 11)
float bacaVolt(int pin) {
  uint32_t total = 0;
  for (int i = 0; i < 16; i++) {
    total += analogReadMilliVolts(pin);
  }
  return (total / 16) / 1000.0;  // mV → V
}

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 13 - Program 1: Tangga Tegangan");

  // dacWrite() mengembalikan false kalau DAC_PIN bukan pin DAC (harus 25 atau 26)
  if (!dacWrite(DAC_PIN, 0)) {
    Serial.println("Gagal! DAC hanya ada di IO25 dan IO26.");
    while (true) {
      delay(1000);  // program berhenti di sini
    }
  }
}

void loop() {
  for (int i = 0; i < JUMLAH_LANGKAH; i++) {
    int nilai = DAFTAR_NILAI[i];
    dacWrite(DAC_PIN, nilai);  // keluarkan tegangan di IO25
    delay(100);                // tunggu sebentar sampai tegangannya stabil

    float ideal = nilai * 3.3 / 255;     // hitungan rumus (volt)
    float terukur = bacaVolt(BACA_PIN);  // hasil ukur ADC di IO32 (volt)

    Serial.print("Nilai DAC ");
    Serial.print(nilai);
    Serial.print(" -> ideal ");
    Serial.print(ideal, 2);
    Serial.print(" V | terukur ADC ");
    Serial.print(terukur, 2);
    Serial.println(" V");

    delay(2900);  // tahan ±3 detik supaya sempat diamati atau diukur
  }
  Serial.println("--- ulang dari awal ---");
}
