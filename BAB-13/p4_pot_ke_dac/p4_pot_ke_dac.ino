/*
 * BAB 13 - Program 4: Potensiometer ke DAC
 * Tegangan potensiometer dibaca ADC (IO34), diubah menjadi nilai DAC 0–255,
 * lalu dikeluarkan lagi oleh DAC1 (IO25, dibaca balik di IO32)
 * dan DAC2 (IO26, menyalakan LED merah).
 *
 * Rangkaian:
 *   VR10K (potensiometer) → IO34
 *   Kabel jumper pin header 25 → pin header 32
 *   LED1 (merah)          → IO26
 */

#define POT_PIN  34  // potensiometer → IO34 (ADC1)
#define DAC_PIN  25  // DAC1 → IO25
#define BACA_PIN 32  // ADC1 → IO32, membaca balik DAC1
#define DAC_LED  26  // DAC2 → IO26 → LED merah

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 13 - Program 4: Potensiometer ke DAC");
}

void loop() {
  int potMv = analogReadMilliVolts(POT_PIN);          // tegangan potensiometer (mV)
  int nilai = constrain(potMv * 255 / 3300, 0, 255);  // ubah menjadi nilai DAC 0–255

  dacWrite(DAC_PIN, nilai);  // salin tegangan potensiometer ke IO25
  dacWrite(DAC_LED, nilai);  // tegangan yang sama untuk LED merah di IO26
  delay(20);                 // tunggu sebentar sampai tegangannya stabil

  int keluaranMv = analogReadMilliVolts(BACA_PIN);  // tegangan DAC1 yang terbaca

  Serial.print("Potensiometer: ");
  Serial.print(potMv);
  Serial.print(" mV -> nilai DAC ");
  Serial.print(nilai);
  Serial.print(" -> keluaran DAC: ");
  Serial.print(keluaranMv);
  Serial.println(" mV");

  delay(200);
}
