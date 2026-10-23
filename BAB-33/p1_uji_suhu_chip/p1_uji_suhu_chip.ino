/*
 * BAB 33 - Program 1: Menguji Sensor Suhu Chip
 *
 * Membaca suhu chip ESP32 dengan temperatureRead() setiap 1 detik,
 * menampilkan nilai mentahnya (°F), dan menandai bacaan tidak valid
 * (nilai mentah 128 = 53.33 °C). Setiap 30 bacaan, program mencetak
 * ringkasan dan kesimpulan tentang sensor suhu di chip-mu.
 *
 * Rangkaian: tidak perlu - sensornya ada di dalam chip ESP32.
 */

const long MENTAH_TIDAK_VALID = 128;   // nilai mentah yang berarti "tidak ada data"
const int  BACAAN_PER_RINGKASAN = 30;  // ringkasan setiap 30 bacaan

int   nomorBacaan      = 0;
int   jumlahValid      = 0;
int   jumlahTidakValid = 0;
float suhuTerendah     = 0.0;
float suhuTertinggi    = 0.0;

// Menghitung balik nilai mentah (°F, bilangan bulat) dari hasil temperatureRead()
long hitungMentah(float suhuC) {
  return lroundf(suhuC * 1.8f + 32.0f);
}

void cetakRingkasan() {
  Serial.println();
  Serial.println("--- Ringkasan 30 bacaan terakhir ---");
  Serial.printf("Valid       : %d\n", jumlahValid);
  Serial.printf("Tidak valid : %d\n", jumlahTidakValid);

  if (jumlahValid == 0) {
    Serial.println("Rentang     : -");
    Serial.println("Kesimpulan  : sensor suhu chip ini tidak memberi data (selalu 53.33 °C).");
  } else {
    Serial.printf("Rentang     : %.2f °C sampai %.2f °C\n", suhuTerendah, suhuTertinggi);
    if (jumlahTidakValid > 0) {
      Serial.println("Kesimpulan  : sebagian bacaan tidak valid. Buang nilai 53.33 sebelum dipakai.");
    } else {
      Serial.println("Kesimpulan  : sensor memberi data. Ingat: ini suhu chip, belum dikalibrasi.");
    }
  }
  Serial.println("------------------------------------");
  Serial.println();

  // Mulai menghitung lagi untuk ringkasan berikutnya
  jumlahValid = 0;
  jumlahTidakValid = 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println();
  Serial.println("BAB 33 - Program 1: Menguji Sensor Suhu Chip");
  Serial.println("Bacaan | Mentah (°F) | Suhu chip (°C) | Keterangan");
}

void loop() {
  float suhu = temperatureRead();  // suhu di dalam chip, dalam °C
  long mentah = hitungMentah(suhu);
  nomorBacaan = nomorBacaan + 1;

  if (mentah == MENTAH_TIDAK_VALID) {
    jumlahTidakValid = jumlahTidakValid + 1;
    Serial.printf("%6d | %11ld | %14.2f | tidak valid\n", nomorBacaan, mentah, suhu);
  } else {
    // Catat suhu terendah dan tertinggi dari bacaan yang valid saja
    if (jumlahValid == 0) {
      suhuTerendah = suhu;
      suhuTertinggi = suhu;
    }
    if (suhu < suhuTerendah) suhuTerendah = suhu;
    if (suhu > suhuTertinggi) suhuTertinggi = suhu;
    jumlahValid = jumlahValid + 1;
    Serial.printf("%6d | %11ld | %14.2f | OK\n", nomorBacaan, mentah, suhu);
  }

  if (nomorBacaan % BACAAN_PER_RINGKASAN == 0) {
    cetakRingkasan();
  }

  delay(1000);  // cukup untuk alat uji sederhana; program berikutnya non-blocking
}
