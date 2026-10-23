/*
 * BAB 35 - Program 3: Ikon Bitmap
 * Menampilkan ikon termometer dan tetes air (16x16 piksel) dengan
 * drawBitmap(), lalu menggambar ikon tetes air 2x lebih besar
 * dengan membaca bit demi bit.
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

#define LEBAR_IKON  16
#define TINGGI_IKON 16

Adafruit_SSD1306 oled(LEBAR_LAYAR, TINGGI_LAYAR, &Wire, -1);

// Ikon 16x16: 2 byte per baris x 16 baris = 32 byte. Bit 1 = piksel menyala.
const unsigned char ikonTermometer[] PROGMEM = {
  0x03, 0x00, 0x04, 0x80, 0x04, 0xB0, 0x04, 0x80,
  0x04, 0xB8, 0x04, 0x80, 0x07, 0xB0, 0x07, 0x80,
  0x07, 0xB8, 0x0F, 0xC0, 0x1F, 0xE0, 0x1F, 0xE0,
  0x1F, 0xE0, 0x1F, 0xE0, 0x0F, 0xC0, 0x07, 0x80
};

const unsigned char ikonTetes[] PROGMEM = {
  0x01, 0x80, 0x01, 0x80, 0x03, 0xC0, 0x03, 0xC0,
  0x07, 0xE0, 0x07, 0xE0, 0x0F, 0xF0, 0x1F, 0xF8,
  0x1F, 0xF8, 0x3F, 0xFC, 0x37, 0xFC, 0x37, 0xFC,
  0x3B, 0xFC, 0x1F, 0xF8, 0x0F, 0xF0, 0x03, 0xC0
};

void berhenti(const char *pesan) {
  Serial.println(pesan);
  while (true) {
    delay(1000);
  }
}

// Menggambar bitmap dengan setiap piksel diperbesar menjadi kotak skala x skala.
// Cara membaca bitnya sama dengan drawBitmap() milik library GFX.
void gambarBitmapBesar(int x, int y, const unsigned char *data,
                       int lebar, int tinggi, int skala) {
  int bytePerBaris = (lebar + 7) / 8;  // dibulatkan ke atas
  for (int baris = 0; baris < tinggi; baris++) {
    for (int kolom = 0; kolom < lebar; kolom++) {
      uint8_t b = data[baris * bytePerBaris + kolom / 8];  // byte yang memuat piksel ini
      bool menyala = b & (0x80 >> (kolom % 8));            // bit paling kiri = 0x80
      if (menyala) {
        oled.fillRect(x + kolom * skala, y + baris * skala, skala, skala, SSD1306_WHITE);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BAB 35 - Program 3: Ikon Bitmap");

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.beginTransmission(ALAMAT_OLED);
  if (Wire.endTransmission() != 0) {
    berhenti("OLED tidak menjawab di 0x3C! Periksa jumper cap SCL/SDA OLED.");
  }
  if (!oled.begin(SSD1306_SWITCHCAPVCC, ALAMAT_OLED)) {
    berhenti("Gagal menyiapkan OLED: RAM tidak cukup untuk buffer.");
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print("Ikon Bitmap 16x16");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Ikon ukuran asli dengan drawBitmap()
  oled.drawBitmap(0, 14, ikonTermometer, LEBAR_IKON, TINGGI_IKON, SSD1306_WHITE);
  oled.setCursor(20, 18);
  oled.print("Suhu");

  oled.drawBitmap(0, 34, ikonTetes, LEBAR_IKON, TINGGI_IKON, SSD1306_WHITE);
  oled.setCursor(20, 38);
  oled.print("Kelembapan");

  // Ikon yang sama, diperbesar 2x (32x32 piksel) di kanan
  gambarBitmapBesar(92, 16, ikonTetes, LEBAR_IKON, TINGGI_IKON, 2);

  oled.setCursor(0, 56);
  oled.print("kiri: asli  kanan: 2x");
  oled.display();

  Serial.printf("Ukuran satu ikon: %u byte\n", (unsigned)sizeof(ikonTetes));
  Serial.println("Ikon ditampilkan.");
}

void loop() {
  // Kosong: gambar statis cukup dikirim sekali.
}
