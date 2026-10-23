/*
 * BAB 29 - Program 4: Roll dan Pitch dari Akselerometer
 * Menghitung sudut roll dan pitch dari arah gravitasi setiap 100 ms,
 * lalu mengirimnya ke Serial Plotter (bisa juga dibaca di Serial Monitor).
 *
 * Rangkaian: tidak perlu kabel. MPU-6050 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: Adafruit MPU6050 (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define PIN_SDA 21
#define PIN_SCL 22

const uint8_t MPU_ALAMAT = 0x68;
const unsigned long JEDA_CETAK = 100;  // satu titik grafik setiap 100 ms
unsigned long waktuCetakTerakhir = 0;

Adafruit_MPU6050 mpu;

float ax, ay, az;  // percepatan (g)
float gx, gy, gz;  // kecepatan putar (°/s), tidak dipakai di program ini

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

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);  // bus I2C kit: SDA = IO21, SCL = IO22

  if (!siapkanImu()) {
    Serial.println("MPU-6050 tidak ditemukan. Jalankan Program 1 untuk mencari penyebabnya.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }
  // Sengaja tidak mencetak judul: Serial Plotter menganggap setiap baris sebagai data
}

void loop() {
  if (millis() - waktuCetakTerakhir < JEDA_CETAK) {
    return;  // belum 100 ms
  }
  waktuCetakTerakhir = millis();

  if (!bacaImu()) {
    return;  // sensor tidak menjawab, coba lagi 100 ms kemudian
  }

  // Sudut dari arah gravitasi (hasil atan2 dalam radian, dikali RAD_TO_DEG menjadi derajat)
  float roll  = atan2(ay, az) * RAD_TO_DEG;
  float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;

  Serial.printf("Roll:%.1f,Pitch:%.1f\n", roll, pitch);
}
