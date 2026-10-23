/*
 * BAB 33 - Program 3: Pemantau Kesehatan ESP32
 *
 * - Saat mulai : identitas chip dan penyebab reset terakhir.
 * - Setiap 1 s : membaca suhu chip (bacaan tidak valid 53.33 °C dibuang).
 * - Setiap 5 s : laporan lama menyala, suhu chip (rata-rata 10 bacaan
 *                valid terakhir) dan kenaikannya dari suhu awal, serta
 *                sisa memori heap.
 * - LED1 (merah) menyala kalau suhu chip naik >= 15 °C dari suhu awal
 *   atau sisa heap terendah < 20 KB.
 *
 * Rangkaian: LED1 (merah) -> IO4.
 */
#include "esp_system.h"   // esp_reset_reason()

#define LED_MERAH 4       // LED1 (merah) -> IO4

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

const float    BATAS_KENAIKAN_SUHU = 15.0;    // °C di atas suhu awal
const uint32_t BATAS_HEAP_KECIL    = 20480;   // 20 KB (dalam byte)
const int      JUMLAH_RATA         = 10;      // rata-rata 10 bacaan valid terakhir
const long     MENTAH_TIDAK_VALID  = 128;     // 53.33 °C = sensor tidak memberi data

Timer timerSuhu    = { 1000, 0 };   // baca suhu setiap 1 detik
Timer timerLaporan = { 5000, 0 };   // laporan setiap 5 detik

float riwayatSuhu[JUMLAH_RATA];     // 10 bacaan valid terakhir
int   indeksRiwayat = 0;            // kotak berikutnya yang akan diisi
int   isiRiwayat    = 0;            // berapa kotak yang sudah terisi
float suhuAwal      = 0.0;
bool  adaSuhuAwal   = false;

// Menerjemahkan hasil esp_reset_reason() menjadi teks (sama seperti BAB 21)
const char *namaPenyebabReset(esp_reset_reason_t alasan) {
  switch (alasan) {
    case ESP_RST_POWERON:   return "POWERON (baru dinyalakan / tombol EN / upload)";
    case ESP_RST_SW:        return "SW (ESP.restart() dari program)";
    case ESP_RST_PANIC:     return "PANIC (crash: Guru Meditation / abort)";
    case ESP_RST_INT_WDT:   return "INT_WDT (interrupt watchdog)";
    case ESP_RST_TASK_WDT:  return "TASK_WDT (task watchdog: program macet)";
    case ESP_RST_WDT:       return "WDT (watchdog lain)";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP (bangun dari deep sleep)";
    case ESP_RST_BROWNOUT:  return "BROWNOUT (tegangan catu daya turun)";
    default:                return "lainnya";
  }
}

void simpanSuhu(float suhu) {
  riwayatSuhu[indeksRiwayat] = suhu;
  indeksRiwayat = (indeksRiwayat + 1) % JUMLAH_RATA;   // setelah kotak terakhir, kembali ke 0
  if (isiRiwayat < JUMLAH_RATA) {
    isiRiwayat = isiRiwayat + 1;
  }
}

float rataRataSuhu() {
  float total = 0.0;
  for (int i = 0; i < isiRiwayat; i++) {
    total = total + riwayatSuhu[i];
  }
  return total / isiRiwayat;
}

void bacaSuhu() {
  float suhu = temperatureRead();
  if (lroundf(suhu * 1.8f + 32.0f) == MENTAH_TIDAK_VALID) {
    return;   // bacaan tidak valid dibuang
  }
  simpanSuhu(suhu);
  // Suhu awal = rata-rata 10 bacaan valid pertama
  if (!adaSuhuAwal && isiRiwayat == JUMLAH_RATA) {
    suhuAwal = rataRataSuhu();
    adaSuhuAwal = true;
  }
}

void cetakLaporan(unsigned long sekarang) {
  // Lama menyala dalam format jam:menit:detik
  unsigned long detik = sekarang / 1000;
  Serial.printf("[%02lu:%02lu:%02lu] ", detik / 3600, (detik % 3600) / 60, detik % 60);

  // Suhu chip
  bool suhuTerlaluNaik = false;
  if (isiRiwayat == 0) {
    Serial.print("Suhu chip: tidak tersedia");
  } else {
    float rata = rataRataSuhu();
    Serial.printf("Suhu chip: %.1f °C", rata);
    if (adaSuhuAwal) {
      float kenaikan = rata - suhuAwal;
      Serial.printf(" (%+.1f dari awal)", kenaikan);
      suhuTerlaluNaik = (kenaikan >= BATAS_KENAIKAN_SUHU);
    } else {
      Serial.print(" (menghitung suhu awal)");
    }
  }

  // Memori heap
  uint32_t heapBebas    = ESP.getFreeHeap();
  uint32_t heapTerendah = ESP.getMinFreeHeap();
  bool heapKritis = (heapTerendah < BATAS_HEAP_KECIL);
  Serial.printf(" | Heap bebas: %lu KB (terendah %lu KB)",
                (unsigned long)(heapBebas / 1024), (unsigned long)(heapTerendah / 1024));

  // Status dan LED merah
  if (suhuTerlaluNaik) Serial.print(" | PERINGATAN: suhu chip naik tinggi");
  if (heapKritis)      Serial.print(" | PERINGATAN: heap hampir habis");
  if (!suhuTerlaluNaik && !heapKritis) Serial.print(" | AMAN");
  Serial.println();

  digitalWrite(LED_MERAH, (suhuTerlaluNaik || heapKritis) ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);

  int revisi = ESP.getChipRevision();   // contoh: 301 = v3.1
  esp_reset_reason_t alasan = esp_reset_reason();

  Serial.println();
  Serial.println("BAB 33 - Program 3: Pemantau Kesehatan ESP32");
  Serial.printf("Chip           : %s (revisi v%d.%d)\n", ESP.getChipModel(), revisi / 100, revisi % 100);
  Serial.printf("Jumlah inti    : %d\n", ESP.getChipCores());
  Serial.printf("Kecepatan CPU  : %lu MHz\n", (unsigned long)ESP.getCpuFreqMHz());
  Serial.printf("Penyebab reset : %s\n", namaPenyebabReset(alasan));
  if (alasan == ESP_RST_BROWNOUT) {
    Serial.println("                 -> periksa kabel USB dan catu daya (BAB 21)");
  } else if (alasan == ESP_RST_PANIC || alasan == ESP_RST_INT_WDT ||
             alasan == ESP_RST_TASK_WDT || alasan == ESP_RST_WDT) {
    Serial.println("                 -> program sebelumnya crash atau macet (BAB 21)");
  }
  Serial.println("Suhu awal      : rata-rata 10 bacaan valid pertama");
  Serial.printf("LED merah      : menyala kalau suhu chip naik >= %.1f °C atau heap terendah < %lu KB\n",
                BATAS_KENAIKAN_SUHU, (unsigned long)(BATAS_HEAP_KECIL / 1024));
  Serial.println();
}

void loop() {
  unsigned long sekarang = millis();

  if (timerSuhu.sudahWaktunya(sekarang)) {
    bacaSuhu();
  }

  if (timerLaporan.sudahWaktunya(sekarang)) {
    cetakLaporan(sekarang);
  }
}
