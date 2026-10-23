/*
 * BAB 21 - Program 2: Log Bawaan dan Core Debug Level
 * Mencoba log_e() ... log_v() dan ESP_LOGx(), lalu memperlihatkan pesan
 * kesalahan dari core saat digitalWrite() dipanggil tanpa pinMode().
 *
 * PENTING  : pilih Tools → Core Debug Level → Info sebelum upload
 * Rangkaian: LED1 (merah) → IO4
 * Alat uji : Serial Monitor, baud 115200
 */

#define LED_MERAH 4  // LED1 (merah) → IO4

static const char *TAG = "TANGKI";  // label bebas untuk ESP_LOGx()

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BAB 21 - Program 2: Log Bawaan dan Core Debug Level");

  // 1. Lima tingkat log bawaan core (tampil sesuai Core Debug Level)
  log_e("Contoh pesan ERROR, kode = %d", 12);
  log_w("Contoh pesan WARNING");
  log_i("Contoh pesan INFO");
  log_d("Contoh pesan DEBUG");
  log_v("Contoh pesan VERBOSE");

  // 2. Gaya ESP-IDF: ESP_LOGx(TAG, ...) ikut diatur Core Debug Level
  ESP_LOGI(TAG, "Level air %d %%", 75);
  ESP_LOGW(TAG, "Sensor belum dikalibrasi");

  // 3. Kesalahan klasik: digitalWrite() tanpa pinMode()
  Serial.println("Menyalakan LED merah TANPA pinMode()...");
  digitalWrite(LED_MERAH, HIGH);  // gagal: IO4 belum diatur sebagai output
  delay(2000);

  Serial.println("Menyalakan LED merah DENGAN pinMode()...");
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, HIGH);  // berhasil: LED merah menyala
}

void loop() {
  static unsigned long waktuLog = 0;
  if (millis() - waktuLog >= 5000) {
    waktuLog = millis();
    log_i("Heap bebas: %lu byte", (unsigned long)ESP.getFreeHeap());
    log_d("millis() = %lu", millis());
  }
}
