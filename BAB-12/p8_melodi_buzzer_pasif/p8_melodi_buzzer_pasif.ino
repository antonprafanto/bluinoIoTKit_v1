/*
 * BAB 12 - Program 8 (Opsional): Melodi dengan Buzzer Pasif
 * Memainkan tangga nada do-re-mi-fa-sol-la-si-do.
 * BUTUH buzzer pasif tambahan (tidak termasuk kit).
 * Buzzer aktif bawaan kit TIDAK bisa memainkan melodi.
 *
 * Rangkaian (modul buzzer pasif):
 *   S (sinyal) → IO13   (lepas dulu kabel BUZZ kit dari IO13)
 *   − (GND)    → G
 */

#define BUZZER_PIN 13

// Frekuensi nada dalam Hz (dibulatkan): C4, D4, E4, F4, G4, A4, B4, C5
const uint32_t NADA[] = {262, 294, 330, 349, 392, 440, 494, 523};
const char *NAMA_NADA[] = {"do", "re", "mi", "fa", "sol", "la", "si", "do (tinggi)"};
const int JUMLAH_NADA = sizeof(NADA) / sizeof(NADA[0]);
const int LAMA_NADA_MS = 400;  // lama setiap nada

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12 - Program 8: Melodi dengan Buzzer Pasif");

  // Frekuensi awal bebas (di sini 1000 Hz); ledcWriteTone() akan menggantinya
  if (!ledcAttach(BUZZER_PIN, 1000, 10)) {
    Serial.println("Gagal mengatur PWM di IO13! Periksa frekuensi dan resolusi.");
    while (true) {
      delay(1000);
    }
  }
  ledcWrite(BUZZER_PIN, 0);  // diam dulu
}

void loop() {
  for (int i = 0; i < JUMLAH_NADA; i++) {
    ledcWriteTone(BUZZER_PIN, NADA[i]);  // bunyikan nada ke-i

    Serial.print(NAMA_NADA[i]);
    Serial.print(" = ");
    Serial.print(NADA[i]);
    Serial.println(" Hz");

    delay(LAMA_NADA_MS);
    ledcWriteTone(BUZZER_PIN, 0);  // diam sebentar supaya setiap nada terdengar terpisah
    delay(50);
  }

  delay(2000);  // jeda sebelum mengulang
}
