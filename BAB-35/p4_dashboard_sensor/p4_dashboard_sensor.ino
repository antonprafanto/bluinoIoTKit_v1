/*
 * BAB 35 - Program 4: Dashboard Sensor
 * Menampilkan suhu & kelembapan DHT11 (dengan ikon) dan tekanan BMP180.
 * Sensor dibaca setiap 2 detik, tetapi layar hanya digambar ulang
 * kalau angka yang tampil berubah.
 *
 * Rangkaian: pin DHT (header hijau) → IO27 (kabel jumper)
 *            OLED (0x3C) dan BMP180 (0x77) di IO21/IO22 lewat jumper cap
 * Library: Adafruit SSD1306, Adafruit GFX Library, DHT sensor library,
 *          Adafruit Unified Sensor, Adafruit BMP085 Library (semua by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>

#define PIN_SDA      21
#define PIN_SCL      22
#define LEBAR_LAYAR  128
#define TINGGI_LAYAR 64
#define ALAMAT_OLED  0x3C
#define DHT_PIN      27
#define DHT_TYPE     DHT11

Adafruit_SSD1306 oled(LEBAR_LAYAR, TINGGI_LAYAR, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP085 bmp;
bool bmpAda = false;

// Ikon 16x16 dari Program 3
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

// Timer serbaguna dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

// Angka dalam bentuk yang TAMPIL di layar (sudah dibulatkan)
const long NILAI_GAGAL = -9999;  // penanda: sensor gagal dibaca atau tidak ada

struct DataLayar {
  long suhuX10;     // suhu x 10, misalnya 285 berarti 28.5 °C
  long kelembapan;  // persen, bilangan bulat
  long tekanan;     // hPa, bilangan bulat
};

Timer timerSensor = {2000, 0};  // DHT11 tidak boleh dibaca lebih cepat dari 2 detik
DataLayar tampil;               // angka yang sedang tampil di layar
bool sudahDigambar = false;
unsigned long jumlahGambar = 0; // berapa kali layar digambar ulang

void berhenti(const char *pesan) {
  Serial.println(pesan);
  while (true) {
    delay(1000);
  }
}

DataLayar bacaSensor() {
  DataLayar d;

  dht.read(true);  // paksa membaca DHT11 sekarang (±25 ms), lihat BAB 26
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  if (isnan(suhu) || isnan(kelembapan)) {
    d.suhuX10 = NILAI_GAGAL;
    d.kelembapan = NILAI_GAGAL;
  } else {
    d.suhuX10 = lround(suhu * 10);      // 28.46 → 285
    d.kelembapan = lround(kelembapan);  // 64.8 → 65
  }

  if (bmpAda) {
    d.tekanan = lround(bmp.readPressure() / 100.0);  // Pa → hPa, dibulatkan
  } else {
    d.tekanan = NILAI_GAGAL;
  }
  return d;
}

void gambarDashboard(const DataLayar &d) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Judul + jumlah gambar ulang di pojok kanan atas
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("Dashboard Sensor");
  oled.setCursor(110, 0);
  oled.printf("%3lu", jumlahGambar % 1000);
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Suhu: ikon + angka besar + satuan kecil
  oled.drawBitmap(0, 14, ikonTermometer, 16, 16, SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(20, 14);
  if (d.suhuX10 == NILAI_GAGAL) {
    oled.print("--");
  } else {
    oled.print(d.suhuX10 / 10.0, 1);
  }
  int x = oled.getCursorX() + 2;  // satuan tepat di belakang angka
  oled.setTextSize(1);
  oled.setCursor(x, 14);
  oled.write(248);                // simbol derajat (butuh cp437(true))
  oled.print("C");

  // Kelembapan
  oled.drawBitmap(0, 34, ikonTetes, 16, 16, SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(20, 34);
  if (d.kelembapan == NILAI_GAGAL) {
    oled.print("--");
  } else {
    oled.print(d.kelembapan);
  }
  x = oled.getCursorX() + 2;
  oled.setTextSize(1);
  oled.setCursor(x, 34);
  oled.print("%");

  // Tekanan di baris bawah
  oled.drawLine(0, 53, 127, 53, SSD1306_WHITE);
  oled.setCursor(0, 56);
  if (d.tekanan == NILAI_GAGAL) {
    oled.print("Tekanan: BMP180 --");
  } else {
    oled.printf("Tekanan: %ld hPa", d.tekanan);
  }

  oled.display();  // satu kali kirim, di paling akhir
}

void cetakSerial(const DataLayar &d) {
  if (d.suhuX10 == NILAI_GAGAL) {
    Serial.print("DHT11 gagal dibaca");
  } else {
    Serial.printf("Suhu %.1f °C | Kelembapan %ld %%", d.suhuX10 / 10.0, d.kelembapan);
  }
  if (d.tekanan == NILAI_GAGAL) {
    Serial.print(" | Tekanan --");
  } else {
    Serial.printf(" | Tekanan %ld hPa", d.tekanan);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BAB 35 - Program 4: Dashboard Sensor");

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.beginTransmission(ALAMAT_OLED);
  if (Wire.endTransmission() != 0) {
    berhenti("OLED tidak menjawab di 0x3C! Periksa jumper cap SCL/SDA OLED.");
  }
  if (!oled.begin(SSD1306_SWITCHCAPVCC, ALAMAT_OLED)) {
    berhenti("Gagal menyiapkan OLED: RAM tidak cukup untuk buffer.");
  }
  oled.cp437(true);  // supaya write(248) = simbol derajat

  // Layar sementara selama menunggu pembacaan pertama (2 detik)
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 28);
  oled.print("Membaca sensor...");
  oled.display();

  dht.begin();
  bmpAda = bmp.begin();
  Serial.println(bmpAda ? "BMP180 siap." : "BMP180 tidak ditemukan, tekanan tidak ditampilkan.");
}

void loop() {
  unsigned long sekarang = millis();

  if (timerSensor.sudahWaktunya(sekarang)) {
    DataLayar baru = bacaSensor();
    cetakSerial(baru);

    bool berubah = !sudahDigambar ||
                   baru.suhuX10 != tampil.suhuX10 ||
                   baru.kelembapan != tampil.kelembapan ||
                   baru.tekanan != tampil.tekanan;

    if (berubah) {
      tampil = baru;
      sudahDigambar = true;
      jumlahGambar = jumlahGambar + 1;
      gambarDashboard(tampil);
      Serial.printf(" -> layar digambar ulang (ke-%lu)\n", jumlahGambar);
    } else {
      Serial.println(" -> sama, layar tidak dikirim ulang");
    }
  }
}
