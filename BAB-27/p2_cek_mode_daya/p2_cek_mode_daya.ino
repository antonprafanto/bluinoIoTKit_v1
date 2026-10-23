/*
 * BAB 27 - Program 2: Memeriksa Mode Daya DS18B20
 * Mencari semua DS18B20 di bus, menanyakan mode daya setiap sensor
 * (perintah Read Power Supply, 0xB4), lalu mengukur suhu setiap 5 detik
 * sambil mencatat lama pengukurannya.
 *
 * Dengan kit saja: 1 sensor, daya eksternal (VDD).
 * Eksperimen opsional: tambahkan DS18B20 kedua dalam mode parasit.
 *
 * Rangkaian: pin DS (header hijau) → IO14
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ONEWIRE 14  // jalur data DS18B20 (pin DS -> IO14)

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensor(&oneWire);

int jumlahSensor = 0;

// Mencetak kode ROM 8 byte, misalnya 28:61:64:12:3C:7C:2F:27
void cetakAlamat(const uint8_t *alamat) {
  for (int i = 0; i < 8; i++) {
    Serial.printf("%02X", alamat[i]);
    if (i < 7) {
      Serial.print(":");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println("BAB 27 - Program 2: Memeriksa Mode Daya DS18B20");

  sensor.begin();  // mencari sensor sekaligus menanyakan mode dayanya
  jumlahSensor = sensor.getDeviceCount();
  Serial.printf("Sensor ditemukan: %d\n", jumlahSensor);

  for (int i = 0; i < jumlahSensor; i++) {
    DeviceAddress alamat;
    if (!sensor.getAddress(alamat, i)) {
      continue;
    }
    bool parasit = sensor.readPowerSupply(alamat);  // true = sensor ini memakai daya parasit
    Serial.printf("  Sensor %d (", i);
    cetakAlamat(alamat);
    Serial.printf("): %s\n", parasit ? "daya PARASIT" : "daya eksternal (VDD)");
  }

  if (sensor.isParasitePowerMode()) {
    Serial.println("Ada sensor parasit: saat mengukur, IO14 dibuat HIGH kuat dan library menunggu waktu penuh.");
  } else {
    Serial.println("Semua sensor memakai VDD: library boleh bertanya 'sudah selesai?' ke sensor.");
  }
}

void loop() {
  if (jumlahSensor == 0) {
    Serial.println("Tidak ada sensor! Periksa kabel DS -> 14, lalu tekan EN.");
    delay(5000);
    return;
  }

  unsigned long mulai = millis();
  sensor.requestTemperatures();  // tunggu sampai semua sensor selesai mengukur
  unsigned long lamaUkur = millis() - mulai;

  Serial.printf("Lama mengukur: %lu ms", lamaUkur);
  for (int i = 0; i < jumlahSensor; i++) {
    float suhu = sensor.getTempCByIndex(i);
    if (suhu == DEVICE_DISCONNECTED_C) {
      Serial.printf(" | Sensor %d: gagal", i);
    } else {
      Serial.printf(" | Sensor %d: %.2f °C", i, suhu);
    }
  }
  Serial.println();

  delay(5000);  // program pemeriksa ini tidak perlu responsif, jadi delay() cukup
}
