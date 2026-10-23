/*
 * BAB 20 - Program 3: Kebocoran Memori (Memory Leak)
 * Setiap 100 ms program meminjam 1 KB heap untuk "menyiapkan data".
 * Versi bocor lupa mengembalikannya, sehingga heap terus berkurang
 * sampai malloc() gagal. Ubah PAKAI_VERSI_BOCOR menjadi false untuk
 * melihat versi yang benar.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino (tidak perlu merangkai)
 * Alat uji : Serial Monitor, baud 115200
 */

#include <esp_heap_caps.h>

const bool PAKAI_VERSI_BOCOR = true;    // true = versi bocor, false = versi benar
const size_t UKURAN_DATA = 1024;        // byte yang dipinjam setiap pengiriman
const unsigned long JEDA_KIRIM = 100;   // ms antarpengiriman

unsigned long jumlahKirim = 0;   // berapa kali data sudah "dikirim"
unsigned long waktuKirim = 0;    // kapan pengiriman terakhir
uint8_t *alamatTerakhir = NULL;  // alamat buffer terakhir, hanya untuk ditampilkan
bool heapHabis = false;          // menjadi true setelah malloc() gagal

// Heap yang bisa dipakai malloc(), new, dan String
uint32_t heapBebasData() {
  return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}

// ❌ Versi bocor: buffer dipinjam, tetapi tidak pernah dikembalikan
bool kirimDataBocor() {
  uint8_t *buffer = (uint8_t *)malloc(UKURAN_DATA);
  if (buffer == NULL) {
    return false;  // heap tidak cukup
  }
  memset(buffer, 0x55, UKURAN_DATA);  // pura-pura menyiapkan data
  // ... di program sungguhan, isi buffer dikirim lewat WiFi/Serial di sini ...
  alamatTerakhir = buffer;
  return true;  // LUPA free(buffer)! Pinjaman yang lama tidak pernah kembali
}

// ✅ Versi benar: setiap malloc() punya pasangan free()
bool kirimDataBenar() {
  uint8_t *buffer = (uint8_t *)malloc(UKURAN_DATA);
  if (buffer == NULL) {
    return false;
  }
  memset(buffer, 0x55, UKURAN_DATA);
  // ... kirim data ...
  alamatTerakhir = buffer;
  free(buffer);  // kembalikan ke heap
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BAB 20 - Program 3: Kebocoran Memori");
  if (PAKAI_VERSI_BOCOR) {
    Serial.println("Mode: VERSI BOCOR (lupa free)");
  } else {
    Serial.println("Mode: VERSI BENAR (malloc + free)");
  }
  Serial.print("Heap bebas untuk data di awal: ");
  Serial.print(heapBebasData() / 1024);
  Serial.println(" KB");
}

void loop() {
  if (heapHabis) {
    return;  // berhenti mengirim, tetapi ESP32 tetap hidup
  }
  if (millis() - waktuKirim < JEDA_KIRIM) {
    return;  // belum waktunya mengirim lagi
  }
  waktuKirim = millis();

  bool berhasil = PAKAI_VERSI_BOCOR ? kirimDataBocor() : kirimDataBenar();

  if (!berhasil) {
    heapHabis = true;
    Serial.print("malloc() GAGAL pada pengiriman ke-");
    Serial.print(jumlahKirim + 1);
    Serial.println(": heap habis!");
    Serial.println("Program berhenti mengirim data. Tekan EN untuk mengulang.");
    return;
  }

  jumlahKirim = jumlahKirim + 1;
  if (jumlahKirim % 10 == 0) {  // laporan setiap 10 pengiriman (1 detik)
    Serial.print("Kirim ke-");
    Serial.print(jumlahKirim);
    Serial.print(" | buffer di 0x");
    Serial.print((uint32_t)alamatTerakhir, HEX);
    Serial.print(" | heap bebas untuk data: ");
    Serial.print(heapBebasData() / 1024);
    Serial.println(" KB");
  }
}
