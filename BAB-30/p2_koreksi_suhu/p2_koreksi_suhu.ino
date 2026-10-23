/*
 * BAB 30 - Program 2: Jarak dengan Koreksi Suhu
 * Suhu udara dari DHT11 dipakai untuk menghitung kecepatan suara,
 * lalu jarak terkoreksi dibandingkan dengan jarak tanpa koreksi (20 °C).
 *
 * Rangkaian:
 *   Tr  (header hijau) → IO25   (lepas dulu kabel SW2 → 25)
 *   Ec  (header hijau) → pembagi tegangan 1 kΩ + 2 kΩ → IO36 (VP)
 *   DHT (header hijau) → IO27
 *
 * Library: DHT sensor library by Adafruit (+ Adafruit Unified Sensor)
 */

#include <DHT.h>

#define PIN_TRIG 25
#define PIN_ECHO 36
#define DHT_PIN  27
#define DHT_TYPE DHT11

const unsigned long BATAS_TUNGGU_US = 30000;  // 30 ms

DHT dht(DHT_PIN, DHT_TYPE);

// Timer serbaguna dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerSuhu  = {2000, 0};  // baca DHT11 setiap 2 detik (jangan kurang dari 2000)
Timer timerJarak = {500, 0};   // ukur jarak setiap 500 ms

float suhuUdara = 20.0;        // °C; dipakai 20 °C sampai DHT11 berhasil dibaca
bool suhuDariSensor = false;   // true setelah DHT11 pernah berhasil dibaca

// Kecepatan suara (m/s) pada suhu tertentu (°C), rumus pendekatan
float kecepatanSuara(float suhuC) {
  return 331.3 + 0.606 * suhuC;
}

unsigned long ukurEchoUs() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  return pulseIn(PIN_ECHO, HIGH, BATAS_TUNGGU_US);
}

void bacaSuhu() {
  dht.read(true);                   // paksa membaca sensor sekarang (±25 ms)
  float t = dht.readTemperature();  // ambil suhu dari bacaan barusan
  if (isnan(t)) {
    Serial.println("Gagal membaca DHT11, suhu sebelumnya tetap dipakai");
    return;
  }
  suhuUdara = t;
  suhuDariSensor = true;
}

void ukurJarak() {
  unsigned long durasi = ukurEchoUs();
  if (durasi == 0) {
    Serial.println("Tidak ada pantulan");
    return;
  }

  // m/s → cm/µs: kali 100 (m → cm), bagi 1.000.000 (s → µs) = bagi 10.000
  float v = kecepatanSuara(suhuUdara);
  float jarakKoreksi = durasi * (v / 10000.0) / 2;
  float jarak20C = durasi * (kecepatanSuara(20.0) / 10000.0) / 2;

  Serial.printf("Suhu %.1f °C%s | v = %.1f m/s | Jarak %.1f cm (tanpa koreksi %.1f cm)\n",
                suhuUdara, suhuDariSensor ? "" : " (anggapan)", v, jarakKoreksi, jarak20C);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO, INPUT);
  dht.begin();
  Serial.println("BAB 30 - Program 2: Jarak dengan Koreksi Suhu");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: perbarui suhu udara setiap 2 detik
  if (timerSuhu.sudahWaktunya(sekarang)) {
    bacaSuhu();
  }

  // Tugas 2: ukur jarak setiap 500 ms
  if (timerJarak.sudahWaktunya(sekarang)) {
    ukurJarak();
  }
}
