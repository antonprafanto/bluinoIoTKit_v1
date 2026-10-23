/*
 * BAB 20 - Program 4: Melihat Fragmentasi Heap
 * 1. Mengisi heap dengan blok 1 KB sampai penuh.
 * 2. Mengembalikan blok secara selang-seling -> heap berlubang-lubang.
 * 3. Membuktikan malloc(4096) gagal walaupun heap bebas masih banyak.
 * 4. Mengembalikan semua blok -> malloc(4096) berhasil lagi.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino (tidak perlu merangkai)
 * Alat uji : Serial Monitor, baud 115200
 */

#include <esp_heap_caps.h>

const size_t UKURAN_BLOK = 1024;  // ukuran setiap blok kecil (1 KB)
const int MAKS_BLOK = 400;        // batas jumlah blok yang bisa dicatat

uint8_t *daftarBlok[MAKS_BLOK];   // alamat semua blok yang dipinjam
int jumlahBlok = 0;               // berapa blok yang berhasil dipinjam

// Mencetak heap bebas (untuk data) dan blok bebas terbesarnya
void cetakHeap(const char *keadaan) {
  Serial.print(keadaan);
  Serial.print(" -> heap bebas untuk data: ");
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
  Serial.print(" KB | blok terbesar: ");
  Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  Serial.println(" byte");
}

// Mencoba meminjam sejumlah byte, melaporkan hasilnya, lalu langsung mengembalikannya
void cobaPinjam(size_t ukuran) {
  uint8_t *p = (uint8_t *)malloc(ukuran);
  Serial.print("   Coba malloc(");
  Serial.print(ukuran);
  Serial.print(") -> ");
  if (p == NULL) {
    Serial.println("GAGAL");
  } else {
    Serial.println("BERHASIL");
    free(p);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BAB 20 - Program 4: Melihat Fragmentasi Heap");
  Serial.println();
  cetakHeap("Awal          ");
  cobaPinjam(4096);
  Serial.println();

  // Langkah 1: pinjam blok 1 KB terus sampai malloc() gagal
  Serial.println("Langkah 1: mengisi heap dengan blok 1 KB sampai penuh...");
  while (jumlahBlok < MAKS_BLOK) {
    uint8_t *p = (uint8_t *)malloc(UKURAN_BLOK);
    if (p == NULL) {
      break;  // heap sudah penuh
    }
    daftarBlok[jumlahBlok] = p;
    jumlahBlok = jumlahBlok + 1;
  }
  Serial.print("   Berhasil meminjam ");
  Serial.print(jumlahBlok);
  Serial.println(" blok");
  cetakHeap("Heap penuh    ");
  Serial.println();

  // Langkah 2: kembalikan blok nomor 0, 2, 4, ... (selang-seling)
  Serial.println("Langkah 2: mengembalikan blok secara selang-seling...");
  for (int i = 0; i < jumlahBlok; i += 2) {
    free(daftarBlok[i]);
    daftarBlok[i] = NULL;
  }
  cetakHeap("Heap berlubang");
  cobaPinjam(4096);
  Serial.println();

  // Langkah 3: kembalikan sisa blok (nomor 1, 3, 5, ...)
  Serial.println("Langkah 3: mengembalikan semua blok yang tersisa...");
  for (int i = 1; i < jumlahBlok; i += 2) {
    free(daftarBlok[i]);
    daftarBlok[i] = NULL;
  }
  cetakHeap("Heap utuh lagi");
  cobaPinjam(4096);
}

void loop() {
  // Kosong: percobaan hanya dijalankan sekali di setup().
}
