/*
 * BAB 11 - Program 2: Sensor Cahaya LDR
 * Menampilkan nilai ADC, perkiraan kecerahan, dan kategori cahaya.
 * Di kit, LDR berada di sisi GND: makin gelap → tegangan naik → nilai ADC makin BESAR.
 *
 * Rangkaian: LDR (header kuning, pin ke-1) → IO35
 */

#define LDR_PIN 35  // LDR → IO35 (ADC1_CH7, aman dipakai bersama WiFi)

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 11 - Program 2: Sensor Cahaya LDR");
  Serial.println("Tutup LDR dengan jari, lalu sinari dengan senter HP...");
}

void loop() {
  int nilaiAdc = analogRead(LDR_PIN);  // 0 = sangat terang, 4095 = sangat gelap

  // Balik skalanya dengan map(): ADC 0 → kecerahan 100 %, ADC 4095 → kecerahan 0 %
  int kecerahan = map(nilaiAdc, 0, 4095, 100, 0);

  // Kategori cahaya. Ingat: makin gelap, makin BESAR nilai ADC.
  // Ambang (500, 1500, ...) boleh kamu sesuaikan dengan ruanganmu.
  const char *kategori;
  if (nilaiAdc < 500) {
    kategori = "Sangat terang";
  } else if (nilaiAdc < 1500) {
    kategori = "Terang";
  } else if (nilaiAdc < 2500) {
    kategori = "Redup";
  } else if (nilaiAdc < 3500) {
    kategori = "Gelap";
  } else {
    kategori = "Sangat gelap";
  }

  Serial.print("ADC: ");
  Serial.print(nilaiAdc);
  Serial.print(" | Kecerahan: ");
  Serial.print(kecerahan);
  Serial.print(" % | Kategori: ");
  Serial.println(kategori);

  delay(500);
}
