/*
 * BAB 23 - Program 3: Suhu Chip MPU-6050
 * Membangunkan MPU-6050, lalu membaca suhu chip-nya setiap 1 detik.
 * Rumus dari datasheet: suhu (°C) = nilai mentah / 340 + 36.53
 *
 * Rangkaian: tidak perlu kabel. MPU-6050 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 */

#include <Wire.h>

#define PIN_SDA 21
#define PIN_SCL 22

#define MPU_ALAMAT      0x68  // alamat I2C MPU-6050
#define MPU_PWR_MGMT_1  0x6B  // register pengaturan daya
#define MPU_TEMP_OUT_H  0x41  // byte atas suhu (byte bawahnya di 0x42)

const unsigned long JEDA_BACA = 1000;  // baca suhu setiap 1000 ms
unsigned long waktuBacaTerakhir = 0;

// Menulis 1 byte ke sebuah register. Hasil: true kalau target menjawab ACK.
bool tulisRegister(uint8_t alamat, uint8_t reg, uint8_t nilai) {
  Wire.beginTransmission(alamat);
  Wire.write(reg);     // byte pertama: nomor register
  Wire.write(nilai);   // byte kedua: nilai yang ditulis
  return Wire.endTransmission() == 0;
}

// Membaca beberapa register berurutan mulai dari regAwal (burst read).
// Hasil: true kalau semua byte diterima.
bool bacaBanyak(uint8_t alamat, uint8_t regAwal, uint8_t data[], uint8_t jumlah) {
  Wire.beginTransmission(alamat);
  Wire.write(regAwal);                               // tunjuk register pertama
  Wire.endTransmission(false);                       // tanpa STOP: repeated START
  if (Wire.requestFrom(alamat, jumlah) != jumlah) {  // minta beberapa byte sekaligus
    return false;
  }
  for (uint8_t i = 0; i < jumlah; i++) {
    data[i] = Wire.read();
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);

  Serial.println("BAB 23 - Program 3: Suhu Chip MPU-6050");

  // PWR_MGMT_1 = 0x01: bit SLEEP = 0 (bangun), sumber clock = PLL giroskop X
  if (tulisRegister(MPU_ALAMAT, MPU_PWR_MGMT_1, 0x01)) {
    Serial.println("MPU-6050 dibangunkan.");
  } else {
    Serial.println("Gagal menulis ke MPU-6050! Periksa jumper cap-nya.");
  }
}

void loop() {
  if (millis() - waktuBacaTerakhir < JEDA_BACA) {
    return;  // belum 1 detik, keluar dulu dari loop()
  }
  waktuBacaTerakhir = millis();

  uint8_t data[2];  // data[0] = TEMP_OUT_H (0x41), data[1] = TEMP_OUT_L (0x42)
  if (!bacaBanyak(MPU_ALAMAT, MPU_TEMP_OUT_H, data, 2)) {
    Serial.println("Gagal membaca MPU-6050.");
    return;
  }

  // Gabungkan byte atas dan byte bawah menjadi angka 16 bit bertanda (bisa negatif)
  int16_t mentah = (int16_t)((data[0] << 8) | data[1]);
  float suhu = mentah / 340.0 + 36.53;

  Serial.printf("Mentah: %6d  Suhu chip: %.2f °C\n", mentah, suhu);
}
