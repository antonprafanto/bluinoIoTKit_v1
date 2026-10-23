/*
 * BAB 29 - Program 5: Sudut dari Giroskop dan Drift
 * Mengintegrasikan giroskop setiap 10 ms (dt diukur dengan micros())
 * untuk mendapatkan sudut roll dan yaw, lalu menggambarnya di Serial Plotter
 * setiap 200 ms bersama roll dari akselerometer sebagai pembanding.
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
const bool PAKAI_KALIBRASI = true;       // ubah ke false untuk melihat drift tanpa kalibrasi
const unsigned long PERIODE_US = 10000;  // hitung sudut setiap 10000 µs (10 ms)
const unsigned long JEDA_CETAK = 200;    // satu titik grafik setiap 200 ms
const int SAMPEL_KALIBRASI = 200;        // 200 bacaan x 10 ms = 2 detik

unsigned long waktuSampelLalu = 0;     // micros() saat sudut terakhir dihitung
unsigned long waktuCetakTerakhir = 0;  // millis() saat titik grafik terakhir dikirim

Adafruit_MPU6050 mpu;

float ax, ay, az;                                // percepatan (g)
float gx, gy, gz;                                // kecepatan putar mentah (°/s)
float offsetGx = 0, offsetGy = 0, offsetGz = 0;  // hasil kalibrasi (°/s)

float rollGiro = 0;  // sudut roll hasil integrasi giroskop (°)
float yawGiro = 0;   // sudut yaw hasil integrasi giroskop (°)

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

  if (PAKAI_KALIBRASI) {
    kalibrasiGiro();
  }

  // Giroskop hanya tahu perubahan: sudut awal roll diambil dari akselerometer,
  // sedangkan yaw awal dianggap 0
  if (bacaImu()) {
    rollGiro = atan2(ay, az) * RAD_TO_DEG;
  }
  waktuSampelLalu = micros();
}

void loop() {
  // 1. Setiap 10 ms: baca giroskop lalu tambahkan ke sudut (integrasi)
  unsigned long sekarang = micros();
  if (sekarang - waktuSampelLalu >= PERIODE_US) {
    float dt = (sekarang - waktuSampelLalu) / 1000000.0;  // selang waktu nyata (detik)
    waktuSampelLalu = sekarang;

    if (bacaImu()) {
      rollGiro += (gx - offsetGx) * dt;  // °/s × detik = derajat
      yawGiro  += (gz - offsetGz) * dt;
    }
  }

  // 2. Setiap 200 ms: kirim satu titik grafik
  if (millis() - waktuCetakTerakhir >= JEDA_CETAK) {
    waktuCetakTerakhir = millis();
    float rollAksel = atan2(ay, az) * RAD_TO_DEG;  // pembanding dari akselerometer
    Serial.printf("RollAksel:%.1f,RollGiro:%.1f,YawGiro:%.1f\n", rollAksel, rollGiro, yawGiro);
  }
}
