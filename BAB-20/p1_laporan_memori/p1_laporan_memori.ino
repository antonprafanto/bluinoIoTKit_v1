/*
 * BAB 20 - Program 1: Laporan Memori ESP32
 * Menampilkan keadaan heap, stack loop(), flash, dan PSRAM sekali saat mulai,
 * lalu memantau heap setiap 5 detik.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino (tidak perlu merangkai)
 * Alat uji : Serial Monitor, baud 115200
 */

#include <esp_heap_caps.h>  // fungsi heap_caps_...() dari ESP-IDF

const unsigned long INTERVAL_PANTAU = 5000;  // ms: jeda antarlaporan di loop()
unsigned long waktuPantau = 0;               // kapan laporan terakhir dicetak

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor bersiap

  Serial.println("=== Laporan Memori ESP32 ===");
  Serial.println();

  // ----- Heap: RAM yang dipinjam-kembalikan saat program berjalan -----
  Serial.println("[Heap]");
  Serial.print("Total heap internal  : ");
  Serial.print(ESP.getHeapSize() / 1024);  // byte -> KB
  Serial.println(" KB");
  Serial.print("Heap bebas           : ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");
  Serial.print("Heap bebas terendah  : ");
  Serial.print(ESP.getMinFreeHeap() / 1024);
  Serial.println(" KB");
  Serial.print("Blok bebas terbesar  : ");
  Serial.print(ESP.getMaxAllocHeap() / 1024);
  Serial.println(" KB");
  Serial.print("Heap bebas untuk data: ");
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);  // tanpa bagian IRAM
  Serial.println(" KB");
  Serial.println();

  // ----- Stack milik loopTask (yang menjalankan setup() dan loop()) -----
  Serial.println("[Stack loop()]");
  Serial.print("Ukuran stack         : ");
  Serial.print(getArduinoLoopTaskStackSize());
  Serial.println(" byte");
  Serial.print("Sisa stack minimum   : ");
  Serial.print(uxTaskGetStackHighWaterMark(NULL));  // NULL = task yang sedang berjalan
  Serial.println(" byte");
  Serial.println();

  // ----- Flash -----
  Serial.println("[Flash]");
  Serial.print("Ukuran chip flash    : ");
  Serial.print(ESP.getFlashChipSize() / (1024 * 1024));  // byte -> MB
  Serial.println(" MB");
  Serial.print("Ukuran program       : ");
  Serial.print(ESP.getSketchSize() / 1024);
  Serial.println(" KB");
  Serial.print("Batas ukuran program : ");
  Serial.print(ESP.getFreeSketchSpace() / 1024);  // ukuran partisi program
  Serial.println(" KB");
  Serial.println();

  // ----- PSRAM -----
  Serial.println("[PSRAM]");
  Serial.print("Ukuran PSRAM         : ");
  Serial.print(ESP.getPsramSize());
  Serial.println(" byte");
  Serial.println();

  Serial.println("=== Pemantauan heap setiap 5 detik ===");
}

void loop() {
  if (millis() - waktuPantau >= INTERVAL_PANTAU) {
    waktuPantau = millis();

    Serial.print("Detik ke-");
    Serial.print(millis() / 1000);
    Serial.print(" | bebas: ");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" byte | terendah: ");
    Serial.print(ESP.getMinFreeHeap());
    Serial.print(" byte | blok terbesar: ");
    Serial.print(ESP.getMaxAllocHeap());
    Serial.println(" byte");
  }
}
