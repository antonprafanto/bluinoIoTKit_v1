/*
 * BAB 39 - Program 2: Lampu yang Ingat Mode Terakhir
 * Tombol S1 mengganti mode: MATI -> MERAH -> HIJAU -> BIRU -> KUNING -> MATI ...
 * Mode disimpan ke NVS 3 detik setelah tekanan terakhir (simpan tertunda),
 * dan hanya kalau berbeda dari yang sudah tersimpan.
 *
 * Rangkaian (header kit → pin ESP32):
 *   SW1 (tombol S1) → IO15
 *   LED1 (merah)    → IO4     LED2 (hijau)  → IO17
 *   LED3 (biru)     → IO2     LED4 (kuning) → IO16
 * Alat uji: Serial Monitor, baud 115200
 */

#include <Preferences.h>

#define TOMBOL_S1  15
#define LED_MERAH  4
#define LED_HIJAU  17
#define LED_BIRU   2
#define LED_KUNING 16

const int JUMLAH_LED = 4;
const int PIN_LED[JUMLAH_LED] = {LED_MERAH, LED_HIJAU, LED_BIRU, LED_KUNING};

const uint8_t JUMLAH_MODE = 5;
const char *NAMA_MODE[JUMLAH_MODE] = {"MATI", "MERAH", "HIJAU", "BIRU", "KUNING"};

const unsigned long WAKTU_DEBOUNCE = 50;    // ms, sama seperti BAB 9
const unsigned long JEDA_SIMPAN    = 3000;  // ms: simpan 3 detik setelah tekanan terakhir

Preferences prefs;

uint8_t modeLampu = 0;        // mode yang sedang aktif (di RAM)
uint8_t modeTersimpan = 0;    // mode yang sudah tersimpan di NVS
bool perluDisimpan = false;   // ada perubahan yang belum disimpan?
unsigned long waktuUbah = 0;  // kapan mode terakhir kali diubah
uint32_t jumlahTulis = 0;     // berapa kali program ini menulis ke NVS sejak menyala

// Variabel debounce tombol S1 (pola BAB 9)
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

// Menyalakan LED sesuai mode: mode 1-4 menyalakan satu LED, mode 0 mematikan semua
void tampilkanMode() {
  for (int i = 0; i < JUMLAH_LED; i++) {
    digitalWrite(PIN_LED[i], (modeLampu == i + 1) ? HIGH : LOW);
  }
}

// Menulis mode ke NVS, tetapi hanya kalau berbeda dari yang sudah tersimpan
void simpanMode() {
  if (modeLampu == modeTersimpan) {
    Serial.println("  Mode sama dengan yang tersimpan, tidak perlu menulis ke flash.");
    return;
  }
  prefs.begin("lampu", false);
  size_t tertulis = prefs.putUChar("mode", modeLampu);
  prefs.end();

  if (tertulis == 0) {
    Serial.println("  Gagal menyimpan ke NVS!");
    return;
  }
  modeTersimpan = modeLampu;
  jumlahTulis = jumlahTulis + 1;
  Serial.printf("  Disimpan ke NVS: mode %s (penulisan ke-%lu sejak menyala)\n",
                NAMA_MODE[modeLampu], (unsigned long)jumlahTulis);
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);  // tombol kit punya pull-down 10 kΩ: ditekan = HIGH
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(PIN_LED[i], OUTPUT);
  }
  delay(1000);

  Serial.println();
  Serial.println("BAB 39 - Program 2: Lampu yang Ingat Mode Terakhir");

  // Mode baca-tulis supaya namespace dibuat kalau belum ada
  prefs.begin("lampu", false);
  bool adaData = prefs.isKey("mode");
  modeLampu = prefs.getUChar("mode", 0);  // 0 (MATI) kalau belum pernah disimpan
  prefs.end();

  if (modeLampu >= JUMLAH_MODE) {         // jaga-jaga kalau isinya tidak masuk akal
    modeLampu = 0;
  }
  modeTersimpan = modeLampu;

  if (adaData) {
    Serial.printf("Mode terakhir dari NVS: %s\n", NAMA_MODE[modeLampu]);
  } else {
    Serial.println("Belum ada data di NVS, mulai dari mode MATI.");
  }
  Serial.println("Tekan S1 untuk mengganti mode.");
  tampilkanMode();
}

void loop() {
  unsigned long sekarang = millis();

  // 1. Baca tombol S1 dengan debounce
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = sekarang;
    bacaanSebelumnya = bacaan;
  }
  if ((sekarang - waktuBerubah) > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    if (statusStabil == HIGH) {                     // S1 baru ditekan
      modeLampu = (modeLampu + 1) % JUMLAH_MODE;    // pindah ke mode berikutnya
      tampilkanMode();
      Serial.printf("Mode: %s (belum disimpan)\n", NAMA_MODE[modeLampu]);
      perluDisimpan = true;
      waktuUbah = sekarang;                         // hitungan 3 detik diulang dari nol
    }
  }

  // 2. Simpan tertunda: tulis ke NVS kalau sudah 3 detik tidak ada tekanan baru
  if (perluDisimpan && (sekarang - waktuUbah >= JEDA_SIMPAN)) {
    perluDisimpan = false;
    simpanMode();
  }
}
