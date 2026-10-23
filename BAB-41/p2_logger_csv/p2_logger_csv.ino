/*
 * BAB 41 - Program 2: Data Logger CSV Sederhana
 * - Membaca DHT11 setiap 10 detik; setiap bacaan menjadi satu baris CSV
 * - Baris-baris ditampung dulu di RAM, lalu ditulis sekaligus ke
 *   /dht11.csv setiap 60 detik (buka -> tambah -> tutup)
 * - Baris judul ditulis otomatis kalau file belum ada
 * - LED biru di board (IO2) menyala selama menulis ke kartu
 *
 * Rangkaian: DHT (header hijau) -> IO27.
 * microSD: CS = IO5, MOSI = IO23, CLK = IO18, MISO = IO19 (lewat jumper cap).
 */

#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <esp_timer.h>

#define PIN_SCK  18  // CLK
#define PIN_MISO 19
#define PIN_MOSI 23
#define PIN_CS   5
#define PIN_DHT  27
#define TIPE_DHT DHT11
#define LED_BIRU 2   // LED biru bawaan board ESP32

// Format CSV (penjelasan di 41.5.3):
//   false -> pemisah kolom koma, desimal titik       contoh: 10,28.4,70,ok
//   true  -> pemisah kolom titik koma, desimal koma  contoh: 10;28,4;70;ok
const bool FORMAT_INDONESIA = false;

const uint32_t KECEPATAN_SPI = 4000000;  // 4 MHz
const char *FILE_LOG = "/dht11.csv";
const char *JUDUL_KOLOM = "detik,suhu_C,kelembapan_persen,status";

// struct Timer dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerBaca = {10000, 0};    // baca sensor setiap 10 detik
Timer timerSimpan = {60000, 0};  // tulis ke kartu setiap 60 detik

DHT dht(PIN_DHT, TIPE_DHT);
bool kartuSiap = false;
String penampung = "";             // baris CSV yang belum ditulis ke kartu
int barisDitampung = 0;            // jumlah baris di penampung
unsigned long barisTersimpan = 0;  // baris yang sudah ditulis sejak menyala

// Mengubah satu baris ke format Indonesia kalau FORMAT_INDONESIA = true:
// koma (pemisah kolom) -> titik koma, titik (desimal) -> koma
void sesuaikanFormat(char *baris) {
  if (!FORMAT_INDONESIA) {
    return;
  }
  for (int i = 0; baris[i] != '\0'; i++) {
    if (baris[i] == ',') {
      baris[i] = ';';
    } else if (baris[i] == '.') {
      baris[i] = ',';
    }
  }
}

// Membuat file log beserta baris judulnya kalau file belum ada
bool siapkanFileLog() {
  if (SD.exists(FILE_LOG)) {
    return true;
  }
  File f = SD.open(FILE_LOG, FILE_WRITE);
  if (!f) {
    return false;
  }
  char judul[64];
  snprintf(judul, sizeof(judul), "%s", JUDUL_KOLOM);
  sesuaikanFormat(judul);
  f.print(judul);
  f.print("\r\n");
  f.close();
  Serial.printf("[FILE] %s baru dibuat\n", FILE_LOG);
  return true;
}

// Membaca DHT11 lalu menambahkan satu baris CSV ke penampung
void bacaSensor() {
  // Detik sejak ESP32 menyala, dari pewaktu 64 bit (tidak kembali ke 0)
  unsigned long long detik = esp_timer_get_time() / 1000000ULL;
  float suhu = dht.readTemperature();
  float lembap = dht.readHumidity();

  char baris[48];
  if (isnan(suhu) || isnan(lembap)) {
    snprintf(baris, sizeof(baris), "%llu,,,gagal", detik);  // kolom nilai dikosongkan
    Serial.printf("[BACA] %llu s: gagal membaca DHT11 (cek kabel DHT -> 27)\n", detik);
  } else {
    snprintf(baris, sizeof(baris), "%llu,%.1f,%.0f,ok", detik, suhu, lembap);
    Serial.printf("[BACA] %llu s: %.1f °C, %.0f %%\n", detik, suhu, lembap);
  }
  sesuaikanFormat(baris);
  penampung += baris;
  penampung += "\r\n";
  barisDitampung = barisDitampung + 1;
}

// Menulis seluruh isi penampung ke kartu: buka -> tambah -> tutup
void simpanKeKartu() {
  if (barisDitampung == 0) {
    return;
  }
  bool berhasil = false;
  if (kartuSiap) {
    digitalWrite(LED_BIRU, HIGH);  // tanda: sedang menulis, jangan cabut kartu
    File f = SD.open(FILE_LOG, FILE_APPEND);
    if (f) {
      size_t ditulis = f.print(penampung);
      f.close();  // close() = data dikirim ke kartu
      berhasil = (ditulis == penampung.length());
    }
    digitalWrite(LED_BIRU, LOW);
  }

  if (berhasil) {
    barisTersimpan = barisTersimpan + barisDitampung;
    Serial.printf("[SIMPAN] %d baris ditulis ke %s (total %lu baris sejak menyala)\n",
                  barisDitampung, FILE_LOG, barisTersimpan);
  } else {
    Serial.printf("[SIMPAN] Gagal! %d baris dibuang. Kartu tidak ada atau dicabut?\n",
                  barisDitampung);
  }
  penampung = "";  // kosongkan penampung untuk menit berikutnya
  barisDitampung = 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  dht.begin();
  penampung.reserve(512);  // siapkan tempat di RAM sejak awal
  Serial.println();
  Serial.println("BAB 41 - Program 2: Data Logger CSV Sederhana");

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);  // urutan: SCK, MISO, MOSI
  if (SD.begin(PIN_CS, SPI, KECEPATAN_SPI) && siapkanFileLog()) {
    kartuSiap = true;
    Serial.printf("Kartu siap. Data dicatat ke %s\n", FILE_LOG);
  } else {
    Serial.println("Kartu tidak siap! Data hanya ditampilkan, tidak disimpan.");
  }
  Serial.println("Baca setiap 10 detik, tulis ke kartu setiap 60 detik.");
}

void loop() {
  unsigned long sekarang = millis();

  if (timerBaca.sudahWaktunya(sekarang)) {
    bacaSensor();
  }
  if (timerSimpan.sudahWaktunya(sekarang)) {
    simpanKeKartu();
  }
}
