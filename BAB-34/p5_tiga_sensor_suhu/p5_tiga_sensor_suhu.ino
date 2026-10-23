/*
 * BAB 34 - Program 5: Tiga Sensor Suhu, Satu Angka
 * Membaca suhu dari DS18B20, BMP180, dan DHT11 setiap 2 detik tanpa delay().
 * Setiap bacaan diperiksa (data gagal, batas wajar, laju perubahan),
 * dihaluskan dengan EMA, lalu ketiganya digabung:
 *   - median untuk mencari sensor yang mencurigakan,
 *   - rata-rata berbobot dari sensor yang sehat sebagai hasil akhir.
 *
 * Rangkaian: DS (header hijau) → IO14, DHT (header hijau) → IO27,
 *            BMP180 lewat jumper cap JP (SDA = IO21, SCL = IO22)
 * Library  : DHT sensor library, Adafruit Unified Sensor, OneWire,
 *            DallasTemperature, Adafruit BMP085 Library
 */

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BMP085.h>

#define PIN_DS  14  // DS18B20 (pin DS) → IO14
#define PIN_DHT 27  // DHT11 (pin DHT) → IO27

// ── Pengaturan pengolahan data ──
const unsigned long JEDA_BACA_MS = 2000;    // DHT11: jangan kurang dari 2000 ms
const float ALPHA = 0.3;                    // EMA untuk setiap sensor
const float MAKS_PERUBAHAN = 3.0;           // °C per bacaan; lebih dari ini dianggap lonjakan
const int   BATAS_TOLAK_BERUNTUN = 3;       // menyimpang 3 kali berturut-turut = perubahan nyata
const float BATAS_BEDA_MEDIAN = 3.0;        // °C; lebih jauh dari median = mencurigakan
const unsigned long BATAS_BASI_MS = 10000;  // tanpa bacaan valid lebih dari 10 detik = tidak ada data

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

// Satu paket data untuk setiap sensor
struct SensorSuhu {
  const char *nama;
  float akurasi;             // ± °C menurut datasheet, untuk menghitung bobot
  float batasBawah;          // batas wajar (°C)
  float batasAtas;
  float ema;                 // suhu yang sudah dihaluskan
  float pembanding;          // bacaan terakhir yang diterima (untuk cek laju)
  int   tolakBeruntun;       // berapa kali berturut-turut menyimpang terlalu jauh
  bool  adaData;             // sudah pernah menerima bacaan valid?
  unsigned long waktuValid;  // millis() saat bacaan valid terakhir
};

const int JUMLAH_SENSOR = 3;
const int S_DS = 0, S_BMP = 1, S_DHT = 2;  // nomor urut setiap sensor di array

SensorSuhu sensor[JUMLAH_SENSOR] = {
  // nama      akurasi  bawah   atas    ema  pembanding tolak adaData waktu
  {"DS18B20",  0.5,    -55.0,  125.0,  NAN, NAN,       0,    false,  0},
  {"BMP180",   1.0,    -40.0,   85.0,  NAN, NAN,       0,    false,  0},
  {"DHT11",    2.0,      0.0,   50.0,  NAN, NAN,       0,    false,  0},
};

OneWire oneWire(PIN_DS);
DallasTemperature ds18b20(&oneWire);
Adafruit_BMP085 bmp;
DHT dht(PIN_DHT, DHT11);
bool bmpAda = false;

Timer timerBaca = {JEDA_BACA_MS, 0};

float median3(float a, float b, float c) {
  if (a > b) { float t = a; a = b; b = t; }
  if (b > c) { float t = b; b = c; c = t; }
  if (a > b) { float t = a; a = b; b = t; }
  return b;
}

float bobotSensor(const SensorSuhu &s) {
  return 1.0 / (s.akurasi * s.akurasi);  // makin akurat, makin besar bobotnya
}

// Periksa satu bacaan mentah, lalu masukkan ke EMA kalau lolos
void prosesBacaan(SensorSuhu &s, float mentah, unsigned long sekarang) {
  // Lapis 1: data gagal
  if (isnan(mentah)) {
    Serial.printf("  ! %s: data gagal, dilewati\n", s.nama);
    return;
  }
  // Lapis 2: batas wajar
  if (mentah < s.batasBawah || mentah > s.batasAtas) {
    Serial.printf("  ! %s: %.1f °C di luar batas wajar, ditolak\n", s.nama, mentah);
    return;
  }
  // Lapis 3: laju perubahan (dilewati kalau belum punya data atau datanya sudah basi)
  bool mulaiBaru = !s.adaData || (sekarang - s.waktuValid > BATAS_BASI_MS);
  if (!mulaiBaru && fabsf(mentah - s.pembanding) > MAKS_PERUBAHAN) {
    s.tolakBeruntun = s.tolakBeruntun + 1;
    if (s.tolakBeruntun < BATAS_TOLAK_BERUNTUN) {
      Serial.printf("  ! %s: %.1f °C berubah terlalu cepat, ditolak\n", s.nama, mentah);
      return;
    }
    Serial.printf("  ! %s: %.1f °C menyimpang %d kali berturut-turut, dianggap perubahan nyata\n",
                  s.nama, mentah, s.tolakBeruntun);
    mulaiBaru = true;
  }
  // Lolos semua pemeriksaan
  s.tolakBeruntun = 0;
  s.pembanding = mentah;
  s.waktuValid = sekarang;
  s.adaData = true;
  if (mulaiBaru) {
    s.ema = mentah;                            // mulai EMA dari bacaan ini
  } else {
    s.ema = s.ema + ALPHA * (mentah - s.ema);  // EMA biasa
  }
}

void bacaSemuaSensor(unsigned long sekarang) {
  // DS18B20: ambil hasil pengukuran yang dimulai 2 detik lalu, lalu mulai pengukuran baru
  float suhuDS = ds18b20.getTempCByIndex(0);
  if (suhuDS == DEVICE_DISCONNECTED_C || suhuDS == 85.0) {
    suhuDS = NAN;  // -127 (tidak terhubung) atau 85 (belum pernah mengukur) = gagal
  }
  ds18b20.requestTemperatures();  // tidak menunggu, karena setWaitForConversion(false)
  prosesBacaan(sensor[S_DS], suhuDS, sekarang);

  // BMP180: hanya kalau terdeteksi saat setup()
  if (bmpAda) {
    prosesBacaan(sensor[S_BMP], bmp.readTemperature(), sekarang);
  }

  // DHT11: paksa membaca sekarang; timer sudah menjamin jeda 2 detik (lihat BAB 26)
  dht.read(true);
  prosesBacaan(sensor[S_DHT], dht.readTemperature(), sekarang);
}

void gabungkanDanCetak(unsigned long sekarang) {
  // 1. Sensor mana yang datanya masih segar?
  bool segar[JUMLAH_SENSOR];
  float nilaiSegar[JUMLAH_SENSOR];
  int jumlahSegar = 0;
  for (int i = 0; i < JUMLAH_SENSOR; i++) {
    segar[i] = sensor[i].adaData && (sekarang - sensor[i].waktuValid <= BATAS_BASI_MS);
    if (segar[i]) {
      nilaiSegar[jumlahSegar] = sensor[i].ema;
      jumlahSegar = jumlahSegar + 1;
    }
  }

  // 2. Median dari sensor yang segar
  float median = NAN;
  if (jumlahSegar == 3) {
    median = median3(nilaiSegar[0], nilaiSegar[1], nilaiSegar[2]);
  } else if (jumlahSegar == 2) {
    median = (nilaiSegar[0] + nilaiSegar[1]) / 2.0;  // median dua angka = rata-ratanya
  } else if (jumlahSegar == 1) {
    median = nilaiSegar[0];
  }

  // 3. Tandai yang mencurigakan (hanya bisa kalau ada 3 sensor), lalu hitung rata-rata berbobot
  bool curiga[JUMLAH_SENSOR];
  float jumlahBobot = 0;
  float jumlahBerbobot = 0;
  int jumlahDipakai = 0;
  for (int i = 0; i < JUMLAH_SENSOR; i++) {
    curiga[i] = segar[i] && jumlahSegar == 3 && fabsf(sensor[i].ema - median) > BATAS_BEDA_MEDIAN;
    if (segar[i] && !curiga[i]) {
      jumlahBobot = jumlahBobot + bobotSensor(sensor[i]);
      jumlahBerbobot = jumlahBerbobot + bobotSensor(sensor[i]) * sensor[i].ema;
      jumlahDipakai = jumlahDipakai + 1;
    }
  }

  // 4. Cetak laporan
  Serial.printf("--- Detik ke-%lu ---\n", sekarang / 1000);
  for (int i = 0; i < JUMLAH_SENSOR; i++) {
    if (!segar[i]) {
      Serial.printf("%-8s:  --.- °C  TIDAK ADA DATA\n", sensor[i].nama);
    } else if (curiga[i]) {
      Serial.printf("%-8s: %5.1f °C  CURIGA (beda %.1f °C dari median), tidak dipakai\n",
                    sensor[i].nama, sensor[i].ema, fabsf(sensor[i].ema - median));
    } else {
      Serial.printf("%-8s: %5.1f °C  OK, bobot %.0f %%\n",
                    sensor[i].nama, sensor[i].ema, 100.0 * bobotSensor(sensor[i]) / jumlahBobot);
    }
  }

  if (jumlahDipakai == 0) {
    Serial.println("Gabungan: tidak ada sensor yang bisa dipakai!");
  } else {
    Serial.printf("Median  : %5.1f °C\n", median);
    Serial.printf("Gabungan: %5.1f °C (dari %d sensor)\n", jumlahBerbobot / jumlahBobot, jumlahDipakai);
    if (jumlahSegar == 2 && fabsf(nilaiSegar[0] - nilaiSegar[1]) > BATAS_BEDA_MEDIAN) {
      Serial.println("Peringatan: dua sensor tidak sepakat, belum bisa ditentukan mana yang salah");
    }
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 34 - Program 5: Tiga Sensor Suhu, Satu Angka");

  ds18b20.begin();
  ds18b20.setResolution(12);            // langkah 0,0625 °C, pengukuran paling lama 750 ms
  ds18b20.setWaitForConversion(false);  // requestTemperatures() tidak menunggu
  ds18b20.requestTemperatures();        // mulai pengukuran pertama; hasilnya diambil 2 detik lagi
  if (ds18b20.getDeviceCount() == 0) {
    Serial.println("Peringatan: DS18B20 tidak ditemukan. Periksa kabel DS -> IO14.");
  }

  bmpAda = bmp.begin();
  if (!bmpAda) {
    Serial.println("Peringatan: BMP180 tidak terdeteksi. Periksa jumper cap JP SCL/SDA BMP180.");
  }

  dht.begin();
  Serial.println("Bacaan pertama muncul setelah 2 detik...");
}

void loop() {
  unsigned long sekarang = millis();

  if (timerBaca.sudahWaktunya(sekarang)) {
    bacaSemuaSensor(sekarang);
    gabungkanDanCetak(sekarang);
  }

  // Tugas lain bisa ditambahkan di sini, misalnya menampilkan suhu gabungan di OLED (BAB 35).
}
