/*
 * BAB 32 - Program 1: Membaca Tegangan dan Hambatan LDR
 * Setiap 500 ms menampilkan nilai ADC mentah, tegangan terkalibrasi (mV),
 * dan hambatan LDR hasil hitungan rumus pembagi tegangan. Tanpa delay().
 *
 * Rangkaian: LDR (header kuning, pin ke-1) → IO35
 * Rangkaian di kit: 3V3 → 10 kΩ → titik tengah (IO35) → LDR → GND
 */

#define LDR_PIN 35  // LDR → IO35 (ADC1_CH7, aman dipakai bersama WiFi)

const float R_TETAP_KOHM = 10.0;        // resistor tetap di kit: 10 kΩ
const float TEGANGAN_3V3_MV = 3300.0;   // tegangan 3V3 kit; ganti dengan hasil ukur multimeter kalau ada
const int BATAS_TELITI_MV = 150;        // di bawah ini, ADC kurang teliti
const int ADC_MENTOK = 4095;            // nilai terbesar ADC 12 bit

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

Timer timerLaporan = {500, 0};  // laporan setiap 500 ms

// Menghitung hambatan LDR (kΩ) dari tegangan titik tengah (mV).
// Hasil -1 berarti tidak bisa dihitung.
float hitungHambatanKohm(int milivolt) {
  if (milivolt >= TEGANGAN_3V3_MV) {
    return -1.0;  // penyebut nol atau negatif
  }
  return R_TETAP_KOHM * milivolt / (TEGANGAN_3V3_MV - milivolt);
}

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 32 - Program 1: Tegangan dan Hambatan LDR");
  Serial.println("Tutup LDR dengan jari, lalu sinari dengan senter HP...");
}

void loop() {
  unsigned long sekarang = millis();

  if (timerLaporan.sudahWaktunya(sekarang)) {
    int nilaiAdc = analogRead(LDR_PIN);            // 0-4095, makin gelap makin besar
    int milivolt = analogReadMilliVolts(LDR_PIN);  // tegangan terkalibrasi (mV)
    float hambatan = hitungHambatanKohm(milivolt);

    Serial.printf("ADC: %4d | Tegangan: %4d mV | Hambatan LDR: ", nilaiAdc, milivolt);

    if (hambatan < 0) {
      Serial.println("tidak bisa dihitung");
    } else if (nilaiAdc >= ADC_MENTOK) {
      // ADC mentok: hambatan sebenarnya lebih besar daripada hasil hitungan
      Serial.printf("lebih dari %.0f kOhm (terlalu gelap, ADC mentok)\n", hambatan);
    } else if (milivolt < BATAS_TELITI_MV) {
      Serial.printf("%.2f kOhm (terlalu terang, kurang teliti)\n", hambatan);
    } else {
      Serial.printf("%.2f kOhm\n", hambatan);
    }
  }

  // Di sini loop() bebas mengerjakan tugas lain, karena tidak ada delay().
}
