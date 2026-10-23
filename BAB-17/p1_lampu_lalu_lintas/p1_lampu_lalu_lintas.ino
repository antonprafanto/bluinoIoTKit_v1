/*
 * BAB 17 - Program 1: Lampu Lalu Lintas
 * State machine yang dipicu waktu:
 *   MERAH (5 detik) → HIJAU (4 detik) → KUNING (2 detik) → MERAH → ...
 *
 * Rangkaian:
 *   LED1 (merah)  → IO4
 *   LED4 (kuning) → IO16
 *   LED2 (hijau)  → IO17
 */

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17

// 1. Daftar semua state
enum StateLampu {
  MERAH,
  HIJAU,
  KUNING
};

// 2. Lama setiap state (ms)
const unsigned long LAMA_MERAH  = 5000;
const unsigned long LAMA_HIJAU  = 4000;
const unsigned long LAMA_KUNING = 2000;

// 3. Variabel state dan waktu masuknya
StateLampu state = MERAH;
unsigned long waktuMasukState = 0;  // kapan state sekarang dimulai

void aturLampu(bool merah, bool kuning, bool hijau) {
  digitalWrite(LED_MERAH, merah ? HIGH : LOW);
  digitalWrite(LED_KUNING, kuning ? HIGH : LOW);
  digitalWrite(LED_HIJAU, hijau ? HIGH : LOW);
}

// 4. Pindah ke state baru, lalu jalankan aksi masuknya (SEKALI)
void pindahState(StateLampu stateBaru) {
  state = stateBaru;
  waktuMasukState = millis();

  switch (state) {
    case MERAH:
      aturLampu(true, false, false);
      Serial.print("[MERAH]  kendaraan berhenti selama ");
      Serial.print(LAMA_MERAH / 1000);
      Serial.println(" detik");
      break;
    case HIJAU:
      aturLampu(false, false, true);
      Serial.print("[HIJAU]  kendaraan boleh jalan selama ");
      Serial.print(LAMA_HIJAU / 1000);
      Serial.println(" detik");
      break;
    case KUNING:
      aturLampu(false, true, false);
      Serial.print("[KUNING] kendaraan bersiap berhenti selama ");
      Serial.print(LAMA_KUNING / 1000);
      Serial.println(" detik");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);

  Serial.println("BAB 17 - Program 1: Lampu Lalu Lintas");
  pindahState(MERAH);  // masuk ke state awal sekaligus menjalankan aksi masuknya
}

void loop() {
  unsigned long lamaDiState = millis() - waktuMasukState;

  // 5. Periksa transisi milik state saat ini (sesuai tabel transisi)
  switch (state) {
    case MERAH:
      if (lamaDiState >= LAMA_MERAH) pindahState(HIJAU);
      break;
    case HIJAU:
      if (lamaDiState >= LAMA_HIJAU) pindahState(KUNING);
      break;
    case KUNING:
      if (lamaDiState >= LAMA_KUNING) pindahState(MERAH);
      break;
  }

  // Tugas lain boleh ditambahkan di sini — tidak ada yang menunggu
}
