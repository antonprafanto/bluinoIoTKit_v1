/*
 * BAB 20 - Program 5: Variabel yang Bertahan Saat Deep Sleep
 * jumlahBangun disimpan di memori RTC (RTC_DATA_ATTR), sehingga nilainya
 * bertahan saat ESP32 deep sleep. jumlahBiasa disimpan di RAM biasa,
 * sehingga selalu mulai dari 0 setiap kali ESP32 bangun.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino (tidak perlu merangkai)
 * Alat uji : Serial Monitor, baud 115200
 */

const uint64_t LAMA_TIDUR_US = 5ULL * 1000000ULL;  // 5 detik, dalam mikrodetik

RTC_DATA_ATTR int jumlahBangun = 0;  // memori RTC: bertahan saat deep sleep
int jumlahBiasa = 0;                 // RAM biasa: hilang saat deep sleep

void setup() {
  Serial.begin(115200);
  delay(500);  // beri waktu Serial Monitor menangkap teks setelah bangun

  jumlahBangun = jumlahBangun + 1;
  jumlahBiasa = jumlahBiasa + 1;

  Serial.println();
  Serial.println("BAB 20 - Program 5: RTC_DATA_ATTR dan Deep Sleep");
  Serial.print("Penyebab mulai : ");
  if (esp_reset_reason() == ESP_RST_DEEPSLEEP) {
    Serial.println("bangun dari deep sleep");
  } else {
    Serial.println("dinyalakan / tombol EN / upload");
  }
  Serial.print("jumlahBangun (RTC_DATA_ATTR) : ");
  Serial.println(jumlahBangun);
  Serial.print("jumlahBiasa  (RAM biasa)     : ");
  Serial.println(jumlahBiasa);

  Serial.println("Tidur 5 detik...");
  Serial.flush();  // tunggu sampai semua teks terkirim sebelum tidur

  esp_sleep_enable_timer_wakeup(LAMA_TIDUR_US);  // pasang "alarm" bangun
  esp_deep_sleep_start();                        // tidur sekarang
}

void loop() {
  // Tidak pernah dijalankan: setelah deep sleep, ESP32 selalu mulai dari setup().
}
