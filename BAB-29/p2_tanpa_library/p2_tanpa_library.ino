/*
 * BAB 29 - Program 2: Membaca Sensor Tanpa Library
 * Membaca akselerometer dan giroskop MPU-6050 (atau chip pengganti seperti
 * MPU-6500/MPU-9250) langsung dari register dengan library Wire,
 * lalu menampilkannya setiap 500 ms.
 *
 * Rangkaian: tidak perlu kabel. MPU-6050 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: tidak perlu library tambahan (Wire sudah termasuk paket esp32)
 */

#include <Wire.h>

#define PIN_SDA 21
#define PIN_SCL 22

const uint8_t MPU_ALAMAT = 0x68;  // ganti 0x69 kalau pin AD0 modulmu HIGH

// Nomor register (sama untuk MPU-6050 dan keluarga MPU-6500)
#define REG_CONFIG        0x1A  // filter DLPF giroskop (di MPU-6050 juga akselerometer)
#define REG_GYRO_CONFIG   0x1B  // rentang giroskop
#define REG_ACCEL_CONFIG  0x1C  // rentang akselerometer
#define REG_ACCEL_CONFIG2 0x1D  // filter akselerometer, hanya ada di keluarga MPU-6500
#define REG_ACCEL_XOUT_H  0x3B  // awal 14 byte data: aksel X, Y, Z, suhu, giro X, Y, Z
#define REG_PWR_MGMT_1    0x6B  // pengaturan daya
#define REG_WHO_AM_I      0x75  // identitas chip

const unsigned long JEDA_BACA = 500;  // baca sensor setiap 500 ms
unsigned long waktuBacaTerakhir = 0;
int idChip = -1;                      // isi WHO_AM_I yang terbaca

float ax, ay, az;  // percepatan (g)
float gx, gy, gz;  // kecepatan putar (°/s)

// ---------- Fungsi I2C dasar (sama seperti BAB 23) ----------

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

// Menerjemahkan isi register WHO_AM_I menjadi nama chip
const char *namaChip(int id) {
  switch (id) {
    case 0x68: return "MPU-6050";
    case 0x70: return "MPU-6500";
    case 0x71: return "MPU-9250";
    case 0x73: return "MPU-9255";
    case 0x98: return "ICM-20689";
    default:   return "chip tidak dikenal";
  }
}

// ---------- Fungsi sensor (bisa dipindah ke Program 3-7) ----------

// Memilih filter DLPF: 0 = paling lebar (sekitar 260 Hz) sampai 6 = paling sempit (5 Hz)
void aturFilter(uint8_t pilihan) {
  tulisRegister(MPU_ALAMAT, REG_CONFIG, pilihan);
  if (idChip != 0x68) {
    tulisRegister(MPU_ALAMAT, REG_ACCEL_CONFIG2, pilihan);  // keluarga MPU-6500
  }
}

// Menyiapkan sensor: ±2 g, ±500 °/s, filter sekitar 20 Hz. Hasil: true kalau berhasil.
bool siapkanImu() {
  idChip = bacaRegister(MPU_ALAMAT, REG_WHO_AM_I);
  if (idChip < 0) {
    return false;                                     // tidak ada yang menjawab
  }
  tulisRegister(MPU_ALAMAT, REG_PWR_MGMT_1, 0x80);    // reset semua register chip
  delay(100);                                         // tunggu reset selesai
  if (!tulisRegister(MPU_ALAMAT, REG_PWR_MGMT_1, 0x01)) {
    return false;                                     // gagal membangunkan chip
  }
  tulisRegister(MPU_ALAMAT, REG_GYRO_CONFIG, 0x08);   // giroskop ±500 °/s (65.5 = 1 °/s)
  tulisRegister(MPU_ALAMAT, REG_ACCEL_CONFIG, 0x00);  // akselerometer ±2 g (16384 = 1 g)
  aturFilter(4);                                      // filter sekitar 20 Hz
  delay(50);                                          // beri waktu giroskop stabil
  return true;
}

// Membaca sensor ke variabel ax..az (g) dan gx..gz (°/s). Hasil: true kalau berhasil.
bool bacaImu() {
  uint8_t d[14];  // isi register 0x3B sampai 0x48
  if (!bacaBanyak(MPU_ALAMAT, REG_ACCEL_XOUT_H, d, 14)) {
    return false;
  }
  ax = gabung(d[0], d[1]) / 16384.0;  // ±2 g: 16384 = 1 g
  ay = gabung(d[2], d[3]) / 16384.0;
  az = gabung(d[4], d[5]) / 16384.0;
  // d[6] dan d[7] berisi suhu chip; tidak dipakai karena rumusnya berbeda antar-chip
  gx = gabung(d[8], d[9]) / 65.5;     // ±500 °/s: 65.5 = 1 °/s
  gy = gabung(d[10], d[11]) / 65.5;
  gz = gabung(d[12], d[13]) / 65.5;
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);                   // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);  // bus I2C kit: SDA = IO21, SCL = IO22

  Serial.println("BAB 29 - Program 2: Membaca Sensor Tanpa Library");

  if (!siapkanImu()) {
    Serial.printf("Tidak ada yang menjawab di 0x%02X. Periksa jumper cap MPU-6050.\n", MPU_ALAMAT);
    Serial.println("Program berhenti. Perbaiki dulu, lalu tekan tombol EN.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }
  Serial.printf("WHO_AM_I = 0x%02X (%s). Sensor siap: ±2 g, ±500 °/s.\n", idChip, namaChip(idChip));
}

void loop() {
  if (millis() - waktuBacaTerakhir < JEDA_BACA) {
    return;  // belum 500 ms
  }
  waktuBacaTerakhir = millis();

  if (!bacaImu()) {
    Serial.println("Gagal membaca sensor. Periksa jumper cap MPU-6050.");
    return;
  }
  Serial.printf("Aksel (g)  : X=%6.2f  Y=%6.2f  Z=%6.2f\n", ax, ay, az);
  Serial.printf("Giro (°/s) : X=%6.2f  Y=%6.2f  Z=%6.2f\n", gx, gy, gz);
  Serial.println("---");
}
