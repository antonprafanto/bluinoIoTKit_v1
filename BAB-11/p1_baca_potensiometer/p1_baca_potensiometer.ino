/*
 * BAB 11 - Program 1: Membaca Potensiometer
 * Membaca potensiometer sebagai angka mentah 0-4095 (analogRead)
 * dan sebagai tegangan terkalibrasi dalam milivolt (analogReadMilliVolts).
 *
 * Rangkaian: VR10K (potensiometer, header kuning pin ke-5) → IO34
 */

#define POT_PIN 34  // potensiometer → IO34 (ADC1_CH6, aman dipakai bersama WiFi)

void setup() {
  Serial.begin(115200);
  // Tidak perlu pinMode(): analogRead() otomatis menyiapkan pin sebagai input analog.
  // Resolusi bawaan 12 bit (0-4095) dan atenuasi bawaan 11 dB sudah cocok untuk 0-3.3 V.

  Serial.println("BAB 11 - Program 1: Membaca Potensiometer");
  Serial.println("Putar kenop potensiometer pelan-pelan dari ujung ke ujung...");
}

void loop() {
  int nilaiAdc = analogRead(POT_PIN);                 // angka mentah 0-4095
  uint32_t milivolt = analogReadMilliVolts(POT_PIN);  // tegangan terkalibrasi (mV)
  float persen = nilaiAdc * 100.0 / 4095.0;           // perkiraan posisi putaran 0-100 %

  Serial.print("ADC: ");
  Serial.print(nilaiAdc);
  Serial.print(" | Tegangan: ");
  Serial.print(milivolt);
  Serial.print(" mV | Posisi: ");
  Serial.print(persen, 1);  // 1 angka di belakang titik desimal
  Serial.println(" %");

  delay(500);  // 2 pembacaan per detik supaya mudah dibaca
}
