/*
 * BAB 8 - Program 1: LED Merah Berkedip
 * LED merah menyala 1 detik, mati 1 detik, berulang terus.
 *
 * Rangkaian: LED1 (merah) di header LED → IO4
 */

#define LED_MERAH 4  // LED merah → IO4

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);    // IO4 menjadi output
  digitalWrite(LED_MERAH, LOW);  // mulai dalam keadaan mati

  Serial.println("BAB 8 - Program 1: LED Merah Berkedip");
}

void loop() {
  digitalWrite(LED_MERAH, HIGH);  // IO4 = HIGH → LED menyala (active-high)
  Serial.println("LED merah: NYALA");
  delay(1000);                    // tunggu 1 detik

  digitalWrite(LED_MERAH, LOW);   // IO4 = LOW → LED mati
  Serial.println("LED merah: MATI");
  delay(1000);                    // tunggu 1 detik
}
