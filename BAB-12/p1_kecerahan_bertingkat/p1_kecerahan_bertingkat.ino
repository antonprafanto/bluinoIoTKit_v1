/*
 * BAB 12 - Program 1: Kecerahan LED Bertingkat
 * LED merah menyala dengan duty cycle 0%, 10%, 25%, 50%, 75%, dan 100%,
 * masing-masing selama 2 detik, lalu mengulang dari awal.
 *
 * Rangkaian: LED1 (merah) di header LED → IO4
 */

#define LED_MERAH 4  // LED merah → IO4

const uint32_t FREKUENSI = 5000;  // 5 kHz: kedipannya terlalu cepat untuk terlihat mata
const uint8_t RESOLUSI = 8;       // 8 bit → nilai duty 0–255
const int DUTY_MAKS = 255;        // nilai duty terbesar pada resolusi 8 bit

const int DAFTAR_PERSEN[] = {0, 10, 25, 50, 75, 100};
const int JUMLAH_LANGKAH = sizeof(DAFTAR_PERSEN) / sizeof(DAFTAR_PERSEN[0]);

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12 - Program 1: Kecerahan LED Bertingkat");

  // Sambungkan IO4 ke LEDC: frekuensi 5 kHz, resolusi 8 bit
  if (!ledcAttach(LED_MERAH, FREKUENSI, RESOLUSI)) {
    Serial.println("Gagal mengatur PWM di IO4! Periksa frekuensi dan resolusi.");
    while (true) {
      delay(1000);  // program berhenti di sini
    }
  }
}

void loop() {
  for (int i = 0; i < JUMLAH_LANGKAH; i++) {
    int persen = DAFTAR_PERSEN[i];
    int duty = round(persen * DUTY_MAKS / 100.0);  // ubah persen menjadi nilai duty

    ledcWrite(LED_MERAH, duty);  // LEDC terus membuat PWM dengan duty ini

    Serial.print("Duty cycle ");
    Serial.print(persen);
    Serial.print("% -> nilai duty ");
    Serial.print(duty);
    Serial.println(" dari 255");

    delay(2000);  // tahan 2 detik supaya sempat diamati
  }
}
