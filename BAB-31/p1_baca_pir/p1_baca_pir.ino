/*
 * BAB 31 - Program 1: Membaca Sensor PIR
 * Membaca keadaan sensor PIR AM312 dan menampilkannya di Serial Monitor
 * setiap 0,5 detik. LED biru bawaan board (IO2) mengikuti keluaran PIR.
 *
 * Rangkaian: PIR (header hijau, pin ke-5 dari atas) → IO33
 */

#define PIN_PIR  33  // keluaran sensor PIR AM312
#define LED_BIRU 2   // LED biru bawaan board ESP32 (tanpa kabel)

const unsigned long INTERVAL_CETAK = 500;  // ms: cetak status setiap 0,5 detik
unsigned long waktuCetakLalu = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);  // INPUT biasa: AM312 sendiri yang mendorong HIGH/LOW
  pinMode(LED_BIRU, OUTPUT);

  Serial.println("BAB 31 - Program 1: Membaca Sensor PIR");
  Serial.println("Kit baru dicolokkan? Diam dulu sekitar 1 menit supaya PIR stabil.");
}

void loop() {
  int statusPir = digitalRead(PIN_PIR);  // HIGH = ada gerakan
  digitalWrite(LED_BIRU, statusPir);     // LED langsung mengikuti sensor

  // Cetak status setiap 0,5 detik (pola millis() dari BAB 16)
  unsigned long sekarang = millis();
  if (sekarang - waktuCetakLalu >= INTERVAL_CETAK) {
    waktuCetakLalu = sekarang;

    Serial.print("Detik ke-");
    Serial.print(sekarang / 1000.0, 1);  // waktu sejak ESP32 mulai, 1 angka desimal
    if (statusPir == HIGH) {
      Serial.println(" | PIR = 1 | ADA GERAKAN");
    } else {
      Serial.println(" | PIR = 0 | tidak ada gerakan");
    }
  }
}
