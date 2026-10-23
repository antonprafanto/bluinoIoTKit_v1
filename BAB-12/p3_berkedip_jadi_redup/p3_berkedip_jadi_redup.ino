/*
 * BAB 12 - Program 3: Dari Berkedip Menjadi Redup
 * Duty cycle tetap 50%, tetapi frekuensi PWM dinaikkan bertahap dari 1 Hz
 * sampai 1000 Hz. Pada frekuensi rendah LED terlihat berkedip; pada frekuensi
 * tinggi LED terlihat menyala stabil dengan kecerahan setengah.
 *
 * Rangkaian: LED1 (merah) di header LED → IO4
 */

#define LED_MERAH 4  // LED merah → IO4

const uint8_t RESOLUSI = 10;         // 10 bit → nilai duty 0–1023 (cukup untuk frekuensi 1 Hz)
const uint32_t DUTY_SETENGAH = 512;  // 512 dari 1024 tingkat = 50%
const uint32_t DAFTAR_FREKUENSI[] = {1, 2, 5, 10, 25, 50, 100, 1000};
const int JUMLAH_LANGKAH = sizeof(DAFTAR_FREKUENSI) / sizeof(DAFTAR_FREKUENSI[0]);
const unsigned long LAMA_AMATI_MS = 4000;  // lama mengamati setiap frekuensi

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12 - Program 3: Dari Berkedip Menjadi Redup");

  // Mulai dari frekuensi pertama di daftar (1 Hz)
  if (!ledcAttach(LED_MERAH, DAFTAR_FREKUENSI[0], RESOLUSI)) {
    Serial.println("Gagal mengatur PWM di IO4! Periksa frekuensi dan resolusi.");
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  for (int i = 0; i < JUMLAH_LANGKAH; i++) {
    // Ganti frekuensi. Hasilnya = frekuensi yang benar-benar dipakai (0 = gagal)
    uint32_t frekuensi = ledcChangeFrequency(LED_MERAH, DAFTAR_FREKUENSI[i], RESOLUSI);
    if (frekuensi == 0) {
      Serial.println("Gagal mengganti frekuensi!");
      continue;  // lewati frekuensi ini
    }
    ledcWrite(LED_MERAH, DUTY_SETENGAH);  // duty cycle tetap 50%

    float periodeMs = 1000.0 / frekuensi;  // periode = 1 ÷ frekuensi, dalam milidetik

    Serial.print("Frekuensi ");
    Serial.print(frekuensi);
    Serial.print(" Hz -> menyala ");
    Serial.print(periodeMs / 2, 1);
    Serial.print(" ms, mati ");
    Serial.print(periodeMs / 2, 1);
    Serial.println(" ms");

    delay(LAMA_AMATI_MS);
  }
}
