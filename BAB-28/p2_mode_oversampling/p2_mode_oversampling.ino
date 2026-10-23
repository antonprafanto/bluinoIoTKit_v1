/*
 * BAB 28 - Program 2: Membandingkan Mode Oversampling
 * Untuk setiap mode, program mengambil 20 bacaan tekanan, lalu menampilkan
 * waktu satu bacaan, rata-rata tekanan, dan rentang naik-turunnya.
 *
 * Rangkaian: tidak perlu kabel. BMP180 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: Adafruit BMP085 Library (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

#define PIN_SDA 21
#define PIN_SCL 22

const int JUMLAH_BACAAN = 20;  // jumlah bacaan untuk setiap mode
const int JUMLAH_MODE = 4;

// Keempat mode dan namanya (urutan kedua array harus sama)
const uint8_t daftarMode[JUMLAH_MODE] = {BMP085_ULTRALOWPOWER, BMP085_STANDARD,
                                         BMP085_HIGHRES, BMP085_ULTRAHIGHRES};
const char *namaMode[JUMLAH_MODE] = {"Ultra low power", "Standard",
                                     "High resolution", "Ultra high res."};

Adafruit_BMP085 bmp;

// Menguji satu mode, lalu mencetak satu baris hasil
void ujiMode(int i) {
  if (!bmp.begin(daftarMode[i])) {  // mulai ulang sensor dengan mode ini
    Serial.printf("%-16s | BMP180 tidak ditemukan\n", namaMode[i]);
    return;
  }

  int32_t terkecil = 200000;     // lebih besar dari tekanan mana pun
  int32_t terbesar = 0;
  long jumlah = 0;               // jumlah semua bacaan (Pa)
  unsigned long totalWaktu = 0;  // dalam mikrodetik

  for (int n = 0; n < JUMLAH_BACAAN; n++) {
    unsigned long mulai = micros();
    int32_t p = bmp.readPressure();  // Pa
    totalWaktu += micros() - mulai;

    jumlah += p;
    if (p < terkecil) {
      terkecil = p;
    }
    if (p > terbesar) {
      terbesar = p;
    }
  }

  float waktuMs = totalWaktu / 1000.0 / JUMLAH_BACAAN;  // waktu rata-rata satu bacaan
  float rataHPa = jumlah / 100.0 / JUMLAH_BACAAN;       // rata-rata tekanan (hPa)
  long rentangPa = terbesar - terkecil;                 // selisih terbesar dan terkecil
  float rentangM = rentangPa * 0.083;                   // dekat permukaan laut: 1 Pa ≈ 0,083 m

  Serial.printf("%-16s | %5.1f ms | %7.2f hPa | %3ld Pa (~%.1f m)\n",
                namaMode[i], waktuMs, rataHPa, rentangPa, rentangM);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);

  Serial.println("BAB 28 - Program 2: Membandingkan Mode Oversampling");
  Serial.printf("Setiap mode dibaca %d kali. Biarkan kit diam di meja.\n", JUMLAH_BACAAN);
  Serial.printf("%-16s | %-8s | %-11s | %s\n", "Mode", "Waktu", "Rata-rata", "Naik-turun");

  for (int i = 0; i < JUMLAH_MODE; i++) {
    ujiMode(i);
  }
  Serial.println("Selesai. Tekan tombol EN untuk mengulang.");
}

void loop() {
  // Semua pengujian sudah dijalankan sekali di setup().
}
