/*
 * BAB 9 - Program 1: Membaca Tombol S1
 * LED merah menyala selama tombol S1 ditekan.
 * Keadaan tombol ditampilkan di Serial Monitor 5 kali per detik.
 *
 * Rangkaian:
 *   Tombol S1 (pin kuning ke-3) → IO15
 *   LED1 (merah)                → IO4
 */

#define TOMBOL_S1 15  // tombol S1 → IO15 (pull-down 10 kΩ sudah ada di kit)
#define LED_MERAH 4   // LED merah → IO4

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);   // INPUT biasa, bukan INPUT_PULLUP
  pinMode(LED_MERAH, OUTPUT);

  Serial.println("BAB 9 - Program 1: Membaca Tombol S1");
  Serial.println("Tekan dan tahan tombol S1...");
}

void loop() {
  int status = digitalRead(TOMBOL_S1);  // HIGH = ditekan, LOW = dilepas

  if (status == HIGH) {
    digitalWrite(LED_MERAH, HIGH);      // tombol ditekan → LED menyala
    Serial.println("S1: HIGH (ditekan) -> LED menyala");
  } else {
    digitalWrite(LED_MERAH, LOW);       // tombol dilepas → LED mati
    Serial.println("S1: LOW (dilepas) -> LED mati");
  }

  delay(200);  // baca 5 kali per detik supaya Serial Monitor mudah dibaca
}
