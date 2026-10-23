// BAB 6 — Program 2: Pemindai I2C
// Mencari semua perangkat di bus I2C kit Bluino (SDA = IO21, SCL = IO22),
// lalu menampilkan alamat dan nama perangkatnya di Serial Monitor.

#include <Wire.h>

#define PIN_SDA 21
#define PIN_SCL 22

// Mengembalikan nama perangkat kit berdasarkan alamat I2C-nya
const char *namaPerangkat(uint8_t alamat) {
  switch (alamat) {
    case 0x3C: return "OLED SSD1306";
    case 0x68: return "MPU-6050";
    case 0x77: return "BMP180";
    default:   return "perangkat lain";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);
  Serial.println("Pemindai I2C kit Bluino (SDA = IO21, SCL = IO22)");
}

void loop() {
  Serial.println("Memindai alamat 0x01 sampai 0x7E ...");
  int jumlah = 0;

  for (uint8_t alamat = 0x01; alamat <= 0x7E; alamat++) {
    Wire.beginTransmission(alamat);           // "memanggil" alamat ini
    uint8_t hasil = Wire.endTransmission();   // 0 = ada perangkat yang menjawab

    if (hasil == 0) {
      Serial.printf("  Ditemukan: 0x%02X (%s)\n", alamat, namaPerangkat(alamat));
      jumlah++;
    }
  }

  Serial.printf("Selesai: %d perangkat ditemukan.\n\n", jumlah);
  delay(5000);  // ulangi pemindaian setiap 5 detik
}
