/*
 * BAB 30 - Program 4: Mengukur Jarak Tanpa Menunggu
 * Tanpa pulseIn(): pin Echo diperiksa di setiap putaran loop() memakai
 * micros() dan state machine tiga tahap. loop() tidak pernah menunggu
 * pantulan, jadi tugas lain (di sini LED biru berkedip) tetap lancar.
 * Program juga mencatat jeda terlama antarputaran loop() sebagai bukti.
 *
 * Rangkaian:
 *   Tr (header hijau) → IO25   (lepas dulu kabel SW2 → 25)
 *   Ec (header hijau) → pembagi tegangan 1 kΩ + 2 kΩ → IO36 (VP)
 *   LED biru bawaan board sudah tersambung ke IO2
 */

#define PIN_TRIG 25
#define PIN_ECHO 36
#define LED_BIRU 2   // LED biru bawaan board (juga LED3 kit kalau disambung ke IO2)

// Tahap pengukuran (state machine, lihat BAB 17)
enum Tahap { SIAP, TUNGGU_NAIK, TUNGGU_TURUN };

const unsigned long JEDA_UKUR_MS   = 100;    // satu pengukuran setiap 100 ms (minimal 60 ms)
const unsigned long BATAS_NAIK_US  = 10000;  // Echo harus sudah HIGH dalam 10 ms setelah Trig
const unsigned long BATAS_TURUN_US = 30000;  // Echo HIGH lebih dari 30 ms = tidak ada pantulan
const unsigned long KEDIP_MS       = 250;    // LED biru berganti keadaan setiap 250 ms

Tahap tahap = SIAP;
unsigned long waktuTrigMs = 0;  // millis() saat Trig terakhir dikirim
unsigned long waktuTrigUs = 0;  // micros() saat Trig terakhir dikirim
unsigned long waktuNaikUs = 0;  // micros() saat Echo mulai HIGH

unsigned long waktuLedMs = 0;
bool ledNyala = false;

unsigned long awalPutaranLaluUs = 0;  // micros() di awal putaran loop() sebelumnya
unsigned long jedaTerlamaUs = 0;      // jeda terlama antarputaran sejak laporan terakhir

void kirimTrig() {
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);  // satu-satunya "menunggu" di program ini: 10 µs
  digitalWrite(PIN_TRIG, LOW);
}

// Mencetak hasil satu pengukuran; jarakCm < 0 artinya tidak ada pantulan
void laporkan(float jarakCm) {
  if (jarakCm < 0) {
    Serial.printf("Tidak ada pantulan | jeda loop() terlama: %lu us\n", jedaTerlamaUs);
  } else {
    Serial.printf("Jarak: %5.1f cm | jeda loop() terlama: %lu us\n", jarakCm, jedaTerlamaUs);
  }
  jedaTerlamaUs = 0;  // mulai mencatat lagi dari nol
}

void prosesSensor() {
  switch (tahap) {
    case SIAP:
      // Kirim Trig kalau sudah waktunya DAN Echo sedang LOW (modul tidak sibuk)
      if (millis() - waktuTrigMs >= JEDA_UKUR_MS && digitalRead(PIN_ECHO) == LOW) {
        waktuTrigMs = millis();
        kirimTrig();
        waktuTrigUs = micros();
        tahap = TUNGGU_NAIK;
      }
      break;

    case TUNGGU_NAIK:
      if (digitalRead(PIN_ECHO) == HIGH) {
        waktuNaikUs = micros();  // "stopwatch" mulai
        tahap = TUNGGU_TURUN;
      } else if (micros() - waktuTrigUs > BATAS_NAIK_US) {
        Serial.println("Echo tidak pernah HIGH: periksa kabel Ec, pembagi tegangan, dan Tr");
        tahap = SIAP;
      }
      break;

    case TUNGGU_TURUN:
      if (digitalRead(PIN_ECHO) == LOW) {
        unsigned long durasi = micros() - waktuNaikUs;  // "stopwatch" berhenti
        laporkan(durasi * 0.0343 / 2);
        tahap = SIAP;
      } else if (micros() - waktuNaikUs > BATAS_TURUN_US) {
        laporkan(-1);  // pulsa terlalu panjang: tidak ada pantulan
        tahap = SIAP;
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO, INPUT);
  pinMode(LED_BIRU, OUTPUT);
  Serial.println("BAB 30 - Program 4: Mengukur Jarak Tanpa Menunggu");
  awalPutaranLaluUs = micros();
}

void loop() {
  // Catat jeda sejak awal putaran loop() sebelumnya
  unsigned long awalUs = micros();
  unsigned long jeda = awalUs - awalPutaranLaluUs;
  if (jeda > jedaTerlamaUs) {
    jedaTerlamaUs = jeda;
  }
  awalPutaranLaluUs = awalUs;

  // Tugas 1: sensor jarak (tidak pernah menunggu pantulan)
  prosesSensor();

  // Tugas 2: LED biru berkedip setiap 250 ms
  if (millis() - waktuLedMs >= KEDIP_MS) {
    waktuLedMs = millis();
    ledNyala = !ledNyala;
    digitalWrite(LED_BIRU, ledNyala ? HIGH : LOW);
  }
}
