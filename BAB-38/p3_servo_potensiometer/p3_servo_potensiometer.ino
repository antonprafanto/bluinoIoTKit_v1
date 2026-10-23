/*
 * BAB 38 - Program 3: Servo Mengikuti Potensiometer
 * Putar potensiometer kit → servo ikut berputar (0–180°).
 * Nilai ADC dihaluskan dengan EMA, dan perubahan kecil diabaikan
 * supaya servo tidak bergetar.
 *
 * Rangkaian:
 *   SRV (header hijau, paling bawah) → IO32, servo di header servo kit
 *   VR10K (header kuning, pin ke-5)   → IO34
 */

#include <ESP32Servo.h>

#define PIN_SERVO 32  // SRV → IO32
#define PIN_POT 34    // VR10K → IO34 (ADC1)

const int PULSA_MIN_US = 500;   // ← ganti dengan hasil kalibrasi Program 2
const int PULSA_MAX_US = 2400;  // ← ganti dengan hasil kalibrasi Program 2

const unsigned long INTERVAL_BACA_MS = 20;    // baca potensiometer 50 kali per detik
const unsigned long INTERVAL_LAPOR_MS = 500;  // laporan ke Serial Monitor 2 kali per detik
const float ALFA = 0.2;                       // bobot EMA: kecil = halus tetapi lambat
const int AMBANG_DERAJAT = 2;                 // abaikan perubahan kurang dari 2°

Servo servo;

int adcMentah = 0;    // bacaan ADC terakhir (belum dihaluskan)
float adcHalus = 0;   // nilai ADC setelah dihaluskan EMA
int sudutServo = 0;   // sudut terakhir yang dikirim ke servo
unsigned long waktuBacaLalu = 0;
unsigned long waktuLaporLalu = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 38 - Program 3: Servo Mengikuti Potensiometer");

  analogReadResolution(12);  // nilai ADC 0–4095

  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, PULSA_MIN_US, PULSA_MAX_US);
  if (!servo.attached()) {
    Serial.println("Gagal memasang servo di IO32! Periksa nomor pin.");
    while (true) {
      delay(1000);  // program berhenti di sini
    }
  }

  // Isi EMA dengan bacaan pertama supaya servo langsung ke posisi kenop
  adcMentah = analogRead(PIN_POT);
  adcHalus = adcMentah;
  sudutServo = map(adcMentah, 0, 4095, 0, 180);
  servo.write(sudutServo);
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: baca potensiometer dan gerakkan servo
  if (sekarang - waktuBacaLalu >= INTERVAL_BACA_MS) {
    waktuBacaLalu = sekarang;

    adcMentah = analogRead(PIN_POT);
    adcHalus = ALFA * adcMentah + (1 - ALFA) * adcHalus;  // EMA
    int sudutBaru = map((int)(adcHalus + 0.5), 0, 4095, 0, 180);

    // Gerakkan hanya kalau perubahannya cukup besar,
    // atau kalau kenop sudah di ujung (supaya 0° dan 180° tetap tercapai)
    bool cukupJauh = abs(sudutBaru - sudutServo) >= AMBANG_DERAJAT;
    bool diUjung = (sudutBaru == 0 || sudutBaru == 180);
    if (sudutBaru != sudutServo && (cukupJauh || diUjung)) {
      sudutServo = sudutBaru;
      servo.write(sudutServo);
    }
  }

  // Tugas 2: laporan ke Serial Monitor
  if (sekarang - waktuLaporLalu >= INTERVAL_LAPOR_MS) {
    waktuLaporLalu = sekarang;
    Serial.printf("ADC mentah: %4d | ADC halus: %6.1f | sudut servo: %3d derajat\n",
                  adcMentah, adcHalus, sudutServo);
  }
}
