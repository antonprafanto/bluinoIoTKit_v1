/*
 * BAB 21 - Program 5: Grafik Potensiometer dan LDR di Serial Plotter
 * Mengirim nilai potensiometer dan LDR 20 kali per detik dalam format
 * "label:nilai" yang dimengerti Serial Plotter Arduino IDE 2.
 *
 * Rangkaian: VR10K (potensiometer) → IO34, LDR → IO35
 * Alat uji : Tools → Serial Plotter, baud 115200
 */

#define PIN_POT 34  // VR10K (potensiometer) → IO34
#define PIN_LDR 35  // LDR → IO35 (makin gelap, nilainya makin besar)

const unsigned long INTERVAL_KIRIM = 50;  // ms: 20 data per detik
unsigned long waktuKirim = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  if (millis() - waktuKirim >= INTERVAL_KIRIM) {
    waktuKirim = millis();

    int nilaiPot = analogRead(PIN_POT);  // 0 sampai 4095
    int nilaiLdr = analogRead(PIN_LDR);  // 0 sampai 4095

    // Satu baris = satu titik: label:nilai dipisahkan koma, diakhiri println()
    Serial.print("Min:0,Maks:4095,Potensiometer:");
    Serial.print(nilaiPot);
    Serial.print(",LDR:");
    Serial.println(nilaiLdr);
  }
}
