/*
 * BAB 21 - Program 4: Watchdog untuk loop() dan Penyebab Reset
 * - Saat mulai, menampilkan penyebab reset terakhir (esp_reset_reason()).
 * - Mengaktifkan task watchdog untuk loop() dengan batas 5 detik.
 * - Tekan tombol S1 untuk membuat program macet -> watchdog me-reset ESP32.
 *
 * Rangkaian: Tombol S1 (SW1) → IO15
 * Alat uji : Serial Monitor, baud 115200
 */

#define TOMBOL_S1 15  // SW1 → IO15 (pull-down 10 kΩ di kit, ditekan = HIGH)

unsigned long waktuLapor = 0;  // kapan laporan terakhir dicetak
unsigned long detik = 0;       // berapa detik program sudah berjalan normal

// Menerjemahkan hasil esp_reset_reason() menjadi teks yang mudah dibaca
const char *namaPenyebabReset(esp_reset_reason_t alasan) {
  switch (alasan) {
    case ESP_RST_POWERON:   return "POWERON (baru dinyalakan / tombol EN / upload)";
    case ESP_RST_SW:        return "SW (ESP.restart() dari program)";
    case ESP_RST_PANIC:     return "PANIC (crash: Guru Meditation / abort)";
    case ESP_RST_INT_WDT:   return "INT_WDT (interrupt watchdog)";
    case ESP_RST_TASK_WDT:  return "TASK_WDT (task watchdog: program macet)";
    case ESP_RST_WDT:       return "WDT (watchdog lain)";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP (bangun dari deep sleep)";
    case ESP_RST_BROWNOUT:  return "BROWNOUT (tegangan catu daya turun)";
    default:                return "lainnya";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(TOMBOL_S1, INPUT);

  Serial.println("BAB 21 - Program 4: Watchdog dan Penyebab Reset");
  Serial.print("Penyebab reset terakhir: ");
  Serial.println(namaPenyebabReset(esp_reset_reason()));

  enableLoopWDT();  // mulai sekarang loop() diawasi: macet > 5 detik -> reset
  Serial.println("Watchdog loop() aktif (batas 5 detik). Tekan S1 untuk membuat program macet.");
}

void loop() {
  if (millis() - waktuLapor >= 1000) {
    waktuLapor = millis();
    detik = detik + 1;
    Serial.print("Berjalan normal, detik ke-");
    Serial.println(detik);
  }

  if (digitalRead(TOMBOL_S1) == HIGH) {
    Serial.println("S1 ditekan: program macet di while (true)...");
    while (true) {
      // Program terjebak di sini: loop() tidak pernah selesai,
      // jadi watchdog tidak pernah "diberi makan".
    }
  }
}
