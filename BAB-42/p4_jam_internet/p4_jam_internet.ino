/*
 * BAB 42 - Program 4: Jam Internet dan Log Suhu Bertanda Waktu
 * Mengambil waktu dari internet (NTP) dalam zona WIB/WITA/WIT, lalu
 * setiap 10 detik mencetak baris log berisi tanggal-jam dan bacaan DHT11.
 *
 * Rangkaian (opsional): pin DHT (header hijau) → IO27 dengan kabel jumper.
 *                       Tanpa DHT11 program tetap berjalan.
 * Library : DHT sensor library by Adafruit (+ Adafruit Unified Sensor)
 * Alat uji: Serial Monitor, baud 115200.
 * Jaringan: WiFi 2,4 GHz yang tersambung ke internet.
 */

#include <WiFi.h>
#include <time.h>   // struct tm, time(), strftime()
#include <DHT.h>

const char *WIFI_SSID = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char *WIFI_PASS = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

// Pilih SATU zona waktu: hapus // di depan baris yang kamu pakai, beri // di baris lain
const char *ZONA_WAKTU = "WIB-7";        // WIB  = UTC+7
// const char *ZONA_WAKTU = "WITA-8";    // WITA = UTC+8
// const char *ZONA_WAKTU = "WIT-9";     // WIT  = UTC+9

const char *SERVER_NTP_1 = "id.pool.ntp.org";  // kumpulan server NTP di Indonesia
const char *SERVER_NTP_2 = "pool.ntp.org";     // cadangan: kumpulan server seluruh dunia

#define DHT_PIN  27      // pin DHT di header hijau → IO27
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

const unsigned long INTERVAL_LOG = 10000;   // satu baris log setiap 10 detik

const char *NAMA_HARI[] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
const char *NAMA_BULAN[] = {"Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli",
                            "Agustus", "September", "Oktober", "November", "Desember"};

bool ntpDimulai = false;   // configTzTime() sudah dipanggil?
bool waktuSiap = false;    // jam sudah tersinkron?
unsigned long waktuLogTerakhir = 0;

// Mencetak satu baris log: waktu + suhu + kelembapan
void cetakLog(unsigned long sekarang) {
  char teksWaktu[40];
  struct tm waktu;

  if (getLocalTime(&waktu, 0)) {   // 0 = jangan menunggu
    strftime(teksWaktu, sizeof(teksWaktu), "%Y-%m-%d %H:%M:%S %Z", &waktu);
  } else {
    snprintf(teksWaktu, sizeof(teksWaktu), "(belum sinkron) +%lus", sekarang / 1000);
  }

  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.printf("%s | DHT11 gagal dibaca (cek kabel DHT -> IO27)\n", teksWaktu);
  } else {
    Serial.printf("%s | suhu %.1f °C | kelembapan %.0f %%\n", teksWaktu, suhu, kelembapan);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("BAB 42 - Program 4: Jam Internet dan Log Suhu");

  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);   // tidak menunggu: koneksi diperiksa di loop()
  Serial.printf("Menyambung ke \"%s\"...\n", WIFI_SSID);
}

void loop() {
  unsigned long sekarang = millis();

  // 1) Setelah WiFi tersambung, mulai sinkronisasi waktu (cukup sekali)
  if (!ntpDimulai && WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi tersambung (IP %s). Meminta waktu ke server NTP...\n",
                  WiFi.localIP().toString().c_str());
    configTzTime(ZONA_WAKTU, SERVER_NTP_1, SERVER_NTP_2);
    ntpDimulai = true;
  }

  // 2) Umumkan sekali saat jam pertama kali tersinkron
  struct tm waktu;
  if (ntpDimulai && !waktuSiap && getLocalTime(&waktu, 0)) {
    waktuSiap = true;
    char namaZona[8];
    strftime(namaZona, sizeof(namaZona), "%Z", &waktu);   // misalnya "WIB"
    Serial.printf("Waktu tersinkron: %s, %d %s %d pukul %02d:%02d:%02d %s\n",
                  NAMA_HARI[waktu.tm_wday], waktu.tm_mday, NAMA_BULAN[waktu.tm_mon],
                  waktu.tm_year + 1900, waktu.tm_hour, waktu.tm_min, waktu.tm_sec, namaZona);
    Serial.printf("Unix time: %lld\n", (long long)time(nullptr));
  }

  // 3) Log berkala — tetap berjalan walaupun WiFi atau jam belum siap
  if (sekarang - waktuLogTerakhir >= INTERVAL_LOG) {
    waktuLogTerakhir = sekarang;
    cetakLog(sekarang);
  }
}
