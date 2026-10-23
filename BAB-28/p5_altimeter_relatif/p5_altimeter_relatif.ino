/*
 * BAB 28 - Program 5: Altimeter Relatif
 * Saat program mulai, tekanan di tempat itu dicatat sebagai "titik nol".
 * Setelah itu, setiap 0,5 detik program menampilkan perubahan tinggi (meter)
 * terhadap titik nol. Tekan tombol EN untuk menentukan titik nol yang baru.
 *
 * Rangkaian: tidak perlu kabel. BMP180 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: Adafruit BMP085 Library (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

#define PIN_SDA 21
#define PIN_SCL 22
#define ALAMAT_BMP180 0x77

const int JUMLAH_RATA = 8;              // banyaknya bacaan terakhir yang dirata-rata
const unsigned long JEDA_BACA = 100;    // ambil satu bacaan setiap 100 ms
const unsigned long JEDA_TAMPIL = 500;  // tampilkan hasil setiap 500 ms

Adafruit_BMP085 bmp;

float daftarBacaan[JUMLAH_RATA];  // bacaan-bacaan terakhir (Pa)
int posisi = 0;                   // tempat menyimpan bacaan berikutnya
float tekananNol = 0;             // tekanan di titik nol (Pa)
bool sensorBermasalah = false;    // true kalau bacaan terakhir gagal

unsigned long waktuBacaTerakhir = 0;
unsigned long waktuTampilTerakhir = 0;

// Rumus barometrik (sama seperti Program 3)
float hitungKetinggian(float p, float p0) {
  return 44330.0 * (1.0 - pow(p / p0, 0.1903));
}

// Membaca tekanan dengan pemeriksaan. Hasil: true kalau bacaan bisa dipercaya.
bool bacaTekanan(float &hasilPa) {
  Wire.beginTransmission(ALAMAT_BMP180);  // apakah BMP180 masih menjawab?
  if (Wire.endTransmission() != 0) {
    return false;
  }
  int32_t p = bmp.readPressure();
  if (p < 30000 || p > 110000) {  // di luar rentang BMP180 (300-1100 hPa)
    return false;
  }
  hasilPa = p;
  return true;
}

// Rata-rata semua bacaan yang tersimpan di daftarBacaan
float rataRataBacaan() {
  float jumlah = 0;
  for (int i = 0; i < JUMLAH_RATA; i++) {
    jumlah += daftarBacaan[i];
  }
  return jumlah / JUMLAH_RATA;
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);

  Serial.println("BAB 28 - Program 5: Altimeter Relatif");
  if (!bmp.begin(BMP085_ULTRAHIGHRES)) {  // mode paling halus
    Serial.println("BMP180 tidak ditemukan! Jalankan Program 1 untuk memeriksa.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }

  // Titik nol = rata-rata 20 bacaan (sekitar 0,6 detik)
  Serial.println("Menentukan titik nol... jangan gerakkan kit.");
  float jumlah = 0;
  for (int i = 0; i < 20; i++) {
    jumlah += bmp.readPressure();
  }
  tekananNol = jumlah / 20;

  for (int i = 0; i < JUMLAH_RATA; i++) {
    daftarBacaan[i] = tekananNol;  // isi awal supaya rata-ratanya langsung masuk akal
  }
  Serial.printf("Titik nol: %.2f hPa. Sekarang coba naikkan atau turunkan kit.\n",
                tekananNol / 100.0);
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: ambil satu bacaan tekanan setiap JEDA_BACA ms
  if (sekarang - waktuBacaTerakhir >= JEDA_BACA) {
    waktuBacaTerakhir = sekarang;
    float p;
    if (bacaTekanan(p)) {
      daftarBacaan[posisi] = p;             // ganti bacaan yang paling lama
      posisi = (posisi + 1) % JUMLAH_RATA;  // 0, 1, ..., 7, lalu kembali ke 0
      sensorBermasalah = false;
    } else {
      sensorBermasalah = true;
    }
  }

  // Tugas 2: tampilkan hasil setiap JEDA_TAMPIL ms
  if (sekarang - waktuTampilTerakhir >= JEDA_TAMPIL) {
    waktuTampilTerakhir = sekarang;
    if (sensorBermasalah) {
      Serial.println("BMP180 tidak terbaca! Periksa jumper cap-nya.");
      return;
    }
    float tekananRata = rataRataBacaan();
    float tinggi = hitungKetinggian(tekananRata, tekananNol);  // p0 = tekanan titik nol
    Serial.printf("Tekanan rata-rata: %.2f hPa | Tinggi relatif: %+.2f m\n",
                  tekananRata / 100.0, tinggi);
  }
}
