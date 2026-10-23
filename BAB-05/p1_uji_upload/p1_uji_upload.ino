// BAB 5 — Program 1: Uji Upload
// LED biru bawaan board ESP32 DEVKIT V1 berkedip setiap 1 detik,
// dan setiap perubahan dilaporkan ke Serial Monitor.

#define LED_BAWAAN 2  // LED biru bawaan board terhubung ke IO2

void setup() {
  Serial.begin(115200);         // kecepatan komunikasi dengan Serial Monitor
  pinMode(LED_BAWAAN, OUTPUT);  // jadikan IO2 sebagai output
  delay(1000);                  // beri waktu Serial Monitor tersambung
  Serial.println("Halo! ESP32 siap dipakai.");
}

void loop() {
  digitalWrite(LED_BAWAAN, HIGH);  // LED menyala
  Serial.println("LED menyala");
  delay(1000);                     // tunggu 1 detik

  digitalWrite(LED_BAWAAN, LOW);   // LED mati
  Serial.println("LED mati");
  delay(1000);                     // tunggu 1 detik
}
