/*
 * BAB 14 - Program 5: Bangun dari Deep Sleep dengan Sentuhan
 * ESP32 tidur (deep sleep) dan bangun saat touch pad disentuh.
 * Jumlah bangun disimpan di memori RTC supaya tidak hilang selama tidur.
 *
 * Rangkaian:
 *   TOUCH (touch pad, header kuning) → IO33 (T8)
 *   LED1 (LED merah, header LED)     → IO4
 */

#define TOUCH_PIN T8  // T8 = IO33
#define LED_MERAH 4

const uint32_t PERSEN_AMBANG = 70;  // bangun jika nilai < 70% nilai tanpa sentuhan

RTC_DATA_ATTR int jumlahBangun = 0;  // disimpan di memori RTC, tetap ada selama deep sleep

// Callback kosong: hanya dipakai agar touchAttachInterrupt() bisa menyetel ambang
void callbackKosong() {}

// Rata-rata beberapa bacaan supaya derau teredam
uint32_t bacaRataRata(int jumlah) {
  uint32_t total = 0;
  for (int i = 0; i < jumlah; i++) {
    total += touchRead(TOUCH_PIN);
    delay(20);  // driver memperbarui nilai sekitar setiap 10 ms
  }
  return total / jumlah;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, HIGH);  // LED menyala = ESP32 sedang bangun
  delay(1000);                    // beri waktu Serial Monitor tersambung

  jumlahBangun++;
  Serial.println("BAB 14 - Program 5: Bangun dengan Sentuhan");
  Serial.print("Bangun ke-");
  Serial.println(jumlahBangun);

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TOUCHPAD) {
    Serial.println("Penyebab: touch pad disentuh");
  } else {
    Serial.println("Penyebab: baru dinyalakan atau di-reset");
  }

  Serial.println("Lepaskan jari dari pad. Tidur lagi dalam 3 detik...");
  delay(3000);

  // Ukur nilai tanpa sentuhan, lalu setel ambang bangun
  uint32_t nilaiLepas = bacaRataRata(25);
  uint32_t ambang = nilaiLepas * PERSEN_AMBANG / 100;
  touchAttachInterrupt(TOUCH_PIN, callbackKosong, ambang);  // menyetel ambang di driver
  touchSleepWakeUpEnable(TOUCH_PIN, ambang);                // pad ini menjadi sumber bangun

  Serial.print("Ambang bangun: ");
  Serial.println(ambang);
  Serial.println("Tidur... sentuh pad untuk membangunkan.");
  Serial.flush();                // tunggu sampai semua teks selesai terkirim
  digitalWrite(LED_MERAH, LOW);  // LED mati = ESP32 tidur

  esp_deep_sleep_start();        // mulai deep sleep; baris setelah ini tidak dijalankan
}

void loop() {
  // Tidak pernah dijalankan: setelah bangun, ESP32 mulai lagi dari setup()
}
