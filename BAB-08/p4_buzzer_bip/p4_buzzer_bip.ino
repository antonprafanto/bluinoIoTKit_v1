/*
 * BAB 8 - Program 4: Buzzer Bip-Bip-Bip
 * Buzzer aktif berbunyi pendek 3 kali, lalu diam 2 detik, berulang.
 *
 * Rangkaian: pin BUZZ di kit → IO13
 */

#define BUZZER_PIN 13  // buzzer aktif (pin BUZZ) → IO13

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // pastikan buzzer diam saat mulai

  Serial.println("BAB 8 - Program 4: Buzzer Bip-Bip-Bip");
}

void loop() {
  Serial.println("Bip-bip-bip!");

  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);  // buzzer berbunyi
    delay(100);                      // selama 100 ms
    digitalWrite(BUZZER_PIN, LOW);   // buzzer diam
    delay(100);                      // selama 100 ms
  }

  delay(2000);  // diam 2 detik sebelum mengulang
}
