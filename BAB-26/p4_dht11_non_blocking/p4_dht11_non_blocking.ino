/*
 * BAB 26 - Program 4: Membaca DHT11 Tanpa delay()
 * DHT11 dibaca setiap 2 detik memakai millis(), sementara LED biru
 * bawaan board terus berkedip setiap 250 ms tanpa tersendat.
 * Bacaan valid terakhir disimpan dalam struct supaya bisa dipakai
 * bagian program lain (misalnya OLED atau halaman web).
 *
 * Rangkaian: pin DHT (header hijau) → IO27
 *            LED biru bawaan board ESP32 sudah tersambung ke IO2
 */

#include <DHT.h>

#define DHT_PIN  27
#define DHT_TYPE DHT11
#define LED_BIRU 2  // LED biru bawaan board (juga LED3 kit kalau disambung ke IO2)

DHT dht(DHT_PIN, DHT_TYPE);

// Timer serbaguna dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

// Paket data: hasil bacaan valid terakhir
struct DataDHT {
  float suhu;               // °C
  float kelembapan;         // %RH
  float heatIndex;          // °C
  unsigned long waktuBaca;  // millis() saat data ini didapat
  bool adaData;             // sudah pernah berhasil membaca?
};

Timer timerDHT = {2000, 0};  // baca DHT11 setiap 2 detik (jangan kurang dari 2000)
Timer timerLed = {250, 0};   // balik keadaan LED setiap 250 ms

DataDHT dataTerakhir = {NAN, NAN, NAN, 0, false};
int gagalBeruntun = 0;       // berapa kali gagal berturut-turut
bool ledNyala = false;

void bacaDHT(unsigned long sekarang) {
  unsigned long mulai = micros();
  dht.read(true);  // paksa membaca sensor sekarang (±25 ms)
  unsigned long lamaBacaMs = (micros() - mulai) / 1000;

  // Ambil angka dari bacaan barusan (tidak menghubungi sensor lagi)
  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();

  if (isnan(kelembapan) || isnan(suhu)) {
    gagalBeruntun = gagalBeruntun + 1;
    Serial.printf("[%6lu ms] Gagal membaca (%d kali berturut-turut).", sekarang, gagalBeruntun);
    if (dataTerakhir.adaData) {
      Serial.printf(" Data terakhir: %.1f °C, %.1f %%RH, %lu detik lalu\n",
                    dataTerakhir.suhu, dataTerakhir.kelembapan,
                    (sekarang - dataTerakhir.waktuBaca) / 1000);
    } else {
      Serial.println(" Belum ada data valid.");
    }
    return;  // dataTerakhir sengaja tidak diubah
  }

  gagalBeruntun = 0;
  dataTerakhir.suhu = suhu;
  dataTerakhir.kelembapan = kelembapan;
  dataTerakhir.heatIndex = dht.computeHeatIndex(suhu, kelembapan, false);
  dataTerakhir.waktuBaca = sekarang;
  dataTerakhir.adaData = true;

  Serial.printf("[%6lu ms] Suhu %.1f °C | Kelembapan %.1f %%RH | Heat index %.1f °C | lama baca %lu ms\n",
                sekarang, suhu, kelembapan, dataTerakhir.heatIndex, lamaBacaMs);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);
  dht.begin();
  Serial.println("BAB 26 - Program 4: Membaca DHT11 Tanpa delay()");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: kedipkan LED
  if (timerLed.sudahWaktunya(sekarang)) {
    ledNyala = !ledNyala;
    digitalWrite(LED_BIRU, ledNyala ? HIGH : LOW);
  }

  // Tugas 2: baca DHT11 setiap 2 detik
  if (timerDHT.sudahWaktunya(sekarang)) {
    bacaDHT(sekarang);
  }

  // Tugas lain bisa ditambahkan di sini dan memakai dataTerakhir kapan saja,
  // misalnya menampilkan dataTerakhir.suhu di OLED (BAB 35).
}
