/*
 * BAB 27 - Program 3: Moving Average di Serial Plotter
 * Membaca DS18B20 setiap 1 detik tanpa menghambat program (dua langkah:
 * kirim perintah ukur, lalu ambil hasil setelah waktu konversi habis),
 * menghitung rata-rata bergerak 8 bacaan terakhir, lalu mengirim
 * suhu mentah dan suhu halus ke Serial Plotter.
 * Buka Tools → Serial Plotter, lalu pilih baud 115200.
 *
 * Rangkaian: pin DS (header hijau) → IO14
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ONEWIRE   14  // jalur data DS18B20 (pin DS -> IO14)
#define RESOLUSI      12  // langkah 0,0625 °C, konversi maks. 750 ms
#define UKURAN_FILTER  8  // banyaknya bacaan terakhir yang dirata-rata

const unsigned long JEDA_UKUR = 1000;  // mulai pengukuran baru setiap 1000 ms

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensor(&oneWire);

// Dua keadaan pembacaan sensor (seperti BAB 25)
enum StatusSensor { DIAM, MENGUKUR };
StatusSensor status = DIAM;

unsigned long lamaKonversi = 750;     // diisi ulang di setup() sesuai RESOLUSI
unsigned long waktuUkurTerakhir = 0;  // kapan perintah ukur terakhir dikirim

// Penyimpan bacaan untuk moving average (buffer melingkar)
float bufferSuhu[UKURAN_FILTER];
int posisi = 0;  // tempat bacaan berikutnya ditulis
int terisi = 0;  // berapa bacaan yang sudah tersimpan (maks. UKURAN_FILTER)

// Masukkan satu bacaan baru, lalu kembalikan rata-rata bacaan yang tersimpan
float tambahDanRataRata(float bacaanBaru) {
  bufferSuhu[posisi] = bacaanBaru;        // timpa bacaan paling lama
  posisi = (posisi + 1) % UKURAN_FILTER;  // maju satu, kembali ke 0 setelah ujung
  if (terisi < UKURAN_FILTER) {
    terisi = terisi + 1;
  }

  float jumlah = 0;
  for (int i = 0; i < terisi; i++) {
    jumlah = jumlah + bufferSuhu[i];
  }
  return jumlah / terisi;
}

// true kalau angka dari DS18B20 masuk akal (sama seperti Program 1)
bool suhuValid(float suhu) {
  if (suhu == DEVICE_DISCONNECTED_C) return false;
  if (suhu < -55.0 || suhu > 125.0) return false;
  if (suhu == 85.0) return false;
  return true;
}

void setup() {
  Serial.begin(115200);
  sensor.begin();
  sensor.setResolution(RESOLUSI);
  sensor.setWaitForConversion(false);  // requestTemperatures() tidak menunggu
  lamaKonversi = sensor.millisToWaitForConversion(RESOLUSI);
  // Sengaja tidak mencetak judul: Serial Plotter menganggap setiap baris sebagai data grafik.
}

void loop() {
  unsigned long sekarang = millis();

  switch (status) {
    case DIAM:
      if (sekarang - waktuUkurTerakhir >= JEDA_UKUR) {
        waktuUkurTerakhir = sekarang;
        sensor.requestTemperatures();  // langkah 1: kirim perintah ukur
        status = MENGUKUR;
      }
      break;

    case MENGUKUR:
      if (sekarang - waktuUkurTerakhir >= lamaKonversi) {
        float suhu = sensor.getTempCByIndex(0);  // langkah 2: ambil hasilnya
        if (suhuValid(suhu)) {
          float suhuHalus = tambahDanRataRata(suhu);
          Serial.printf("Mentah:%.2f,RataRata:%.2f\n", suhu, suhuHalus);
        }
        status = DIAM;
      }
      break;
  }

  // Tugas lain bisa ditambahkan di sini; loop() tidak pernah tertahan lama.
}
