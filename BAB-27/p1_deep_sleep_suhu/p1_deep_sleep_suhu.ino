/*
 * BAB 27 - Program 1: Termometer Deep Sleep
 * Siklus: bangun → baca DS18B20 → simpan ke memori RTC → tidur 10 detik → ulangi.
 * Penghitung bangun dan 5 suhu terakhir disimpan di memori RTC (RTC_DATA_ATTR),
 * jadi tidak hilang selama deep sleep.
 *
 * Rangkaian: pin DS (header hijau) → IO14 (pull-up 4,7 kΩ sudah ada di PCB)
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ONEWIRE      14  // jalur data DS18B20 (pin DS -> IO14)
#define RESOLUSI         12  // 9 bit = bangun lebih singkat; 12 bit = langkah 0,0625 °C
#define LAMA_TIDUR_DETIK 10  // untuk belajar; pemakaian nyata misalnya 900 (15 menit)
#define JUMLAH_RIWAYAT    5  // banyaknya suhu terakhir yang diingat

// ---- Disimpan di memori RTC: tetap ada selama deep sleep ----
RTC_DATA_ATTR int jumlahBangun = 0;
RTC_DATA_ATTR float riwayatSuhu[JUMLAH_RIWAYAT];
RTC_DATA_ATTR int jumlahData = 0;   // berapa suhu yang sudah tersimpan (maks. JUMLAH_RIWAYAT)
RTC_DATA_ATTR int posisiTulis = 0;  // tempat suhu berikutnya akan ditulis

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensor(&oneWire);

// true kalau angka dari DS18B20 masuk akal
bool suhuValid(float suhu) {
  if (suhu == DEVICE_DISCONNECTED_C) return false;  // -127: gagal atau belum sempat mengukur
  if (suhu < -55.0 || suhu > 125.0) return false;   // di luar rentang DS18B20 (kode error lain)
  if (suhu == 85.0) return false;                   // nilai awal sensor (pada sebagian sensor tiruan)
  return true;
}

void setup() {
  Serial.begin(115200);
  jumlahBangun = jumlahBangun + 1;

  Serial.println();
  Serial.printf("=== Bangun ke-%d ===\n", jumlahBangun);
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Penyebab: timer deep sleep");
  } else {
    Serial.println("Penyebab: baru dinyalakan atau di-reset (memori RTC kembali ke nilai awal)");
  }

  // Baca DS18B20 dengan cara menunggu: selama bangun tidak ada pekerjaan lain
  sensor.begin();
  sensor.setAutoSaveScratchPad(false);  // resolusi tidak disimpan permanen ke EEPROM sensor
  sensor.setResolution(RESOLUSI);
  sensor.requestTemperatures();         // tunggu sampai pengukuran selesai
  float suhu = sensor.getTempCByIndex(0);

  if (suhuValid(suhu)) {
    riwayatSuhu[posisiTulis] = suhu;                   // simpan di posisi berikutnya
    posisiTulis = (posisiTulis + 1) % JUMLAH_RIWAYAT;  // buffer melingkar
    if (jumlahData < JUMLAH_RIWAYAT) {
      jumlahData = jumlahData + 1;
    }
    Serial.printf("Suhu sekarang: %.2f °C\n", suhu);
  } else {
    Serial.printf("Gagal membaca sensor (%.2f). Periksa kabel DS -> 14.\n", suhu);
  }

  // Tampilkan riwayat dari yang paling lama ke yang paling baru
  Serial.printf("Riwayat %d suhu terakhir (lama -> baru):", jumlahData);
  for (int i = 0; i < jumlahData; i++) {
    int indeks = (posisiTulis - jumlahData + i + JUMLAH_RIWAYAT) % JUMLAH_RIWAYAT;
    Serial.printf(" %.2f", riwayatSuhu[indeks]);
  }
  Serial.println();

  Serial.printf("Lama bangun: %lu ms. Tidur %d detik...\n", millis(), LAMA_TIDUR_DETIK);
  Serial.flush();  // tunggu semua teks terkirim sebelum tidur

  esp_sleep_enable_timer_wakeup((uint64_t)LAMA_TIDUR_DETIK * 1000000ULL);  // dalam mikrodetik
  esp_deep_sleep_start();  // tidur sekarang; baris di bawah ini tidak pernah dijalankan
}

void loop() {
  // Tidak pernah dijalankan: setiap kali bangun, ESP32 mulai lagi dari setup().
}
