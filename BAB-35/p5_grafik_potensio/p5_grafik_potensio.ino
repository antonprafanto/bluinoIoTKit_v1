/*
 * BAB 35 - Program 5: Grafik Potensiometer
 * Membaca potensiometer dan menggambar grafik 128 sampel terakhir
 * (±12,8 detik) yang terus bergeser ke kiri. Layar diperbarui dengan
 * laju tetap: 10 kali per detik.
 *
 * Rangkaian: pin VR10K (header kuning, pin ke-5 dari kiri) → IO34 (kabel jumper)
 *            OLED di IO21/IO22 lewat jumper cap, alamat 0x3C
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
#define PIN_POT      34

Adafruit_SSD1306 oled(LEBAR_LAYAR, TINGGI_LAYAR, &Wire, -1);

// Area grafik di layar
const int JUMLAH_SAMPEL = 128;  // satu sampel untuk setiap kolom piksel
const int GRAFIK_ATAS   = 12;   // y teratas area grafik
const int GRAFIK_BAWAH  = 63;   // y terbawah area grafik

// Penyimpanan melingkar (circular buffer) untuk 128 sampel terakhir
int sampel[JUMLAH_SAMPEL];
int posisiTulis  = 0;  // indeks tempat sampel berikutnya ditulis
int jumlahTerisi = 0;  // berapa sampel yang sudah ada (maksimal 128)

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

Timer timerGambar  = {100, 0};   // 10 kali per detik
Timer timerLaporan = {1000, 0};  // laporan ke Serial setiap detik
unsigned long jumlahFrame = 0;   // berapa kali display() sejak laporan terakhir

void berhenti(const char *pesan) {
  Serial.println(pesan);
  while (true) {
    delay(1000);
  }
}

void simpanSampel(int nilai) {
  sampel[posisiTulis] = nilai;
  posisiTulis = (posisiTulis + 1) % JUMLAH_SAMPEL;  // setelah 127 kembali ke 0
  if (jumlahTerisi < JUMLAH_SAMPEL) {
    jumlahTerisi = jumlahTerisi + 1;
  }
}

// Mengubah nilai ADC (0-4095) menjadi koordinat y: makin besar nilai, makin ke atas
int keY(int nilai) {
  return map(nilai, 0, 4095, GRAFIK_BAWAH, GRAFIK_ATAS);
}

void gambarGrafik(int nilaiSekarang) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.printf("Potensio:%5d %3d%%", nilaiSekarang, nilaiSekarang * 100 / 4095);
  oled.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // Garis bantu putus-putus di nilai tengah (2048 = 50%)
  int yTengah = keY(2048);
  for (int x = 0; x < LEBAR_LAYAR; x += 4) {
    oled.drawPixel(x, yTengah, SSD1306_WHITE);
  }

  // Grafik: sampel tertua di kiri, sampel terbaru di kolom paling kanan
  int xAwal = JUMLAH_SAMPEL - jumlahTerisi;
  for (int i = 1; i < jumlahTerisi; i++) {
    int iLama = (posisiTulis - jumlahTerisi + i - 1 + JUMLAH_SAMPEL) % JUMLAH_SAMPEL;
    int iBaru = (iLama + 1) % JUMLAH_SAMPEL;
    oled.drawLine(xAwal + i - 1, keY(sampel[iLama]),
                  xAwal + i,     keY(sampel[iBaru]), SSD1306_WHITE);
  }

  oled.display();  // satu kali kirim per frame
  jumlahFrame = jumlahFrame + 1;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BAB 35 - Program 5: Grafik Potensiometer");

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.beginTransmission(ALAMAT_OLED);
  if (Wire.endTransmission() != 0) {
    berhenti("OLED tidak menjawab di 0x3C! Periksa jumper cap SCL/SDA OLED.");
  }
  if (!oled.begin(SSD1306_SWITCHCAPVCC, ALAMAT_OLED)) {
    berhenti("Gagal menyiapkan OLED: RAM tidak cukup untuk buffer.");
  }
  oled.clearDisplay();
  oled.display();

  // Kedua timer mulai dihitung dari sekarang, bukan dari ESP32 menyala
  unsigned long sekarang = millis();
  timerGambar.waktuLalu = sekarang;
  timerLaporan.waktuLalu = sekarang;
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: baca, simpan, dan gambar dengan laju tetap
  if (timerGambar.sudahWaktunya(sekarang)) {
    int nilai = analogRead(PIN_POT);  // 0-4095
    simpanSampel(nilai);
    gambarGrafik(nilai);
  }

  // Tugas 2: laporan setiap detik
  if (timerLaporan.sudahWaktunya(sekarang)) {
    int terakhir = sampel[(posisiTulis - 1 + JUMLAH_SAMPEL) % JUMLAH_SAMPEL];
    Serial.printf("Potensio: %4d (%3d%%) | layar: %lu frame/detik\n",
                  terakhir, terakhir * 100 / 4095, jumlahFrame);
    jumlahFrame = 0;
  }
}
