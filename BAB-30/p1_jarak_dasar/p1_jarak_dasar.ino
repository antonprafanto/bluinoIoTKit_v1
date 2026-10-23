/*
 * BAB 30 - Program 1: Mengukur Jarak Dasar
 * Mengirim pulsa Trig 10 µs, mengukur lebar pulsa Echo dengan pulseIn(),
 * lalu menghitung jarak dengan kecepatan suara pada suhu 20 °C.
 *
 * Rangkaian:
 *   Modul HC-SR04 di konektor kit (G, Ec, Tr, V)
 *   Tr (header hijau) → IO25   (lepas dulu kabel SW2 → 25)
 *   Ec (header hijau) → pembagi tegangan 1 kΩ + 2 kΩ → IO36 (VP)
 */

#define PIN_TRIG 25  // Tr → IO25
#define PIN_ECHO 36  // Ec → pembagi tegangan → IO36 (label VP)

const unsigned long BATAS_TUNGGU_US = 30000;  // 30 ms, cukup untuk pantulan terjauh (±4 m ≈ 23 ms)
const float KECEPATAN_SUARA = 0.0343;         // cm per mikrodetik (343 m/s, suhu 20 °C)

// Memicu satu pengukuran, lalu mengembalikan lebar pulsa Echo dalam µs.
// Hasil 0 berarti pulsa Echo tidak selesai sebelum batas waktu habis.
unsigned long ukurEchoUs() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);        // pulsa Trig minimal 10 µs
  digitalWrite(PIN_TRIG, LOW);
  return pulseIn(PIN_ECHO, HIGH, BATAS_TUNGGU_US);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO, INPUT);     // IO36 hanya bisa menjadi input
  Serial.println("BAB 30 - Program 1: Mengukur Jarak Dasar");
}

void loop() {
  unsigned long durasi = ukurEchoUs();

  if (durasi == 0) {
    Serial.println("Tidak ada pantulan (benda terlalu jauh/miring, atau cek kabel)");
  } else {
    float jarakCm = durasi * KECEPATAN_SUARA / 2;
    Serial.printf("Echo: %5lu us | Jarak: %5.1f cm\n", durasi, jarakCm);
  }

  delay(200);  // ukur 5 kali per detik (jeda minimal 60 ms)
}
