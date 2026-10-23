/*
 * BAB 25 - Program 4: Membaca Sensor Berdasarkan Alamat
 * Membaca suhu setiap sensor di daftarSensor[] berdasarkan kode ROM-nya
 * (alamat), sehingga setiap nama selalu menempel pada sensor yang sama.
 * Pola pembacaannya sama dengan Program 3 (tanpa menunggu, memakai millis()).
 *
 * Persiapan: jalankan Program 2, lalu ganti isi daftarSensor[] dengan
 * baris "Baris untuk Program 4" yang dicetaknya.
 * Rangkaian: kabel jumper dari pin DS (header hijau) ke IO14.
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ONEWIRE 14   // jalur data DS18B20 (pin DS -> IO14)
#define RESOLUSI    12   // resolusi sensor: 9, 10, 11, atau 12 bit

const unsigned long JEDA_UKUR = 2000;   // mulai pengukuran baru setiap 2000 ms

// Satu sensor = satu kode ROM (alamat) + satu nama
struct SensorSuhu {
  DeviceAddress alamat;
  const char *nama;
};

// Ganti baris contoh di bawah dengan hasil Program 2 (alamat sensormu pasti berbeda).
// Untuk sensor kedua, tambahkan barisnya tepat di bawah baris pertama.
SensorSuhu daftarSensor[] = {
  { {0x28, 0x61, 0x64, 0x12, 0x3C, 0x7C, 0x2F, 0x27}, "Sensor kit" },
};
const int JUMLAH_SENSOR = sizeof(daftarSensor) / sizeof(daftarSensor[0]);

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensor(&oneWire);

enum StatusSensor { DIAM, MENGUKUR };
StatusSensor status = DIAM;

unsigned long lamaKonversi = 750;       // diisi ulang di setup() sesuai RESOLUSI
unsigned long waktuUkurTerakhir = 0;    // kapan perintah ukur terakhir dikirim

// Mencetak 8 byte kode ROM dalam heksadesimal, misalnya 28:61:64:12:3C:7C:2F:27
void cetakKodeRom(const uint8_t rom[8]) {
  for (int i = 0; i < 8; i++) {
    Serial.printf("%02X", rom[i]);
    if (i < 7) {
      Serial.print(":");
    }
  }
}

// Mencetak kode ROM dalam bentuk baris kode yang siap disalin ke daftarSensor[]
void cetakBarisKode(const uint8_t rom[8], int nomor) {
  Serial.print("    { {");
  for (int i = 0; i < 8; i++) {
    Serial.printf("0x%02X", rom[i]);
    if (i < 7) {
      Serial.print(", ");
    }
  }
  Serial.printf("}, \"Sensor %d\" },\n", nomor);
}

// Menampilkan kode ROM semua perangkat yang benar-benar ada di bus
void tampilkanIsiBus() {
  Serial.println("Kode ROM yang ada di bus (salin ke daftarSensor[]):");
  DeviceAddress alamat;
  for (int i = 0; i < sensor.getDeviceCount(); i++) {
    if (sensor.getAddress(alamat, i)) {
      cetakBarisKode(alamat, i + 1);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 25 - Program 4: Membaca Sensor Berdasarkan Alamat");

  sensor.begin();
  sensor.setResolution(RESOLUSI);
  sensor.setWaitForConversion(false);
  lamaKonversi = sensor.millisToWaitForConversion(RESOLUSI);
  Serial.printf("Perangkat 1-Wire di bus: %u\n", sensor.getDeviceCount());

  // Periksa setiap sensor di daftar: apakah kode ROM-nya menjawab?
  bool semuaDitemukan = true;
  for (int i = 0; i < JUMLAH_SENSOR; i++) {
    Serial.printf("%-12s ", daftarSensor[i].nama);
    cetakKodeRom(daftarSensor[i].alamat);
    if (sensor.isConnected(daftarSensor[i].alamat)) {
      Serial.println(" -> terhubung");
    } else {
      Serial.println(" -> TIDAK DITEMUKAN");
      semuaDitemukan = false;
    }
  }
  if (!semuaDitemukan) {
    tampilkanIsiBus();
  }
}

void loop() {
  unsigned long sekarang = millis();

  switch (status) {
    case DIAM:
      if (sekarang - waktuUkurTerakhir >= JEDA_UKUR) {
        waktuUkurTerakhir = sekarang;
        sensor.requestTemperatures();   // semua sensor mulai mengukur bersamaan
        status = MENGUKUR;
      }
      break;

    case MENGUKUR:
      if (sekarang - waktuUkurTerakhir >= lamaKonversi) {
        for (int i = 0; i < JUMLAH_SENSOR; i++) {
          float suhu = sensor.getTempC(daftarSensor[i].alamat);   // baca berdasarkan alamat
          if (suhu == DEVICE_DISCONNECTED_C) {
            Serial.printf("[%6lu ms] %-12s: gagal dibaca\n", sekarang, daftarSensor[i].nama);
          } else {
            Serial.printf("[%6lu ms] %-12s: %.2f °C\n", sekarang, daftarSensor[i].nama, suhu);
          }
        }
        status = DIAM;
      }
      break;
  }
}
