/*
 * BAB 29 - Program 1: Membaca Akselerometer, Giroskop, dan Suhu Chip
 * Membaca MPU-6050 dengan library Adafruit setiap 500 ms, lalu menampilkan
 * percepatan (m/s² dan g), kecepatan putar (°/s), dan suhu chip.
 * Kalau begin() gagal, program memeriksa alamat 0x68 dan 0x69 serta
 * register WHO_AM_I untuk mencari tahu penyebabnya.
 *
 * Rangkaian: tidak perlu kabel. MPU-6050 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: Adafruit MPU6050 (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define PIN_SDA 21
#define PIN_SCL 22

const uint8_t MPU_ALAMAT = 0x68;      // alamat I2C MPU-6050 di kit (AD0 = LOW)
const unsigned long JEDA_BACA = 500;  // baca sensor setiap 500 ms
unsigned long waktuBacaTerakhir = 0;

Adafruit_MPU6050 mpu;  // objek sensor MPU-6050

// Membaca 1 byte dari sebuah register (pola dari BAB 23).
// Hasil: 0-255 kalau berhasil, -1 kalau perangkat tidak menjawab.
int bacaRegister(uint8_t alamat, uint8_t reg) {
  Wire.beginTransmission(alamat);
  Wire.write(reg);
  Wire.endTransmission(false);
  if (Wire.requestFrom(alamat, 1) != 1) {
    return -1;
  }
  return Wire.read();
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

// Dipanggil kalau mpu.begin() gagal: periksa apa yang ada di 0x68 dan 0x69
void periksaChipImu() {
  const uint8_t daftarAlamat[] = {0x68, 0x69};
  bool adaYangMenjawab = false;

  for (int i = 0; i < 2; i++) {
    uint8_t alamat = daftarAlamat[i];
    Wire.beginTransmission(alamat);
    if (Wire.endTransmission() != 0) {
      continue;  // tidak ada perangkat di alamat ini, lanjut ke alamat berikutnya
    }
    adaYangMenjawab = true;

    int id = bacaRegister(alamat, 0x75);  // register WHO_AM_I
    if (id < 0) {
      Serial.printf("  Ada perangkat di 0x%02X, tetapi WHO_AM_I tidak terbaca.\n", alamat);
    } else if (id != 0x68) {
      Serial.printf("  Ada perangkat di 0x%02X, WHO_AM_I = 0x%02X (%s).\n", alamat, id, namaChip(id));
      Serial.println("  Bukan MPU-6050, jadi library Adafruit menolaknya. Baca 29.7.");
    } else if (alamat != MPU_ALAMAT) {
      Serial.printf("  MPU-6050 ada di 0x%02X. Ganti MPU_ALAMAT di kode menjadi 0x%02X.\n", alamat, alamat);
    } else {
      Serial.println("  MPU-6050 menjawab. Cabut USB beberapa detik, lalu coba lagi.");
    }
  }

  if (!adaYangMenjawab) {
    Serial.println("  Tidak ada perangkat di 0x68 maupun 0x69.");
    Serial.println("  Cabut USB, lalu periksa jumper cap SCL/SDA milik MPU-6050.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);                   // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);  // bus I2C kit: SDA = IO21, SCL = IO22

  Serial.println("BAB 29 - Program 1: Membaca Akselerometer, Giroskop, dan Suhu Chip");

  if (!mpu.begin(MPU_ALAMAT)) {
    Serial.println("MPU-6050 tidak ditemukan! Hasil pemeriksaan:");
    periksaChipImu();
    Serial.println("Program berhenti. Perbaiki dulu, lalu tekan tombol EN.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }

  // Atur rentang dan filter SETELAH begin(), karena begin() mengembalikan semuanya ke bawaan
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);  // akselerometer ±2 g
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);       // giroskop ±500 °/s
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);    // filter DLPF 21 Hz

  Serial.println("MPU-6050 siap: akselerometer ±2 g, giroskop ±500 °/s, filter 21 Hz.");
}

void loop() {
  if (millis() - waktuBacaTerakhir < JEDA_BACA) {
    return;  // belum 500 ms, keluar dulu dari loop()
  }
  waktuBacaTerakhir = millis();

  sensors_event_t aksel, giro, suhu;   // tiga "amplop" data
  mpu.getEvent(&aksel, &giro, &suhu);  // isi ketiganya dari satu pembacaan

  // Percepatan dari library dalam m/s², dibagi 9.80665 supaya menjadi g
  float axMs2 = aksel.acceleration.x;
  float ayMs2 = aksel.acceleration.y;
  float azMs2 = aksel.acceleration.z;
  float axG = axMs2 / SENSORS_GRAVITY_STANDARD;  // SENSORS_GRAVITY_STANDARD = 9.80665
  float ayG = ayMs2 / SENSORS_GRAVITY_STANDARD;
  float azG = azMs2 / SENSORS_GRAVITY_STANDARD;

  // Kecepatan putar dari library dalam rad/s, dikali RAD_TO_DEG (57.2958) supaya menjadi °/s
  float gxDps = giro.gyro.x * RAD_TO_DEG;
  float gyDps = giro.gyro.y * RAD_TO_DEG;
  float gzDps = giro.gyro.z * RAD_TO_DEG;

  Serial.printf("Aksel (m/s²): X=%6.2f  Y=%6.2f  Z=%6.2f\n", axMs2, ayMs2, azMs2);
  Serial.printf("Aksel (g)   : X=%6.2f  Y=%6.2f  Z=%6.2f\n", axG, ayG, azG);
  Serial.printf("Giro (°/s)  : X=%6.2f  Y=%6.2f  Z=%6.2f\n", gxDps, gyDps, gzDps);
  Serial.printf("Suhu chip   : %.2f °C\n", suhu.temperature);
  Serial.println("---");
}
