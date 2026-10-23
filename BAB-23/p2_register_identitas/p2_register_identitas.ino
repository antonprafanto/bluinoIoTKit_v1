/*
 * BAB 23 - Program 2: Membaca Register Identitas
 * Membaca register WHO_AM_I MPU-6050 (0x75) dan chip-id BMP180 (0xD0),
 * lalu membandingkannya dengan nilai di datasheet.
 *
 * Rangkaian: tidak perlu kabel. Modul I2C tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 */

#include <Wire.h>

#define PIN_SDA 21
#define PIN_SCL 22

#define MPU_ALAMAT    0x68  // alamat I2C MPU-6050
#define MPU_WHO_AM_I  0x75  // register identitas MPU-6050
#define MPU_ID        0x68  // isi WHO_AM_I menurut datasheet

#define BMP_ALAMAT    0x77  // alamat I2C BMP180
#define BMP_CHIP_ID   0xD0  // register identitas BMP180
#define BMP_ID        0x55  // isi chip-id menurut datasheet

// Membaca 1 byte dari sebuah register.
// Hasil: 0-255 kalau berhasil, -1 kalau target tidak menjawab.
int bacaRegister(uint8_t alamat, uint8_t reg) {
  Wire.beginTransmission(alamat);
  Wire.write(reg);                          // tunjuk register yang ingin dibaca
  Wire.endTransmission(false);              // tanpa STOP: lanjut dengan repeated START
  if (Wire.requestFrom(alamat, 1) != 1) {   // minta 1 byte
    return -1;                              // tidak ada byte yang diterima
  }
  return Wire.read();
}

// Membaca register identitas sebuah chip, lalu mencetak hasil pemeriksaannya
void periksaIdentitas(const char *nama, uint8_t alamat, uint8_t reg, uint8_t idBenar) {
  int id = bacaRegister(alamat, reg);

  Serial.printf("%-8s (0x%02X), register 0x%02X", nama, alamat, reg);
  if (id < 0) {
    Serial.println(" -> tidak menjawab");
  } else if (id == idBenar) {
    Serial.printf(" = 0x%02X -> cocok, chip dikenali\n", id);
  } else {
    Serial.printf(" = 0x%02X -> tidak cocok (seharusnya 0x%02X)\n", id, idBenar);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);

  Serial.println("BAB 23 - Program 2: Membaca Register Identitas");
  periksaIdentitas("MPU-6050", MPU_ALAMAT, MPU_WHO_AM_I, MPU_ID);
  periksaIdentitas("BMP180", BMP_ALAMAT, BMP_CHIP_ID, BMP_ID);
  Serial.println("Selesai. Tekan tombol EN untuk mengulang.");
}

void loop() {
  // Pemeriksaan hanya dijalankan sekali di setup().
}
