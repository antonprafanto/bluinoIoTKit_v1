/*
 * BAB 32 - Program 2: Perkiraan Lux dengan Kalibrasi Dua Titik
 * Menghitung hambatan LDR, lalu memperkirakan kuat cahaya (lux)
 * dengan rumus gamma dari dua titik kalibrasi:
 *   gamma = log(R1 / R2) / log(E2 / E1)
 *   lux   = E1 * (R1 / R) pangkat (1 / gamma)
 * Angka lux ini PERKIRAAN, bukan hasil alat ukur yang teliti.
 *
 * Rangkaian: LDR (header kuning, pin ke-1) → IO35
 */

#define LDR_PIN 35  // LDR → IO35 (ADC1_CH7)

const float R_TETAP_KOHM = 10.0;       // resistor tetap di kit: 10 kΩ
const float TEGANGAN_3V3_MV = 3300.0;  // tegangan 3V3 kit
const int BATAS_TELITI_MV = 150;       // di bawah ini, ADC kurang teliti
const int ADC_MENTOK = 4095;           // nilai terbesar ADC 12 bit

// Dua titik kalibrasi: ganti dengan hasil ukurmu (lihat 32.5.4).
// Nilai bawaan = angka khas datasheet GL5528, jadi hasilnya sangat kasar.
const float TITIK1_LUX = 10.0;    // titik 1: kuat cahaya (lux)
const float TITIK1_KOHM = 15.0;   // titik 1: hambatan LDR (kΩ)
const float TITIK2_LUX = 100.0;   // titik 2: kuat cahaya (lux)
const float TITIK2_KOHM = 3.77;   // titik 2: hambatan LDR (kΩ)

// Timer serbaguna dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerLaporan = {1000, 0};  // laporan setiap 1 detik
float nilaiGamma = 0.6;          // dihitung ulang di setup()

// Hambatan LDR (kΩ) dari tegangan titik tengah (mV); -1 = tidak bisa dihitung
float hitungHambatanKohm(int milivolt) {
  if (milivolt >= TEGANGAN_3V3_MV) {
    return -1.0;
  }
  return R_TETAP_KOHM * milivolt / (TEGANGAN_3V3_MV - milivolt);
}

// Perkiraan lux dari hambatan LDR (rumus gamma, titik 1 sebagai acuan)
float perkiraanLux(float hambatanKohm) {
  return TITIK1_LUX * pow(TITIK1_KOHM / hambatanKohm, 1.0 / nilaiGamma);
}

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 32 - Program 2: Perkiraan Lux dengan Kalibrasi Dua Titik");

  // Gamma dihitung sekali dari dua titik kalibrasi
  nilaiGamma = log10(TITIK1_KOHM / TITIK2_KOHM) / log10(TITIK2_LUX / TITIK1_LUX);
  if (!(nilaiGamma > 0)) {  // juga menangkap hasil tak terdefinisi (NaN)
    Serial.println("PERINGATAN: titik kalibrasi tidak masuk akal, gamma diganti 0.6.");
    Serial.println("Titik yang lebih terang harus punya hambatan yang lebih kecil.");
    nilaiGamma = 0.6;
  }

  Serial.printf("Titik 1: %.1f lux = %.2f kOhm | Titik 2: %.1f lux = %.2f kOhm | Gamma: %.2f\n",
                TITIK1_LUX, TITIK1_KOHM, TITIK2_LUX, TITIK2_KOHM, nilaiGamma);
  Serial.println("Ingat: angka lux di bawah ini hanya PERKIRAAN.");
}

void loop() {
  unsigned long sekarang = millis();

  if (timerLaporan.sudahWaktunya(sekarang)) {
    int nilaiAdc = analogRead(LDR_PIN);
    int milivolt = analogReadMilliVolts(LDR_PIN);
    float hambatan = hitungHambatanKohm(milivolt);

    if (nilaiAdc >= ADC_MENTOK || hambatan < 0) {
      Serial.printf("Tegangan: %4d mV | terlalu gelap untuk diperkirakan (ADC mentok)\n", milivolt);
    } else if (hambatan == 0) {
      Serial.printf("Tegangan: %4d mV | terlalu terang untuk diperkirakan\n", milivolt);
    } else {
      float lux = perkiraanLux(hambatan);
      Serial.printf("Tegangan: %4d mV | Hambatan: %6.2f kOhm | Cahaya: sekitar ", milivolt, hambatan);
      if (lux < 10) {
        Serial.printf("%.1f lux", lux);  // di bawah 10 lux: tampilkan 1 desimal
      } else {
        Serial.printf("%.0f lux", lux);  // selebihnya cukup bilangan bulat
      }
      if (milivolt < BATAS_TELITI_MV) {
        Serial.print(" (terlalu terang, kurang teliti)");
      }
      Serial.println();
    }
  }
}
