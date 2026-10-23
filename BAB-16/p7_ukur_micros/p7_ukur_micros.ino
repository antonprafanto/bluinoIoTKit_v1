/*
 * BAB 16 - Program 7: Mengukur Waktu dengan micros()
 * Setiap 1 detik menampilkan:
 *   - berapa kali loop() berputar dalam 1 detik terakhir
 *   - lama analogRead() dalam mikrodetik (us)
 *   - lama delayMicroseconds(100) sebagai pembanding (seharusnya ±100 us)
 *
 * Rangkaian: Potensiometer VR10K (pin kuning ke-5) → IO34
 */

#define POT_PIN 34

const unsigned long INTERVAL_LAPORAN = 1000;  // ms
unsigned long waktuLaporanLalu = 0;
unsigned long jumlahPutaran = 0;  // putaran loop() sejak laporan terakhir

void setup() {
  Serial.begin(115200);
  analogRead(POT_PIN);  // pembacaan pertama sekaligus menyiapkan ADC

  Serial.println("BAB 16 - Program 7: Mengukur Waktu dengan micros()");
}

void loop() {
  jumlahPutaran++;  // hitung setiap putaran loop()

  unsigned long sekarang = millis();
  if (sekarang - waktuLaporanLalu >= INTERVAL_LAPORAN) {
    waktuLaporanLalu = sekarang;

    // Ukur lama analogRead()
    unsigned long mulai = micros();
    int nilai = analogRead(POT_PIN);
    unsigned long lamaAdc = micros() - mulai;

    // Ukur lama delayMicroseconds(100) sebagai pembanding
    mulai = micros();
    delayMicroseconds(100);
    unsigned long lamaTunda = micros() - mulai;

    Serial.print("loop() per detik: ");
    Serial.print(jumlahPutaran);
    Serial.print(" | analogRead(): ");
    Serial.print(lamaAdc);
    Serial.print(" us (nilai ");
    Serial.print(nilai);
    Serial.print(") | delayMicroseconds(100): ");
    Serial.print(lamaTunda);
    Serial.println(" us");

    jumlahPutaran = 0;  // mulai menghitung lagi untuk detik berikutnya
  }
}
