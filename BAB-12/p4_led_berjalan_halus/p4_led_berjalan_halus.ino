/*
 * BAB 12 - Program 4: LED Berjalan Halus
 * Cahaya berpindah dengan halus dari satu LED ke LED di sebelahnya:
 * LED sekarang meredup sementara LED berikutnya makin terang.
 *
 * Rangkaian (header LED di kit → pin ESP32):
 *   LED1 (merah)  → IO4
 *   LED2 (hijau)  → IO17
 *   LED3 (biru)   → IO2
 *   LED4 (kuning) → IO16
 */

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2

// Urutan sesuai posisi LED di kit, dari kiri ke kanan
const int LED_PINS[] = {LED_MERAH, LED_HIJAU, LED_BIRU, LED_KUNING};
const char *NAMA_LED[] = {"merah", "hijau", "biru", "kuning"};
const int JUMLAH_LED = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

const uint32_t FREKUENSI = 5000;  // keempat LED memakai frekuensi dan resolusi yang sama
const uint8_t RESOLUSI = 8;       // nilai duty 0–255
const int LANGKAH = 5;            // besar perubahan duty setiap langkah
const int JEDA_MS = 15;           // jeda setiap langkah

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12 - Program 4: LED Berjalan Halus");

  // Sambungkan keempat pin ke LEDC; setiap pin mendapat channel sendiri
  for (int i = 0; i < JUMLAH_LED; i++) {
    if (!ledcAttach(LED_PINS[i], FREKUENSI, RESOLUSI)) {
      Serial.print("Gagal mengatur PWM di IO");
      Serial.println(LED_PINS[i]);
    }
  }

  ledcWrite(LED_PINS[0], 255);  // mulai dengan LED merah menyala penuh
}

void loop() {
  for (int i = 0; i < JUMLAH_LED; i++) {
    int berikut = (i + 1) % JUMLAH_LED;  // LED di sebelahnya; setelah LED terakhir kembali ke 0

    Serial.print("Pindah dari LED ");
    Serial.print(NAMA_LED[i]);
    Serial.print(" ke LED ");
    Serial.println(NAMA_LED[berikut]);

    for (int duty = 0; duty <= 255; duty += LANGKAH) {
      ledcWrite(LED_PINS[berikut], duty);  // LED berikutnya makin terang
      ledcWrite(LED_PINS[i], 255 - duty);  // LED sekarang makin redup
      delay(JEDA_MS);
    }
  }
}
