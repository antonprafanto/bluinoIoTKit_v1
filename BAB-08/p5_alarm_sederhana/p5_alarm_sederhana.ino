/*
 * BAB 8 - Program 5: Alarm Sederhana (LED + Buzzer)
 * LED merah berkedip bersamaan dengan bunyi buzzer 5 kali, lalu jeda 3 detik.
 *
 * Rangkaian:
 *   LED1 (merah) → IO4
 *   BUZZ         → IO13
 */

#define LED_MERAH  4
#define BUZZER_PIN 13

const int JUMLAH_KEDIP = 5;  // berapa kali alarm berbunyi dalam satu siklus

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("BAB 8 - Program 5: Alarm Sederhana");
}

void loop() {
  Serial.println("ALARM! (5x kedip + bunyi)");

  for (int i = 0; i < JUMLAH_KEDIP; i++) {
    // Nyala + bunyi bersamaan
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);

    // Mati + diam bersamaan
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }

  Serial.println("--- Jeda 3 detik ---");
  delay(3000);
}
