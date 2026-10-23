/*
 * BAB 35 - Program 6: Menu Tombol dengan Screen Saver
 * S1 = pindah pilihan, S2 = pilih / kembali.
 * Halaman: Suhu & Lembap (DHT11), Potensiometer, Tentang.
 * Layar hanya digambar ulang kalau ada yang berubah.
 * 30 detik tanpa tombol → layar redup; 60 detik → layar mati.
 * Tekan tombol apa saja untuk menyalakannya lagi.
 *
 * Rangkaian (kabel jumper):
 *   SW1   (header kuning, pin ke-3)  → IO15
 *   SW2   (header kuning, pin ke-4)  → IO25
 *   DHT   (header hijau, pin ke-3)   → IO27
 *   VR10K (header kuning, pin ke-5)  → IO34
 *   OLED di IO21/IO22 lewat jumper cap, alamat 0x3C
 * Library: Adafruit SSD1306, Adafruit GFX Library, DHT sensor library,
 *          Adafruit Unified Sensor (semua by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define PIN_SDA      21
#define PIN_SCL      22
#define LEBAR_LAYAR  128
#define TINGGI_LAYAR 64
#define ALAMAT_OLED  0x3C
#define TOMBOL_S1    15
#define TOMBOL_S2    25
#define DHT_PIN      27
#define DHT_TYPE     DHT11
#define PIN_POT      34

#define PUTIH SSD1306_WHITE
#define HITAM SSD1306_BLACK

Adafruit_SSD1306 oled(LEBAR_LAYAR, TINGGI_LAYAR, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);

// ---- Tipe data (ditulis paling atas supaya dikenali semua fungsi) ----
enum Halaman { MENU_UTAMA, HAL_SENSOR, HAL_POT, HAL_TENTANG };
enum ModeLayar { LAYAR_NORMAL, LAYAR_REDUP, LAYAR_MATI };

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

// ---- Menu dan halaman ----
const int JUMLAH_MENU = 3;
const char *namaMenu[JUMLAH_MENU] = {"Suhu & Lembap", "Potensiometer", "Tentang"};
const Halaman tujuanMenu[JUMLAH_MENU] = {HAL_SENSOR, HAL_POT, HAL_TENTANG};

Halaman halaman = MENU_UTAMA;  // state yang sedang aktif
int pilihan = 0;               // item menu yang sedang ditandai (0-2)
bool perluGambar = true;       // true = layar harus digambar ulang

// ---- Data sensor ----
float suhu = NAN;
float kelembapan = NAN;
int nilaiPot = 0;               // nilai potensiometer yang sedang tampil
const int AMBANG_POT = 41;      // ±1% dari 4095; perubahan lebih kecil diabaikan
Timer timerDHT = {2000, 0};     // DHT11: tidak boleh lebih cepat dari 2 detik
Timer timerPot = {100, 0};      // potensiometer dicek 10 kali per detik

// ---- Screen saver ----
const unsigned long WAKTU_REDUP = 30000;  // ms tanpa tombol sebelum layar redup
const unsigned long WAKTU_MATI  = 60000;  // ms tanpa tombol sebelum layar mati
ModeLayar modeLayar = LAYAR_NORMAL;
unsigned long waktuTombolTerakhir = 0;

// ---- Debounce dua tombol (dari BAB 9): indeks 0 = S1, indeks 1 = S2 ----
const int TOMBOL_PINS[] = {TOMBOL_S1, TOMBOL_S2};
const int JUMLAH_TOMBOL = 2;
const unsigned long WAKTU_DEBOUNCE = 50;  // ms
int bacaanSebelumnya[JUMLAH_TOMBOL] = {LOW, LOW};
int statusStabil[JUMLAH_TOMBOL] = {LOW, LOW};
unsigned long waktuBerubah[JUMLAH_TOMBOL] = {0, 0};

// Mengembalikan true hanya satu kali setiap tombol ke-i baru ditekan
bool baruDitekan(int i) {
  int bacaan = digitalRead(TOMBOL_PINS[i]);

  if (bacaan != bacaanSebelumnya[i]) {
    waktuBerubah[i] = millis();
    bacaanSebelumnya[i] = bacaan;
  }

  if ((millis() - waktuBerubah[i]) > WAKTU_DEBOUNCE && bacaan != statusStabil[i]) {
    statusStabil[i] = bacaan;
    return statusStabil[i] == HIGH;  // true hanya pada tepi naik (LOW → HIGH)
  }
  return false;
}

void berhenti(const char *pesan) {
  Serial.println(pesan);
  while (true) {
    delay(1000);
  }
}

// ---------------- Menggambar setiap halaman ----------------

void gambarJudul(const char *judul) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(PUTIH);
  oled.setCursor(0, 0);
  oled.print(judul);
  oled.drawLine(0, 10, 127, 10, PUTIH);
}

void gambarPetunjuk(const char *teks) {
  oled.drawLine(0, 53, 127, 53, PUTIH);
  oled.setCursor(0, 56);
  oled.print(teks);
}

void gambarMenu() {
  gambarJudul("MENU UTAMA");
  for (int i = 0; i < JUMLAH_MENU; i++) {
    int y = 15 + i * 12;
    if (i == pilihan) {
      oled.fillRect(0, y - 2, LEBAR_LAYAR, 11, PUTIH);  // pita terang
      oled.setTextColor(HITAM);                         // teks gelap di atas pita
      oled.setCursor(2, y);
      oled.write(16);                                   // simbol ►
    } else {
      oled.setTextColor(PUTIH);
    }
    oled.setCursor(12, y);
    oled.print(namaMenu[i]);
  }
  oled.setTextColor(PUTIH);
  gambarPetunjuk("S1:pindah  S2:pilih");
}

void gambarSensor() {
  gambarJudul("Suhu & Lembap");

  oled.setTextSize(2);
  oled.setCursor(0, 16);
  if (isnan(suhu)) {
    oled.print("--");
  } else {
    oled.print(suhu, 1);
  }
  int x = oled.getCursorX() + 2;
  oled.setTextSize(1);
  oled.setCursor(x, 16);
  oled.write(248);  // simbol derajat
  oled.print("C");

  oled.setTextSize(2);
  oled.setCursor(0, 34);
  if (isnan(kelembapan)) {
    oled.print("--");
  } else {
    oled.print(kelembapan, 0);
  }
  x = oled.getCursorX() + 2;
  oled.setTextSize(1);
  oled.setCursor(x, 34);
  oled.print("%");

  gambarPetunjuk("S2: kembali");
}

void gambarPot() {
  gambarJudul("Potensiometer");

  oled.setTextSize(2);
  oled.setCursor(0, 16);
  oled.printf("%4d", nilaiPot);
  oled.setTextSize(1);
  oled.setCursor(56, 22);
  oled.printf("= %3d%%", nilaiPot * 100 / 4095);

  // Batang: bingkai 128x12, isinya sepanjang nilai potensiometer
  oled.drawRect(0, 38, LEBAR_LAYAR, 12, PUTIH);
  int panjang = map(nilaiPot, 0, 4095, 0, LEBAR_LAYAR - 4);
  oled.fillRect(2, 40, panjang, 8, PUTIH);

  gambarPetunjuk("S2: kembali");
}

void gambarTentang() {
  gambarJudul("Tentang");
  oled.setCursor(0, 14);
  oled.print("BAB 35 - OLED 128x64");
  oled.setCursor(0, 24);
  oled.print("S1 : pindah pilihan");
  oled.setCursor(0, 34);
  oled.print("S2 : pilih / kembali");
  oled.setCursor(0, 44);
  oled.print("Redup 30 s, mati 60 s");
  gambarPetunjuk("S2: kembali");
}

void gambarHalaman() {
  switch (halaman) {
    case MENU_UTAMA:  gambarMenu();    break;
    case HAL_SENSOR:  gambarSensor();  break;
    case HAL_POT:     gambarPot();     break;
    case HAL_TENTANG: gambarTentang(); break;
  }
  oled.display();  // satu kali kirim untuk satu halaman
}

// ---------------- Perpindahan state ----------------

void pindahHalaman(Halaman baru) {
  halaman = baru;
  perluGambar = true;
  if (baru == MENU_UTAMA) {
    Serial.println("Kembali ke menu");
  } else {
    Serial.printf("Masuk halaman: %s\n", namaMenu[pilihan]);
  }
}

void aturModeLayar(ModeLayar baru) {
  if (baru == LAYAR_NORMAL) {
    oled.ssd1306_command(SSD1306_DISPLAYON);   // nyalakan panel lagi
    oled.dim(false);                           // kecerahan normal
    Serial.println("Layar menyala lagi");
  } else if (baru == LAYAR_REDUP) {
    oled.dim(true);                            // kontras minimum
    Serial.println("Layar redup (30 detik tanpa tombol)");
  } else {
    oled.ssd1306_command(SSD1306_DISPLAYOFF);  // panel padam, isi GDDRAM tetap tersimpan
    Serial.println("Layar mati (60 detik tanpa tombol)");
  }
  modeLayar = baru;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BAB 35 - Program 6: Menu Tombol dengan Screen Saver");

  for (int i = 0; i < JUMLAH_TOMBOL; i++) {
    pinMode(TOMBOL_PINS[i], INPUT);  // tombol kit: pull-down 10 kΩ sudah ada
  }

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.beginTransmission(ALAMAT_OLED);
  if (Wire.endTransmission() != 0) {
    berhenti("OLED tidak menjawab di 0x3C! Periksa jumper cap SCL/SDA OLED.");
  }
  if (!oled.begin(SSD1306_SWITCHCAPVCC, ALAMAT_OLED)) {
    berhenti("Gagal menyiapkan OLED: RAM tidak cukup untuk buffer.");
  }
  oled.cp437(true);  // supaya write(248) = simbol derajat

  dht.begin();
  nilaiPot = analogRead(PIN_POT);
  waktuTombolTerakhir = millis();
  Serial.println("S1 = pindah pilihan, S2 = pilih/kembali");
}

void loop() {
  unsigned long sekarang = millis();
  bool s1 = baruDitekan(0);
  bool s2 = baruDitekan(1);

  // 1. Screen saver: tombol apa saja membangunkan layar
  if (s1 || s2) {
    waktuTombolTerakhir = sekarang;
    if (modeLayar != LAYAR_NORMAL) {
      aturModeLayar(LAYAR_NORMAL);
      perluGambar = true;  // data mungkin sudah berubah selama layar mati
      s1 = false;          // tekanan ini hanya untuk membangunkan layar,
      s2 = false;          // tidak dipakai untuk menu
    }
  }
  unsigned long lamaDiam = sekarang - waktuTombolTerakhir;
  if (modeLayar == LAYAR_NORMAL && lamaDiam >= WAKTU_REDUP) {
    aturModeLayar(LAYAR_REDUP);
  } else if (modeLayar == LAYAR_REDUP && lamaDiam >= WAKTU_MATI) {
    aturModeLayar(LAYAR_MATI);
  }

  // 2. Tugas latar: sensor tetap dibaca, apa pun halamannya
  if (timerDHT.sudahWaktunya(sekarang)) {
    dht.read(true);  // paksa membaca DHT11 sekarang (±25 ms)
    suhu = dht.readTemperature();
    kelembapan = dht.readHumidity();
    if (halaman == HAL_SENSOR) {
      perluGambar = true;  // halaman sensor diperbarui setiap 2 detik
    }
  }
  if (timerPot.sudahWaktunya(sekarang)) {
    int bacaan = analogRead(PIN_POT);
    if (abs(bacaan - nilaiPot) >= AMBANG_POT) {  // abaikan getaran kecil (noise)
      nilaiPot = bacaan;
      if (halaman == HAL_POT) {
        perluGambar = true;
      }
    }
  }

  // 3. State machine halaman
  switch (halaman) {
    case MENU_UTAMA:
      if (s1) {
        pilihan = (pilihan + 1) % JUMLAH_MENU;  // setelah item terakhir kembali ke atas
        perluGambar = true;
        Serial.printf("Pilihan: %s\n", namaMenu[pilihan]);
      }
      if (s2) {
        pindahHalaman(tujuanMenu[pilihan]);
      }
      break;

    case HAL_SENSOR:
    case HAL_POT:
    case HAL_TENTANG:
      if (s2) {
        pindahHalaman(MENU_UTAMA);
      }
      break;
  }

  // 4. Gambar ulang hanya kalau perlu, dan tidak saat layar mati
  if (perluGambar && modeLayar != LAYAR_MATI) {
    gambarHalaman();
    perluGambar = false;
  }
}
