/*
 * BAB 13 - Program 2: Gelombang di Serial Plotter
 * DAC1 (IO25) membuat gelombang lambat secara bergantian:
 * gigi gergaji, segitiga, lalu sinus (masing-masing 3 gelombang).
 * ADC di IO32 membaca balik tegangannya.
 * Buka Tools → Serial Plotter untuk melihat grafiknya.
 *
 * Rangkaian: kabel jumper dari pin header 25 ke pin header 32 (sama dengan Program 1)
 */

#define DAC_PIN  25  // DAC1 → IO25
#define BACA_PIN 32  // ADC1 → IO32, membaca balik tegangan DAC

const int TITIK_PER_GELOMBANG = 25;  // 25 titik: dua gelombang muat di layar Serial Plotter
const int JEDA_MS = 80;              // 25 titik × 80 ms = 2 detik per gelombang
const int JUMLAH_GELOMBANG = 3;      // setiap bentuk diulang 3 kali

// Menghitung nilai DAC (0–255) untuk titik ke-i dari bentuk gelombang tertentu
int hitungNilai(int bentuk, int i) {
  float posisi = (float)i / TITIK_PER_GELOMBANG;  // dari 0.0 sampai hampir 1.0

  if (bentuk == 0) {          // gigi gergaji: naik lurus, lalu jatuh
    return posisi * 255;
  } else if (bentuk == 1) {   // segitiga: naik lurus, lalu turun lurus
    if (posisi < 0.5) {
      return posisi * 2 * 255;
    }
    return (1.0 - posisi) * 2 * 255;
  } else {                    // sinus: naik-turun melengkung, di tengah 127,5
    return 127.5 + 127.5 * sin(2 * PI * posisi);
  }
}

void setup() {
  Serial.begin(115200);
  // Sengaja tidak mencetak judul supaya grafik di Serial Plotter tetap bersih
}

void loop() {
  for (int bentuk = 0; bentuk < 3; bentuk++) {
    for (int g = 0; g < JUMLAH_GELOMBANG; g++) {
      for (int i = 0; i < TITIK_PER_GELOMBANG; i++) {
        int nilai = hitungNilai(bentuk, i);
        dacWrite(DAC_PIN, nilai);
        delay(JEDA_MS);

        int target = nilai * 3300 / 255;               // tegangan ideal (mV)
        int terukur = analogReadMilliVolts(BACA_PIN);  // tegangan terbaca ADC (mV)

        // Format Serial Plotter: nama:nilai,nama:nilai
        Serial.print("target_mV:");
        Serial.print(target);
        Serial.print(",terukur_mV:");
        Serial.println(terukur);
      }
    }
  }
}
