/*
 * BAB 13 - Program 3: LED dengan Tegangan DAC
 * DAC2 (IO26) menaikkan tegangan ke LED merah sedikit demi sedikit,
 * lalu mematikannya dan mengulang. Amati: LED baru mulai menyala
 * setelah tegangannya melewati tegangan maju (Vf) LED.
 *
 * Rangkaian: LED1 (merah) di header LED → IO26 (bukan IO4!)
 *            Cabut dulu kabel RELAY dari IO26.
 */

#define DAC_LED 26  // DAC2 → IO26 → LED merah

const int LANGKAH = 15;    // kenaikan nilai DAC setiap langkah
const int JEDA_MS = 1000;  // lama setiap langkah: 1 detik

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 13 - Program 3: LED dengan Tegangan DAC");

  if (!dacWrite(DAC_LED, 0)) {
    Serial.println("Gagal! DAC hanya ada di IO25 dan IO26.");
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  for (int nilai = 0; nilai <= 255; nilai += LANGKAH) {
    dacWrite(DAC_LED, nilai);

    float tegangan = nilai * 3.3 / 255;  // perkiraan tegangan di IO26 (volt)
    Serial.print("Nilai DAC ");
    Serial.print(nilai);
    Serial.print(" -> ");
    Serial.print(tegangan, 2);
    Serial.println(" V");

    delay(JEDA_MS);
  }

  dacWrite(DAC_LED, 0);  // matikan LED sebelum mengulang
  Serial.println("--- LED dimatikan, ulang dari awal ---");
  delay(2000);
}
