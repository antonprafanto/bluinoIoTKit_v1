/*
 * BAB 20 - Program 2: Mengukur Pemakaian Stack
 * Membandingkan array static dan array lokal, lalu menunjukkan bahwa
 * rekursi yang makin dalam terus menghabiskan stack loop() (8 KB).
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino (tidak perlu merangkai)
 * Alat uji : Serial Monitor, baud 115200
 */

const unsigned int BATAS_AMAN = 2000;  // byte: berhenti kalau sisa stack di bawah ini

// Mencetak keterangan + sisa stack minimum loopTask saat ini
void cetakSisaStack(const char *keterangan) {
  Serial.print(keterangan);
  Serial.print(" -> sisa stack minimum: ");
  Serial.print(uxTaskGetStackHighWaterMark(NULL));
  Serial.println(" byte");
}

// __attribute__((noinline)): fungsi ini tidak boleh "dilebur" ke pemanggilnya,
// supaya pemakaian stack-nya benar-benar terjadi saat fungsi dipanggil.
// volatile: compiler tidak boleh membuang array yang "kelihatannya" tidak dipakai.

__attribute__((noinline)) uint8_t pakaiArrayStatic() {
  static volatile uint8_t data[1000];  // static: di memori statis, BUKAN di stack
  for (int i = 0; i < 1000; i++) {
    data[i] = i;
  }
  return data[999];  // baca kembali supaya array benar-benar terpakai
}

__attribute__((noinline)) uint8_t pakaiArrayLokal() {
  volatile uint8_t data[1000];  // lokal: 1000 byte di stack selama fungsi berjalan
  for (int i = 0; i < 1000; i++) {
    data[i] = i;
  }
  return data[999];
}

// Fungsi rekursif: memanggil dirinya sendiri sampai "sisa" bernilai 0.
// Setiap level memakai array lokal 100 byte + catatan pemanggilan fungsi.
__attribute__((noinline)) int rekursi(int sisa) {
  volatile uint8_t catatan[100];
  for (int i = 0; i < 100; i++) {
    catatan[i] = sisa;
  }
  if (sisa <= 0) {
    return 0;
  }
  return rekursi(sisa - 1) + catatan[0];
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BAB 20 - Program 2: Mengukur Pemakaian Stack");
  Serial.print("Ukuran stack loop(): ");
  Serial.print(getArduinoLoopTaskStackSize());
  Serial.println(" byte");
  Serial.println();

  // Percobaan 1: array static vs array lokal
  cetakSisaStack("Awal program          ");
  pakaiArrayStatic();
  cetakSisaStack("Array static 1000 byte");
  pakaiArrayLokal();
  cetakSisaStack("Array lokal 1000 byte ");
  Serial.println();

  // Percobaan 2: rekursi makin dalam, 5 level setiap langkah
  for (int level = 10; level <= 60; level += 5) {
    rekursi(level);

    Serial.print("Rekursi ");
    Serial.print(level);
    Serial.print(" level");
    cetakSisaStack("");

    if (uxTaskGetStackHighWaterMark(NULL) < BATAS_AMAN) {
      Serial.println("Berhenti: sisa stack di bawah 2000 byte. Lebih dalam lagi bisa stack overflow!");
      break;
    }
  }
}

void loop() {
  // Kosong: semua percobaan sudah dijalankan sekali di setup().
}
