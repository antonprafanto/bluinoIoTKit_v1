/*
 * BAB 35 - Program 1: Halo, OLED!
 * Menampilkan teks ukuran 1 dan 2 di OLED, lalu mengukur
 * berapa lama oled.display() mengirim buffer 1 KB.
 *
 * Rangkaian: tidak perlu kabel. OLED SSD1306 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit, alamat 0x3C.
 * Library: Adafruit SSD1306 + Adafruit GFX Library (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA      21
#define PIN_SCL      22
#define LEBAR_LAYAR  128
#define TINGGI_LAYAR 64
#define ALAMAT_OLED  0x3C

// Objek layar: 128x64, bus I2C pertama (Wire), tanpa pin reset (-1)
Adafruit_SSD1306 oled(LEBAR_LAYAR, TINGGI_LAYAR, &Wire, -1);

// Mencetak pesan, lalu menghentikan program di sini
void berhenti(const char *pesan) {
  Serial.println(pesan);
  while (true) {
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println("BAB 35 - Program 1: Halo, OLED!");

  Wire.begin(PIN_SDA, PIN_SCL);

  // 1. Pastikan OLED menjawab di alamat 0x3C (begin() tidak memeriksanya)
  Wire.beginTransmission(ALAMAT_OLED);
  if (Wire.endTransmission() != 0) {
    berhenti("OLED tidak menjawab di 0x3C! Periksa jumper cap SCL/SDA OLED.");
  }

  // 2. Siapkan OLED dan buffer 1 KB di RAM ESP32
  if (!oled.begin(SSD1306_SWITCHCAPVCC, ALAMAT_OLED)) {
    berhenti("Gagal menyiapkan OLED: RAM tidak cukup untuk buffer.");
  }
  Serial.println("OLED siap.");

  // 3. Gambar di buffer (layar belum berubah)
  oled.clearDisplay();               // kosongkan buffer (logo Adafruit ikut terhapus)
  oled.setTextColor(SSD1306_WHITE);  // teks = piksel menyala
  oled.setTextSize(1);               // ukuran 1: 6x8 piksel per karakter
  oled.setCursor(0, 0);              // mulai dari pojok kiri atas
  oled.println("Halo, OLED!");
  oled.println("Ukuran 1 = 6x8 px");

  oled.setTextSize(2);               // ukuran 2: 12x16 piksel per karakter
  oled.setCursor(0, 20);
  oled.println("Ukuran 2");

  oled.setTextSize(1);
  oled.setCursor(0, 44);
  oled.println("Satu baris 21 huruf:");
  oled.print("123456789012345678901");  // tepat 21 karakter, mulai di y = 52

  // 4. Kirim buffer ke layar sambil mengukur lamanya
  unsigned long mulai = micros();
  oled.display();
  unsigned long lama = micros() - mulai;
  Serial.printf("oled.display() selesai dalam %lu us\n", lama);
}

void loop() {
  // Kosong. Gambar tetap tampil karena SSD1306 menyimpannya di GDDRAM.
}
