/*
 * BAB 23 - Program 1: Memeriksa Perangkat dan Kode Hasil
 * Memanggil alamat OLED, MPU-6050, BMP180, dan satu alamat kosong (0x50),
 * lalu menampilkan kode hasil Wire.endTransmission() beserta artinya.
 *
 * Rangkaian: tidak perlu kabel. Modul I2C tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 */

#include <Wire.h>

#define PIN_SDA 21
#define PIN_SCL 22

// Alamat yang diperiksa dan namanya (urutan kedua array harus sama)
const uint8_t daftarAlamat[] = {0x3C, 0x68, 0x77, 0x50};
const char *daftarNama[]     = {"OLED SSD1306", "MPU-6050", "BMP180", "(alamat kosong)"};
const int JUMLAH_ALAMAT = 4;

// Menerjemahkan kode hasil endTransmission() menjadi teks
const char *artiKode(uint8_t kode) {
  switch (kode) {
    case 0:  return "ACK, perangkat menjawab";
    case 2:  return "NACK, tidak ada yang menjawab";
    case 5:  return "timeout, bus macet atau pull-up hilang";
    default: return "error lain";
  }
}

// Memanggil setiap alamat tanpa data, lalu mencetak kode hasilnya
void periksaSemua() {
  for (int i = 0; i < JUMLAH_ALAMAT; i++) {
    Wire.beginTransmission(daftarAlamat[i]);  // siapkan panggilan ke alamat ini
    uint8_t kode = Wire.endTransmission();    // kirim alamat saja, lalu tunggu ACK/NACK
    Serial.printf("  0x%02X %-16s -> kode %u: %s\n",
                  daftarAlamat[i], daftarNama[i], kode, artiKode(kode));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung

  if (!Wire.begin(PIN_SDA, PIN_SCL)) {  // SDA = IO21, SCL = IO22, kecepatan bawaan 100 kHz
    Serial.println("Gagal memulai bus I2C! Periksa nomor pin.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }

  Serial.println("BAB 23 - Program 1: Memeriksa Perangkat I2C");

  Serial.printf("Kecepatan bus: %lu Hz\n", (unsigned long)Wire.getClock());
  periksaSemua();

  Wire.setClock(400000);  // pindah ke Fast-mode (400 kHz)
  Serial.printf("Kecepatan bus: %lu Hz\n", (unsigned long)Wire.getClock());
  periksaSemua();

  Serial.println("Selesai. Tekan tombol EN untuk mengulang.");
}

void loop() {
  // Semua pemeriksaan sudah dijalankan sekali di setup().
}
