/*
 * BAB 11 - Program 4: Potensiometer Mengatur Kecerahan LED
 * Putar potensiometer untuk mengatur kecerahan LED merah (ADC → PWM).
 *
 * Rangkaian:
 *   VR10K (potensiometer, header kuning pin ke-5) → IO34
 *   LED1 (merah, header LED)                       → IO4
 */

#define POT_PIN   34  // potensiometer → IO34 (ADC1)
#define LED_MERAH 4   // LED merah → IO4 (output PWM)

const int FREKUENSI_PWM = 5000;  // Hz: LED dinyalakan-dimatikan 5000 kali per detik
const int RESOLUSI_PWM = 8;      // 8 bit → nilai kecerahan 0-255
const int JUMLAH_SAMPEL = 16;    // rata-rata 16 kali baca supaya kecerahan LED stabil

// Baca pin beberapa kali, lalu kembalikan rata-ratanya
int bacaRataRata(int pin, int jumlahSampel) {
  long total = 0;
  for (int i = 0; i < jumlahSampel; i++) {
    total += analogRead(pin);
  }
  return total / jumlahSampel;
}

void setup() {
  Serial.begin(115200);

  // Siapkan PWM di IO4: frekuensi 5000 Hz, resolusi 8 bit (dibahas lengkap di BAB 12)
  ledcAttach(LED_MERAH, FREKUENSI_PWM, RESOLUSI_PWM);

  Serial.println("BAB 11 - Program 4: Potensiometer Mengatur Kecerahan LED");
}

void loop() {
  int nilaiPot = bacaRataRata(POT_PIN, JUMLAH_SAMPEL);  // 0-4095
  int kecerahan = map(nilaiPot, 0, 4095, 0, 255);       // ubah ke rentang PWM 0-255

  ledcWrite(LED_MERAH, kecerahan);  // atur kecerahan LED

  Serial.print("Potensiometer: ");
  Serial.print(nilaiPot);
  Serial.print(" -> Kecerahan LED: ");
  Serial.println(kecerahan);

  delay(100);
}
