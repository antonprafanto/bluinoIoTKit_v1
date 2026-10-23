/*
 * BAB 12 - Program 2: LED Bernapas
 * LED merah perlahan makin terang (fade in), lalu perlahan meredup (fade out),
 * berulang terus.
 *
 * Rangkaian: LED1 (merah) di header LED → IO4
 */

#define LED_MERAH 4  // LED merah → IO4

const uint32_t FREKUENSI = 5000;  // 5 kHz
const uint8_t RESOLUSI = 8;       // nilai duty 0–255
const int JEDA_MS = 8;            // jeda setiap langkah; makin kecil, napasnya makin cepat

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12 - Program 2: LED Bernapas");

  if (!ledcAttach(LED_MERAH, FREKUENSI, RESOLUSI)) {
    Serial.println("Gagal mengatur PWM di IO4! Periksa frekuensi dan resolusi.");
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  Serial.println("Makin terang...");
  for (int duty = 0; duty <= 255; duty++) {   // fade in: 0, 1, 2, ..., 255
    ledcWrite(LED_MERAH, duty);
    delay(JEDA_MS);
  }

  Serial.println("Makin redup...");
  for (int duty = 255; duty >= 0; duty--) {   // fade out: 255, 254, ..., 0
    ledcWrite(LED_MERAH, duty);
    delay(JEDA_MS);
  }

  delay(500);  // istirahat sebentar dalam keadaan mati
}
