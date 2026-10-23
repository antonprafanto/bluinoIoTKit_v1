/*
 * BAB 31 - Program 5: Bangun dari Deep Sleep saat Ada Gerakan
 * ESP32 tidur (deep sleep) dan baru bangun saat keluaran PIR HIGH.
 * Setelah bangun: cetak jumlah bangun, pura-pura kirim notifikasi,
 * tunggu PIR kembali LOW, lalu tidur lagi.
 * LED biru bawaan board menyala selama ESP32 bangun.
 *
 * Rangkaian: PIR (header hijau, pin ke-5 dari atas) → IO33
 */

#define PIN_PIR  33  // IO33 termasuk pin RTC: bisa membangunkan ESP32
#define LED_BIRU 2

RTC_DATA_ATTR int jumlahBangunGerakan = 0;  // memori RTC: bertahan saat deep sleep

void setup() {
  Serial.begin(115200);
  delay(500);  // beri waktu Serial Monitor menangkap teks setelah bangun

  pinMode(PIN_PIR, INPUT);  // juga mengembalikan IO33 menjadi GPIO biasa setelah bangun
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, HIGH);  // LED menyala = ESP32 sedang bangun

  Serial.println();
  Serial.println("BAB 31 - Program 5: Bangun dari Deep Sleep saat Ada Gerakan");

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    jumlahBangunGerakan = jumlahBangunGerakan + 1;
    Serial.print("Bangun karena ada gerakan! Ini yang ke-");
    Serial.println(jumlahBangunGerakan);

    // Di proyek sungguhan, di sinilah ESP32 mengirim notifikasi lewat WiFi.
    Serial.println("Pura-pura mengirim notifikasi... (2 detik)");
    delay(2000);
  } else {
    Serial.println("Mulai karena dinyalakan / tombol EN / upload.");
  }

  // ext0 bereaksi pada LEVEL: kalau PIR masih HIGH saat tidur,
  // ESP32 langsung bangun lagi. Jadi tunggu dulu sampai PIR LOW.
  Serial.println("Menunggu sinyal PIR kembali LOW...");
  while (digitalRead(PIN_PIR) == HIGH) {
    delay(10);
  }

  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_PIR, 1);  // bangun saat IO33 = HIGH (1)
  Serial.println("Tidur... ESP32 bangun lagi saat PIR mendeteksi gerakan.");
  Serial.flush();  // tunggu sampai semua teks terkirim sebelum tidur
  digitalWrite(LED_BIRU, LOW);
  esp_deep_sleep_start();  // tidur sekarang; baris setelahnya tidak dijalankan
}

void loop() {
  // Tidak pernah dijalankan: setelah deep sleep, ESP32 selalu mulai dari setup().
}
