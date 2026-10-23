/*
 * BAB 25 - Program 3: Suhu Tanpa Menghambat Program
 * Membaca DS18B20 setiap 2 detik dalam dua langkah: kirim perintah ukur,
 * lalu ambil hasilnya setelah waktu konversi habis (memakai millis()).
 * Sementara itu LED biru bawaan board berkedip tanpa tersendat,
 * sebagai bukti bahwa loop() tidak pernah tertahan.
 *
 * Rangkaian: kabel jumper dari pin DS (header hijau) ke IO14.
 * LED biru bawaan board ESP32 (IO2) tidak perlu kabel.
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ONEWIRE 14   // jalur data DS18B20 (pin DS -> IO14)
#define LED_BIRU     2   // LED biru bawaan board ESP32
#define RESOLUSI    12   // resolusi sensor: 9, 10, 11, atau 12 bit

const unsigned long JEDA_UKUR  = 2000;  // mulai pengukuran baru setiap 2000 ms
const unsigned long JEDA_KEDIP = 250;   // LED biru berganti nyala/padam setiap 250 ms

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensor(&oneWire);

// Dua keadaan pembacaan sensor
enum StatusSensor { DIAM, MENGUKUR };
StatusSensor status = DIAM;

unsigned long lamaKonversi = 750;       // diisi ulang di setup() sesuai RESOLUSI
unsigned long waktuUkurTerakhir = 0;    // kapan perintah ukur terakhir dikirim
unsigned long waktuKedipTerakhir = 0;
bool ledNyala = false;
float suhuTerakhir = NAN;               // suhu valid terakhir (NAN = belum ada), siap dipakai tugas lain

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  pinMode(LED_BIRU, OUTPUT);

  Serial.println("BAB 25 - Program 3: Suhu Tanpa Menghambat Program");

  sensor.begin();
  sensor.setResolution(RESOLUSI);       // atur resolusi semua sensor di bus
  sensor.setWaitForConversion(false);   // requestTemperatures() tidak lagi menunggu
  lamaKonversi = sensor.millisToWaitForConversion(RESOLUSI);

  Serial.printf("Perangkat 1-Wire: %u, resolusi %d bit, waktu konversi %lu ms\n",
                sensor.getDeviceCount(), RESOLUSI, lamaKonversi);
  if (sensor.getDeviceCount() == 0) {
    Serial.println("Sensor tidak ditemukan! Periksa kabel DS -> 14, lalu tekan EN.");
  }
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: LED biru berkedip. Kalau loop() sempat tertahan, kedipannya tersendat.
  if (sekarang - waktuKedipTerakhir >= JEDA_KEDIP) {
    waktuKedipTerakhir = sekarang;
    ledNyala = !ledNyala;
    digitalWrite(LED_BIRU, ledNyala);
  }

  // Tugas 2: membaca DS18B20 dalam dua langkah
  switch (status) {
    case DIAM:
      if (sekarang - waktuUkurTerakhir >= JEDA_UKUR) {
        waktuUkurTerakhir = sekarang;
        sensor.requestTemperatures();          // langkah 1: kirim perintah ukur, langsung kembali
        status = MENGUKUR;
      }
      break;

    case MENGUKUR:
      if (sekarang - waktuUkurTerakhir >= lamaKonversi) {
        float suhu = sensor.getTempCByIndex(0);  // langkah 2: ambil hasilnya
        if (suhu == DEVICE_DISCONNECTED_C) {
          Serial.printf("[%6lu ms] Gagal membaca sensor (-127)\n", sekarang);
        } else {
          suhuTerakhir = suhu;
          Serial.printf("[%6lu ms] Suhu: %.2f °C\n", sekarang, suhu);
        }
        status = DIAM;
      }
      break;
  }
}
