/*
 * BAB 16 - Program 1: Tombol yang Terlambat Merespons
 * LED merah berkedip dengan delay(). LED hijau seharusnya mengikuti tombol S1,
 * tetapi tombol hanya sempat dibaca sekali setiap 2 detik.
 *
 * Rangkaian:
 *   LED1 (merah)                → IO4
 *   LED2 (hijau)                → IO17
 *   Tombol S1 (pin kuning ke-3) → IO15
 */

#define LED_MERAH 4
#define LED_HIJAU 17
#define TOMBOL_S1 15

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(TOMBOL_S1, INPUT);  // tombol kit: pull-down 10 kΩ sudah ada di PCB

  Serial.println("BAB 16 - Program 1: Tombol yang Terlambat Merespons");
}

void loop() {
  // Tugas 1: LED merah berkedip (cara lama, memakai delay)
  digitalWrite(LED_MERAH, HIGH);
  delay(1000);                   // program tertahan 1 detik di sini...
  digitalWrite(LED_MERAH, LOW);
  delay(1000);                   // ...lalu 1 detik lagi di sini

  // Tugas 2: LED hijau mengikuti S1 — baru sempat dijalankan setiap 2 detik
  int s1 = digitalRead(TOMBOL_S1);
  digitalWrite(LED_HIJAU, s1);   // HIGH (ditekan) → LED hijau menyala
  Serial.print("Tombol S1 dibaca: ");
  Serial.println(s1 == HIGH ? "DITEKAN" : "dilepas");
}
