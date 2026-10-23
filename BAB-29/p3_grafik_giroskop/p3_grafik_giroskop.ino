/*
 * BAB 29 - Program 3: Grafik Giroskop dan Kalibrasi Offset
 * Menggambar kecepatan putar X, Y, Z (°/s) di Serial Plotter setiap 50 ms.
 * Perintah lewat kolom pesan Serial Plotter atau Serial Monitor:
 *   k     = kalibrasi offset (kit harus diam sekitar 2 detik)
 *   r     = hapus hasil kalibrasi (kembali ke nilai mentah)
 *   0..6  = pilih filter DLPF (0 = 260 Hz ... 4 = 21 Hz ... 6 = 5 Hz)
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
const unsigned long JEDA_CETAK = 50;  // satu titik grafik setiap 50 ms
const int SAMPEL_KALIBRASI = 200;     // 200 bacaan x 10 ms = 2 detik
unsigned long waktuCetakTerakhir = 0;

Adafruit_MPU6050 mpu;

float ax, ay, az;                                // percepatan (g)
float gx, gy, gz;                                // kecepatan putar mentah (°/s)
float offsetGx = 0, offsetGy = 0, offsetGz = 0;  // hasil kalibrasi (°/s)

// ---------- Fungsi sensor (versi library Adafruit) ----------

// Memulai MPU-6050: ±2 g, ±500 °/s, filter 21 Hz. Hasil: true kalau berhasil.
bool siapkanImu() {
  if (!mpu.begin(MPU_ALAMAT)) {
    return false;
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  return true;
}

// Memilih filter DLPF: 0 = paling lebar (260 Hz) sampai 6 = paling sempit (5 Hz)
void aturFilter(uint8_t pilihan) {
  mpu.setFilterBandwidth((mpu6050_bandwidth_t)pilihan);
}

// Membaca sensor ke variabel ax..az (g) dan gx..gz (°/s). Hasil: true kalau sensor menjawab.
bool bacaImu() {
  // getEvent() tidak melaporkan kegagalan, jadi pastikan dulu sensornya menjawab
  Wire.beginTransmission(MPU_ALAMAT);
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

// ---------- Kalibrasi ----------

// Rata-rata bacaan giroskop saat kit diam = offset
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
  // Sengaja tidak mencetak judul: Serial Plotter menganggap setiap baris sebagai data
}

void loop() {
  // 1. Layani perintah dari kolom pesan
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'k' || c == 'K') {
      kalibrasiGiro();
    } else if (c == 'r' || c == 'R') {
      offsetGx = 0;
      offsetGy = 0;
      offsetGz = 0;
    } else if (c >= '0' && c <= '6') {
      aturFilter(c - '0');
    }
    // karakter lain, misalnya Enter, diabaikan
  }

  // 2. Kirim satu titik grafik setiap 50 ms
  if (millis() - waktuCetakTerakhir < JEDA_CETAK) {
    return;
  }
  waktuCetakTerakhir = millis();

  if (bacaImu()) {
    Serial.printf("GiroX:%.2f,GiroY:%.2f,GiroZ:%.2f\n",
                  gx - offsetGx, gy - offsetGy, gz - offsetGz);
  }
}
