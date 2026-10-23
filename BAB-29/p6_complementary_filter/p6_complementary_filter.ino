/*
 * BAB 29 - Program 6: Menggabungkan Akselerometer dan Giroskop
 * Complementary filter: sudut roll dihitung 100 kali per detik dari
 * 98% prediksi giroskop + 2% sudut akselerometer.
 * Serial Plotter menampilkan tiga garis setiap 50 ms:
 *   RollAksel  = dari akselerometer saja
 *   RollGiro   = dari giroskop saja (integrasi)
 *   RollFilter = hasil complementary filter
 *
 * Rangkaian: tidak perlu kabel. MPU-6050 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * LED biru bawaan board ESP32 (IO2) menyala selama kalibrasi.
 * Library: Adafruit MPU6050 (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define PIN_SDA   21
#define PIN_SCL   22
#define LED_BOARD 2  // LED biru bawaan board ESP32

const uint8_t MPU_ALAMAT = 0x68;
const float ALPHA = 0.98;                // bobot giroskop (0..1)
const unsigned long PERIODE_US = 10000;  // hitung sudut setiap 10000 µs (10 ms)
const unsigned long JEDA_CETAK = 50;     // satu titik grafik setiap 50 ms
const int SAMPEL_KALIBRASI = 200;        // 200 bacaan x 10 ms = 2 detik

unsigned long waktuSampelLalu = 0;     // micros() saat sudut terakhir dihitung
unsigned long waktuCetakTerakhir = 0;  // millis() saat titik grafik terakhir dikirim

Adafruit_MPU6050 mpu;

float ax, ay, az;                                // percepatan (g)
float gx, gy, gz;                                // kecepatan putar mentah (°/s)
float offsetGx = 0, offsetGy = 0, offsetGz = 0;  // hasil kalibrasi (°/s)

float rollAksel = 0;   // roll dari akselerometer saja (°)
float rollGiro = 0;    // roll dari giroskop saja (°)
float rollFilter = 0;  // roll hasil complementary filter (°)

// ---------- Fungsi sensor (versi library Adafruit, sama seperti Program 3) ----------

bool siapkanImu() {
  if (!mpu.begin(MPU_ALAMAT)) {
    return false;
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);  // ±2 g
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);       // ±500 °/s
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);    // DLPF 21 Hz
  return true;
}

bool bacaImu() {
  Wire.beginTransmission(MPU_ALAMAT);  // pastikan dulu sensornya menjawab
  if (Wire.endTransmission() != 0) {
    return false;
  }
  sensors_event_t aksel, giro, suhu;
  mpu.getEvent(&aksel, &giro, &suhu);
  ax = aksel.acceleration.x / SENSORS_GRAVITY_STANDARD;  // m/s² → g
  ay = aksel.acceleration.y / SENSORS_GRAVITY_STANDARD;
  az = aksel.acceleration.z / SENSORS_GRAVITY_STANDARD;
  gx = giro.gyro.x * RAD_TO_DEG;                          // rad/s → °/s
  gy = giro.gyro.y * RAD_TO_DEG;
  gz = giro.gyro.z * RAD_TO_DEG;
  return true;
}

// Rata-rata bacaan giroskop saat kit diam = offset (sama seperti Program 3)
void kalibrasiGiro() {
  digitalWrite(LED_BOARD, HIGH);  // LED menyala: jangan sentuh kit
  float totalX = 0, totalY = 0, totalZ = 0;
  int jumlah = 0;
  for (int i = 0; i < SAMPEL_KALIBRASI; i++) {
    if (bacaImu()) {
      totalX += gx;
      totalY += gy;
      totalZ += gz;
      jumlah++;
    }
    delay(10);
  }
  if (jumlah > 0) {
    offsetGx = totalX / jumlah;
    offsetGy = totalY / jumlah;
    offsetGz = totalZ / jumlah;
  }
  digitalWrite(LED_BOARD, LOW);   // selesai
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BOARD, OUTPUT);
  Wire.begin(PIN_SDA, PIN_SCL);  // bus I2C kit: SDA = IO21, SCL = IO22

  if (!siapkanImu()) {
    Serial.println("MPU-6050 tidak ditemukan. Jalankan Program 1 untuk mencari penyebabnya.");
    bool nyala = false;
    while (true) {               // LED berkedip cepat = sensor tidak ditemukan
      nyala = !nyala;
      digitalWrite(LED_BOARD, nyala ? HIGH : LOW);
      delay(100);
    }
  }

  kalibrasiGiro();

  // Ketiga sudut dimulai dari sudut akselerometer (kit sedang diam)
  if (bacaImu()) {
    rollAksel = atan2(ay, az) * RAD_TO_DEG;
  }
  rollGiro = rollAksel;
  rollFilter = rollAksel;
  waktuSampelLalu = micros();
}

void loop() {
  // 1. Setiap 10 ms: baca sensor lalu perbarui ketiga sudut
  unsigned long sekarang = micros();
  if (sekarang - waktuSampelLalu >= PERIODE_US) {
    float dt = (sekarang - waktuSampelLalu) / 1000000.0;  // selang waktu nyata (detik)
    waktuSampelLalu = sekarang;

    if (bacaImu()) {
      float lajuRoll = gx - offsetGx;           // kecepatan putar roll (°/s)
      rollAksel = atan2(ay, az) * RAD_TO_DEG;  // akselerometer saja
      rollGiro += lajuRoll * dt;               // giroskop saja

      // Complementary filter: 98% prediksi giroskop + 2% akselerometer
      rollFilter = ALPHA * (rollFilter + lajuRoll * dt) + (1.0 - ALPHA) * rollAksel;
    }
  }

  // 2. Setiap 50 ms: kirim satu titik grafik
  if (millis() - waktuCetakTerakhir >= JEDA_CETAK) {
    waktuCetakTerakhir = millis();
    Serial.printf("RollAksel:%.1f,RollGiro:%.1f,RollFilter:%.1f\n", rollAksel, rollGiro, rollFilter);
  }
}
