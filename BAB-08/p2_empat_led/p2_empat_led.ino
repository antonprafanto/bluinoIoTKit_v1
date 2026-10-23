/*
 * BAB 8 - Program 2: Empat LED Menyala Bergantian
 * LED menyala satu per satu dari kiri ke kanan, lalu mati satu per satu.
 *
 * Rangkaian (header LED di kit → pin ESP32):
 *   LED1 (merah)  → IO4
 *   LED2 (hijau)  → IO17
 *   LED3 (biru)   → IO2
 *   LED4 (kuning) → IO16
 */

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2

// Urutan array = urutan posisi LED di kit, dari kiri ke kanan
const int LED_PINS[] = {LED_MERAH, LED_HIJAU, LED_BIRU, LED_KUNING};
const char *NAMA_LED[] = {"merah", "hijau", "biru", "kuning"};
const int JUMLAH_LED = sizeof(LED_PINS) / sizeof(LED_PINS[0]);  // = 4

void setup() {
  Serial.begin(115200);

  // Jadikan semua pin LED output dan pastikan semuanya mati
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  Serial.println("BAB 8 - Program 2: Empat LED Menyala Bergantian");
}

void loop() {
  // Nyalakan satu per satu
  for (int i = 0; i < JUMLAH_LED; i++) {
    digitalWrite(LED_PINS[i], HIGH);
    Serial.print("LED ");
    Serial.print(NAMA_LED[i]);
    Serial.println(": NYALA");
    delay(500);
  }
  delay(1000);  // semua menyala selama 1 detik

  // Matikan satu per satu
  for (int i = 0; i < JUMLAH_LED; i++) {
    digitalWrite(LED_PINS[i], LOW);
    Serial.print("LED ");
    Serial.print(NAMA_LED[i]);
    Serial.println(": MATI");
    delay(500);
  }
  delay(1000);  // semua mati selama 1 detik
}
