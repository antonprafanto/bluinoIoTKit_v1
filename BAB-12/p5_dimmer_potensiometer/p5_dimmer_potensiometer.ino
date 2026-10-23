/*
 * BAB 12 - Program 5: Dimmer dengan Potensiometer
 * Putar potensiometer untuk mengatur kecerahan LED merah.
 *
 * Rangkaian:
 *   VR10K (potensiometer) → IO34
 *   LED1 (merah)          → IO4
 */

#define POT_PIN   34  // potensiometer → IO34 (ADC1, pin khusus input)
#define LED_MERAH 4   // LED merah → IO4

const uint32_t FREKUENSI = 5000;  // 5 kHz
const uint8_t RESOLUSI = 12;      // 12 bit → nilai duty 0–4095, sama dengan rentang ADC
                                  // cek: 5000 × 4096 = 20480000, masih di bawah 80000000

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12 - Program 5: Dimmer dengan Potensiometer");

  if (!ledcAttach(LED_MERAH, FREKUENSI, RESOLUSI)) {
    Serial.println("Gagal mengatur PWM di IO4! Periksa frekuensi dan resolusi.");
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  int nilaiPot = analogRead(POT_PIN);  // 0–4095 (ADC 12 bit)
  ledcWrite(LED_MERAH, nilaiPot);      // resolusi PWM juga 12 bit → bisa langsung dipakai

  float persen = nilaiPot * 100.0 / 4095;

  Serial.print("Potensiometer: ");
  Serial.print(nilaiPot);
  Serial.print(" | duty cycle: ");
  Serial.print(persen, 0);  // 0 angka di belakang koma → dibulatkan
  Serial.println("%");

  delay(100);  // baca 10 kali per detik
}
