/*
 * BAB 29 - Program 7: Alarm Kemiringan Non-Blocking
 * Roll dan pitch dihitung terus dengan complementary filter (100 kali per detik).
 * Kalau kit miring lebih dari 30°, LED merah menyala dan buzzer berbunyi
 * putus-putus. Alarm baru padam setelah kemiringan kembali di bawah 25°
 * (histeresis). Semua tugas berjalan tanpa delay() memakai struct Timer dari BAB 16.
 *
 * Rangkaian:
 *   LED1 (merah) → IO4, LED2 (hijau) → IO17, BUZZ → IO13
 *   MPU-6050 tersambung ke IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: Adafruit MPU6050 (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define PIN_SDA   21
#define PIN_SCL   22
#define LED_MERAH 4   // LED1 (merah)
#define LED_HIJAU 17  // LED2 (hijau)
#define BUZZER    13  // buzzer aktif

const uint8_t MPU_ALAMAT   = 0x68;
const float ALPHA          = 0.98;  // bobot giroskop pada complementary filter
const float BATAS_MIRING   = 30.0;  // derajat: lebih dari ini, alarm menyala
const float BATAS_AMAN     = 25.0;  // derajat: kurang dari ini, alarm padam lagi
const float TOLERANSI_G    = 0.2;   // akselerometer dipakai hanya kalau |a| = 1 g ± 0.2 g
const int SAMPEL_KALIBRASI = 200;   // 200 bacaan x 10 ms = 2 detik

// Tipe data ditulis di bagian atas supaya dikenali semua fungsi di bawahnya
enum StatusKemiringan { AMAN, MIRING };

struct Timer {
  unsigned long interval;   // jalankan setiap sekian ms
  unsigned long waktuLalu;  // kapan terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerImu     = {10, 0};   // hitung sudut setiap 10 ms
Timer timerBuzzer  = {250, 0};  // buzzer berganti bunyi/diam setiap 250 ms
Timer timerLaporan = {500, 0};  // laporan ke Serial Monitor setiap 500 ms

Adafruit_MPU6050 mpu;

float ax, ay, az;                                // percepatan (g)
float gx, gy, gz;                                // kecepatan putar mentah (°/s)
float offsetGx = 0, offsetGy = 0, offsetGz = 0;  // hasil kalibrasi (°/s)

float roll = 0, pitch = 0;          // hasil complementary filter (°)
unsigned long waktuSampelLalu = 0;  // micros() saat sudut terakhir dihitung
bool imuTersambung = true;
int sampelDiabaikan = 0;            // berapa kali akselerometer diabaikan sejak laporan terakhir
StatusKemiringan status = AMAN;
bool buzzerBunyi = false;

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

// Rata-rata bacaan giroskop saat kit diam = offset
void kalibrasiGiro() {
  float totalX = 0, totalY = 0, totalZ = 0;
  int jumlah = 0;
  for (int i = 0; i < SAMPEL_KALIBRASI; i++) {
    if (bacaImu()) {
      totalX += gx;
      totalY += gy;
      totalZ += gz;
      jumlah++;
    }
    delay(10);  // hanya di setup(), sebelum tugas-tugas lain berjalan
  }
  if (jumlah > 0) {
    offsetGx = totalX / jumlah;
    offsetGy = totalY / jumlah;
    offsetGz = totalZ / jumlah;
  }
}

// ---------- Tugas-tugas ----------

// Hitung roll dan pitch dengan complementary filter
void perbaruiSudut() {
  unsigned long sekarang = micros();
  float dt = (sekarang - waktuSampelLalu) / 1000000.0;  // selang waktu nyata (detik)
  waktuSampelLalu = sekarang;
  if (dt > 0.1) {
    dt = 0.1;  // jaga-jaga kalau loop() sempat tertahan lama
  }

  if (!bacaImu()) {
    imuTersambung = false;
    return;
  }
  imuTersambung = true;

  float rollAksel  = atan2(ay, az) * RAD_TO_DEG;
  float pitchAksel = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;

  // "Satpam" akselerometer: saat terguncang, besar percepatan jauh dari 1 g,
  // jadi sudut dari akselerometer tidak dipercaya dulu
  float totalG = sqrt(ax * ax + ay * ay + az * az);
  float alpha = ALPHA;
  if (fabs(totalG - 1.0) > TOLERANSI_G) {
    alpha = 1.0;  // pakai giroskop saja
    sampelDiabaikan++;
  }

  roll  = alpha * (roll  + (gx - offsetGx) * dt) + (1.0 - alpha) * rollAksel;
  pitch = alpha * (pitch + (gy - offsetGy) * dt) + (1.0 - alpha) * pitchAksel;
}

// State machine alarm dengan histeresis
void perbaruiAlarm() {
  float kemiringan = fabs(roll);
  if (fabs(pitch) > kemiringan) {
    kemiringan = fabs(pitch);  // ambil kemiringan yang paling besar
  }

  if (status == AMAN && kemiringan > BATAS_MIRING) {
    status = MIRING;
  } else if (status == MIRING && kemiringan < BATAS_AMAN) {
    status = AMAN;
    buzzerBunyi = false;
    digitalWrite(BUZZER, LOW);  // pastikan buzzer diam
  }

  digitalWrite(LED_MERAH, status == MIRING ? HIGH : LOW);
  digitalWrite(LED_HIJAU, status == AMAN ? HIGH : LOW);
}

// Sensor hilang: matikan semua output supaya tidak memberi tanda yang salah
void matikanSemua() {
  status = AMAN;
  buzzerBunyi = false;
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  Wire.begin(PIN_SDA, PIN_SCL);  // bus I2C kit: SDA = IO21, SCL = IO22

  Serial.println("BAB 29 - Program 7: Alarm Kemiringan Non-Blocking");

  if (!siapkanImu()) {
    Serial.println("MPU-6050 tidak ditemukan! Jalankan Program 1 untuk mencari penyebabnya.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }

  Serial.println("Kalibrasi giroskop: jangan sentuh kit selama sekitar 2 detik...");
  digitalWrite(LED_MERAH, HIGH);  // kedua LED menyala = sedang kalibrasi
  digitalWrite(LED_HIJAU, HIGH);
  kalibrasiGiro();
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);
  Serial.printf("Offset giroskop (°/s): X=%.2f  Y=%.2f  Z=%.2f\n", offsetGx, offsetGy, offsetGz);

  // Sudut awal diambil dari akselerometer (kit sedang diam)
  if (bacaImu()) {
    roll  = atan2(ay, az) * RAD_TO_DEG;
    pitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;
  }
  waktuSampelLalu = micros();
  Serial.println("Siap. Miringkan kit lebih dari 30° untuk menyalakan alarm.");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: hitung sudut dan perbarui alarm setiap 10 ms
  if (timerImu.sudahWaktunya(sekarang)) {
    perbaruiSudut();
    if (imuTersambung) {
      perbaruiAlarm();
    } else {
      matikanSemua();
    }
  }

  // Tugas 2: buzzer berbunyi putus-putus selama kit miring
  if (status == MIRING && timerBuzzer.sudahWaktunya(sekarang)) {
    buzzerBunyi = !buzzerBunyi;
    digitalWrite(BUZZER, buzzerBunyi ? HIGH : LOW);
  }

  // Tugas 3: laporan ke Serial Monitor setiap 500 ms
  if (timerLaporan.sudahWaktunya(sekarang)) {
    if (imuTersambung) {
      Serial.printf("Roll: %6.1f° | Pitch: %6.1f° | Status: %-6s | Aksel diabaikan: %d\n",
                    roll, pitch, status == MIRING ? "MIRING" : "AMAN", sampelDiabaikan);
    } else {
      Serial.println("MPU-6050 tidak menjawab! Periksa jumper cap-nya.");
    }
    sampelDiabaikan = 0;
  }

  // Tugas lain (tombol, WiFi, OLED) bisa ditambahkan di sini tanpa mengganggu sensor
}
