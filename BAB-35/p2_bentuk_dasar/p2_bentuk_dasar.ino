/*
 * BAB 35 - Program 2: Galeri Bentuk Dasar
 * Menggambar bingkai, garis, kotak, lingkaran, kotak bersudut tumpul,
 * dan segitiga (versi draw = garis tepi, versi fill = penuh).
 * Setiap 2 detik warna layar dibalik dengan invertDisplay().
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

#define PUTIH SSD1306_WHITE  // singkatan supaya baris kode tidak terlalu panjang

Adafruit_SSD1306 oled(LEBAR_LAYAR, TINGGI_LAYAR, &Wire, -1);

const unsigned long INTERVAL_BALIK = 2000;  // ms
unsigned long waktuLalu = 0;
bool dibalik = false;

void berhenti(const char *pesan) {
  Serial.println(pesan);
  while (true) {
    delay(1000);
  }
}

void gambarGaleri() {
  oled.clearDisplay();

  // Bingkai seluruh layar dan judul
  oled.drawRect(0, 0, LEBAR_LAYAR, TINGGI_LAYAR, PUTIH);  // x 0-127, y 0-63
  oled.setTextSize(1);
  oled.setTextColor(PUTIH);
  oled.setCursor(4, 3);
  oled.print("draw=garis  fill=isi");
  oled.drawLine(0, 12, 127, 12, PUTIH);                  // garis pemisah judul

  // Baris atas: garis tepi saja (draw...)
  oled.drawRect(6, 16, 24, 18, PUTIH);                   // pojok kiri atas (6,16), 24x18
  oled.drawCircle(47, 25, 9, PUTIH);                     // pusat (47,25), jari-jari 9
  oled.drawRoundRect(64, 16, 28, 18, 5, PUTIH);          // sudut tumpul, jari-jari 5
  oled.drawTriangle(98, 33, 109, 16, 120, 33, PUTIH);    // tiga titik sudut

  // Baris bawah: bentuk yang sama, terisi penuh (fill...)
  oled.fillRect(6, 40, 24, 18, PUTIH);
  oled.fillCircle(47, 49, 9, PUTIH);
  oled.fillRoundRect(64, 40, 28, 18, 5, PUTIH);
  oled.fillTriangle(98, 57, 109, 40, 120, 57, PUTIH);

  oled.display();  // kirim semuanya ke layar sekaligus
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BAB 35 - Program 2: Galeri Bentuk Dasar");

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.beginTransmission(ALAMAT_OLED);
  if (Wire.endTransmission() != 0) {
    berhenti("OLED tidak menjawab di 0x3C! Periksa jumper cap SCL/SDA OLED.");
  }
  if (!oled.begin(SSD1306_SWITCHCAPVCC, ALAMAT_OLED)) {
    berhenti("Gagal menyiapkan OLED: RAM tidak cukup untuk buffer.");
  }

  gambarGaleri();  // cukup digambar sekali
  Serial.println("Galeri digambar. Warna layar dibalik setiap 2 detik.");
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - waktuLalu >= INTERVAL_BALIK) {
    waktuLalu = sekarang;
    dibalik = !dibalik;
    oled.invertDisplay(dibalik);  // perintah langsung ke SSD1306, tanpa display()
    Serial.println(dibalik ? "invertDisplay(true): warna dibalik"
                           : "invertDisplay(false): warna normal");
  }
}
