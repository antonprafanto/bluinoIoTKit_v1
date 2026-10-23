/*
 * BAB 12 - Program 7: Alarm Bip Makin Cepat (buzzer aktif di kit)
 * PWM berfrekuensi rendah menyalakan dan mematikan buzzer aktif:
 * 1 Hz = 1 bip per detik, 2 Hz = 2 bip per detik, dan seterusnya.
 * Nada bipnya tetap sama karena ditentukan osilator di dalam buzzer.
 *
 * Rangkaian: BUZZ → IO13
 */

#define BUZZER_PIN 13  // buzzer aktif (pin BUZZ) → IO13

const uint8_t RESOLUSI = 10;  // 10 bit: frekuensi serendah 1 Hz bisa dibuat
const uint32_t DAFTAR_BIP_PER_DETIK[] = {1, 2, 4, 8};
const int JUMLAH_LANGKAH = sizeof(DAFTAR_BIP_PER_DETIK) / sizeof(DAFTAR_BIP_PER_DETIK[0]);

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12 - Program 7: Alarm Bip Makin Cepat");

  // Pin harus di-attach dulu sebelum memakai ledcWriteTone()
  if (!ledcAttach(BUZZER_PIN, 1, RESOLUSI)) {
    Serial.println("Gagal mengatur PWM di IO13! Periksa frekuensi dan resolusi.");
    while (true) {
      delay(1000);
    }
  }
  ledcWrite(BUZZER_PIN, 0);  // mulai dalam keadaan diam
}

void loop() {
  for (int i = 0; i < JUMLAH_LANGKAH; i++) {
    uint32_t bip = DAFTAR_BIP_PER_DETIK[i];

    // Sinyal duty cycle 50%: separuh periode berbunyi, separuh periode diam
    if (ledcWriteTone(BUZZER_PIN, bip) == 0) {
      Serial.println("Gagal mengatur frekuensi!");
      continue;
    }

    Serial.print("Alarm: ");
    Serial.print(bip);
    Serial.println(" bip per detik");

    delay(3000);  // tahan 3 detik
  }

  ledcWriteTone(BUZZER_PIN, 0);  // frekuensi 0 = diam
  Serial.println("Diam 3 detik...");
  delay(3000);
}
