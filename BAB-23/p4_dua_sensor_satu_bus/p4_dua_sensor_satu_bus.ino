/*
 * BAB 23 - Program 4: Dua Sensor di Satu Bus
 * Membaca MPU-6050 (percepatan + suhu chip) dan BMP180 (suhu udara)
 * yang berbagi satu bus I2C, setiap 2 detik, tanpa library sensor.
 *
 * Rangkaian: tidak perlu kabel. MPU-6050 dan BMP180 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 */

#include <Wire.h>

#define PIN_SDA 21
#define PIN_SCL 22

// ---- MPU-6050 ----
#define MPU_ALAMAT        0x68
#define MPU_WHO_AM_I      0x75
#define MPU_PWR_MGMT_1    0x6B
#define MPU_ACCEL_CONFIG  0x1C  // pengaturan rentang akselerometer
#define MPU_ACCEL_XOUT_H  0x3B  // awal 8 byte: percepatan X, Y, Z lalu suhu

// ---- BMP180 ----
#define BMP_ALAMAT        0x77
#define BMP_CHIP_ID       0xD0
#define BMP_KALIBRASI     0xAA  // awal 11 koefisien kalibrasi (22 byte)
#define BMP_KONTROL       0xF4  // register perintah
#define BMP_UKUR_SUHU     0x2E  // perintah: ukur suhu
#define BMP_HASIL         0xF6  // hasil pengukuran (2 byte)

const unsigned long JEDA_BACA = 2000;  // baca kedua sensor setiap 2000 ms
unsigned long waktuBacaTerakhir = 0;
int nomorBacaan = 0;

bool mpuSiap = false;
bool bmpSiap = false;

// Koefisien kalibrasi BMP180 yang dibutuhkan untuk menghitung suhu
uint16_t ac5, ac6;  // tanpa tanda
int16_t mc, md;     // bertanda

// ---------- Fungsi I2C dasar (sama seperti Program 2 dan 3) ----------

int bacaRegister(uint8_t alamat, uint8_t reg) {
  Wire.beginTransmission(alamat);
  Wire.write(reg);
  Wire.endTransmission(false);
  if (Wire.requestFrom(alamat, 1) != 1) {
    return -1;
  }
  return Wire.read();
}

bool tulisRegister(uint8_t alamat, uint8_t reg, uint8_t nilai) {
  Wire.beginTransmission(alamat);
  Wire.write(reg);
  Wire.write(nilai);
  return Wire.endTransmission() == 0;
}

bool bacaBanyak(uint8_t alamat, uint8_t regAwal, uint8_t data[], uint8_t jumlah) {
  Wire.beginTransmission(alamat);
  Wire.write(regAwal);
  Wire.endTransmission(false);
  if (Wire.requestFrom(alamat, jumlah) != jumlah) {
    return false;
  }
  for (uint8_t i = 0; i < jumlah; i++) {
    data[i] = Wire.read();
  }
  return true;
}

// Menggabungkan byte atas dan byte bawah menjadi angka 16 bit bertanda
int16_t gabung(uint8_t atas, uint8_t bawah) {
  return (int16_t)((atas << 8) | bawah);
}

// ---------- Persiapan sensor (dipanggil sekali di setup) ----------

bool siapkanMpu() {
  if (bacaRegister(MPU_ALAMAT, MPU_WHO_AM_I) != 0x68) {
    return false;                                     // bukan MPU-6050 atau tidak menjawab
  }
  if (!tulisRegister(MPU_ALAMAT, MPU_PWR_MGMT_1, 0x01)) {
    return false;                                     // gagal membangunkan
  }
  return tulisRegister(MPU_ALAMAT, MPU_ACCEL_CONFIG, 0x00);  // rentang ±2 g (16384 = 1 g)
}

bool siapkanBmp() {
  if (bacaRegister(BMP_ALAMAT, BMP_CHIP_ID) != 0x55) {
    return false;                                     // bukan BMP180 atau tidak menjawab
  }
  uint8_t k[22];                                      // 11 koefisien x 2 byte, mulai 0xAA
  if (!bacaBanyak(BMP_ALAMAT, BMP_KALIBRASI, k, 22)) {
    return false;
  }
  // Datasheet: tidak ada koefisien yang bernilai 0x0000 atau 0xFFFF
  for (int i = 0; i < 22; i += 2) {
    uint16_t kata = (k[i] << 8) | k[i + 1];
    if (kata == 0x0000 || kata == 0xFFFF) {
      return false;
    }
  }
  ac5 = (k[8] << 8) | k[9];     // register 0xB2-0xB3
  ac6 = (k[10] << 8) | k[11];   // register 0xB4-0xB5
  mc  = gabung(k[18], k[19]);   // register 0xBC-0xBD
  md  = gabung(k[20], k[21]);   // register 0xBE-0xBF
  return true;
}

// ---------- Pembacaan sensor ----------

void bacaMpu() {
  uint8_t d[8];  // 0x3B..0x42: ax, ay, az, suhu (masing-masing 2 byte)
  if (!bacaBanyak(MPU_ALAMAT, MPU_ACCEL_XOUT_H, d, 8)) {
    Serial.println("MPU-6050 : gagal dibaca");
    return;
  }
  float ax = gabung(d[0], d[1]) / 16384.0;  // rentang bawaan ±2 g: 16384 = 1 g
  float ay = gabung(d[2], d[3]) / 16384.0;
  float az = gabung(d[4], d[5]) / 16384.0;
  float suhuChip = gabung(d[6], d[7]) / 340.0 + 36.53;
  Serial.printf("MPU-6050 : ax=%5.2f g  ay=%5.2f g  az=%5.2f g  suhu chip=%.2f °C\n",
                ax, ay, az, suhuChip);
}

void bacaBmp() {
  // 1. Kirim perintah "ukur suhu"
  if (!tulisRegister(BMP_ALAMAT, BMP_KONTROL, BMP_UKUR_SUHU)) {
    Serial.println("BMP180   : perintah gagal dikirim");
    return;
  }
  // 2. Tunggu pengukuran selesai (datasheet: paling lama 4,5 ms)
  delay(5);
  // 3. Baca hasil mentah UT (2 byte)
  uint8_t d[2];
  if (!bacaBanyak(BMP_ALAMAT, BMP_HASIL, d, 2)) {
    Serial.println("BMP180   : gagal dibaca");
    return;
  }
  long ut = (d[0] << 8) | d[1];

  // 4. Hitung suhu sebenarnya dengan rumus datasheet BMP180
  long x1 = (ut - ac6) * ac5 / 32768;       // 32768 = 2^15
  long x2 = (long)mc * 2048 / (x1 + md);    // 2048 = 2^11
  long b5 = x1 + x2;
  long t  = (b5 + 8) / 16;                  // hasilnya dalam satuan 0,1 °C

  Serial.printf("BMP180   : UT=%ld  suhu udara=%.1f °C\n", ut, t / 10.0);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);

  Serial.println("BAB 23 - Program 4: Dua Sensor di Satu Bus");
  mpuSiap = siapkanMpu();
  bmpSiap = siapkanBmp();
  Serial.printf("MPU-6050 (0x%02X): %s\n", MPU_ALAMAT, mpuSiap ? "siap" : "tidak ditemukan");
  Serial.printf("BMP180   (0x%02X): %s\n", BMP_ALAMAT, bmpSiap ? "siap" : "tidak ditemukan");
  if (bmpSiap) {
    Serial.printf("Kalibrasi BMP180: AC5=%u AC6=%u MC=%d MD=%d\n", ac5, ac6, mc, md);
  }
}

void loop() {
  if (millis() - waktuBacaTerakhir < JEDA_BACA) {
    return;  // belum 2 detik
  }
  waktuBacaTerakhir = millis();
  nomorBacaan++;

  Serial.printf("--- Pembacaan ke-%d ---\n", nomorBacaan);
  if (mpuSiap) {
    bacaMpu();
  } else {
    Serial.println("MPU-6050 : tidak tersedia");
  }
  if (bmpSiap) {
    bacaBmp();
  } else {
    Serial.println("BMP180   : tidak tersedia");
  }
}
