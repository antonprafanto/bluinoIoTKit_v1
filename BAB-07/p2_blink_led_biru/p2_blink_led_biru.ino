/*
 * BAB 7 - Program 2: LED Biru Berkedip (Blink)
 * Menyalakan dan memadamkan LED biru bawaan board (IO2) setiap 1 detik,
 * sambil menampilkan statusnya di Serial Monitor.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino
 * Pin      : IO2 = LED biru bawaan board
 *            (opsional: LED biru kit, pin LED3 di header LED → IO2)
 * Alat uji : Serial Monitor, baud 115200
 */

// ---------- Pengaturan pin ----------
#define LED_BIRU 2  // LED biru terhubung ke IO2

// ---------- Pengaturan waktu ----------
const int JEDA_KEDIP = 1000;  // lama LED menyala/padam dalam milidetik (1000 ms = 1 detik)

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);  // jadikan IO2 sebagai OUTPUT (pin pengirim sinyal)
  Serial.println("Program Blink dimulai!");
}

void loop() {
  digitalWrite(LED_BIRU, HIGH);  // HIGH = pin bertegangan 3.3 V → LED menyala
  Serial.println("LED: NYALA");
  delay(JEDA_KEDIP);             // tunggu selama JEDA_KEDIP milidetik

  digitalWrite(LED_BIRU, LOW);   // LOW = pin 0 V → LED padam
  Serial.println("LED: PADAM");
  delay(JEDA_KEDIP);
}
