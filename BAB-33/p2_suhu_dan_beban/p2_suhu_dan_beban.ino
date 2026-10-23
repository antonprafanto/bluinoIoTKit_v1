/*
 * BAB 33 - Program 2: Suhu Chip Saat Diam, Sibuk, dan WiFi Menyala
 *
 * Program berganti fase setiap 60 detik:
 *   1. DIAM      : CPU beristirahat
 *   2. CPU SIBUK : CPU menghitung terus-menerus
 *   3. WIFI      : radio WiFi menyala dan terus memindai jaringan
 * Setiap 5 detik, suhu chip dicetak beserta perubahannya sejak awal fase.
 *
 * Rangkaian: tidak perlu. Tidak perlu nama/kata sandi WiFi.
 */
#include <WiFi.h>

// Tipe data ditulis di bagian atas supaya dikenali di seluruh sketch
enum Fase { FASE_DIAM, FASE_SIBUK, FASE_WIFI };
const char *NAMA_FASE[] = { "DIAM", "CPU SIBUK", "WIFI" };

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

const unsigned long LAMA_FASE_MS = 60000;   // setiap fase 60 detik
const long MENTAH_TIDAK_VALID = 128;        // 53.33 °C = sensor tidak memberi data

Timer timerLaporan = { 5000, 0 };   // laporan setiap 5 detik

Fase fase = FASE_DIAM;
unsigned long waktuMulaiFase = 0;
float suhuAwalFase = 0.0;
bool  adaSuhuAwal = false;          // sudah punya suhu awal fase yang valid?
int   jumlahJaringan = 0;           // hasil pindaian WiFi terakhir

volatile float hasilHitung = 1.0;   // volatile: hitungan "kerja berat" tidak dibuang compiler

bool suhuValid(float suhu) {
  return lroundf(suhu * 1.8f + 32.0f) != MENTAH_TIDAK_VALID;
}

// Pekerjaan hitung tanpa arti, hanya untuk membuat CPU sibuk beberapa milidetik
void kerjaBerat() {
  float x = hasilHitung;
  for (int i = 0; i < 100000; i++) {
    x = x * 1.0001f + 0.0001f;
    if (x > 1000.0f) x = 1.0f;
  }
  hasilHitung = x;
}

void mulaiFase(Fase baru) {
  // Matikan WiFi kalau fase sebelumnya WIFI
  if (fase == FASE_WIFI && baru != FASE_WIFI) {
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
  }

  fase = baru;
  waktuMulaiFase = millis();
  timerLaporan.waktuLalu = waktuMulaiFase;   // laporan pertama 5 detik lagi

  float suhu = temperatureRead();
  adaSuhuAwal = suhuValid(suhu);
  suhuAwalFase = suhu;

  if (fase == FASE_WIFI) {
    jumlahJaringan = 0;
    WiFi.mode(WIFI_STA);        // nyalakan radio WiFi (mode stasiun)
    WiFi.scanNetworks(true);    // true = memindai di latar belakang
  }

  Serial.println();
  Serial.printf("=== Fase %s dimulai (%lu detik) ===\n", NAMA_FASE[fase], LAMA_FASE_MS / 1000);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println();
  Serial.println("BAB 33 - Program 2: Suhu Chip Saat Diam, Sibuk, dan WiFi Menyala");
  WiFi.mode(WIFI_OFF);          // pastikan radio WiFi mati di awal
  fase = FASE_DIAM;
  mulaiFase(FASE_DIAM);
}

void loop() {
  unsigned long sekarang = millis();

  // 1. Ganti fase setiap 60 detik: DIAM -> CPU SIBUK -> WIFI -> DIAM ...
  if (sekarang - waktuMulaiFase >= LAMA_FASE_MS) {
    if (fase == FASE_DIAM)       mulaiFase(FASE_SIBUK);
    else if (fase == FASE_SIBUK) mulaiFase(FASE_WIFI);
    else                         mulaiFase(FASE_DIAM);
  }

  // 2. Laporan setiap 5 detik
  if (timerLaporan.sudahWaktunya(sekarang)) {
    unsigned long detikFase = (sekarang - waktuMulaiFase) / 1000;
    float suhu = temperatureRead();

    Serial.printf("[%-9s %2lu s] ", NAMA_FASE[fase], detikFase);
    if (!suhuValid(suhu)) {
      Serial.print("suhu chip tidak valid (53.33 °C)");
    } else {
      if (!adaSuhuAwal) {           // suhu awal fase belum ada: pakai bacaan ini
        suhuAwalFase = suhu;
        adaSuhuAwal = true;
      }
      Serial.printf("suhu chip %.2f °C | perubahan sejak awal fase %+.2f °C",
                    suhu, suhu - suhuAwalFase);
    }
    if (fase == FASE_WIFI) {
      Serial.printf(" | jaringan terdeteksi: %d", jumlahJaringan);
    }
    Serial.println();
  }

  // 3. Pekerjaan sesuai fase
  if (fase == FASE_SIBUK) {
    kerjaBerat();                   // CPU terus menghitung, tanpa istirahat
  } else {
    if (fase == FASE_WIFI) {
      int16_t hasil = WiFi.scanComplete();
      if (hasil >= 0) {             // pindaian selesai: simpan jumlahnya, mulai lagi
        jumlahJaringan = hasil;
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
      } else if (hasil == WIFI_SCAN_FAILED) {
        WiFi.scanNetworks(true);    // pindaian gagal/belum jalan: coba lagi
      }
    }
    delay(10);                      // DIAM dan WIFI: beri CPU waktu beristirahat
  }
}
