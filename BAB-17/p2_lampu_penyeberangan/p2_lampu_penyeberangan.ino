/*
 * BAB 17 - Program 2: Lampu Penyeberangan
 * Lampu kendaraan tetap HIJAU sampai tombol penyeberang (S1) ditekan.
 * Setelah hijau menyala minimal 5 detik: KUNING (2 detik) → MERAH (6 detik,
 * penyeberang lewat: LED biru menyala, buzzer berbunyi "tik-tik") → HIJAU.
 *
 * Rangkaian:
 *   LED1 (merah)                → IO4
 *   LED4 (kuning)               → IO16
 *   LED2 (hijau)                → IO17
 *   LED3 (biru)                 → IO2   (lampu penyeberang)
 *   BUZZ                        → IO13
 *   Tombol S1 (pin kuning ke-3) → IO15  (tombol penyeberang)
 */

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2
#define BUZZER_PIN 13
#define TOMBOL_S1  15

enum StatePenyeberangan {
  HIJAU,   // kendaraan jalan, menunggu tombol penyeberang
  KUNING,  // kendaraan bersiap berhenti
  MERAH    // kendaraan berhenti, penyeberang lewat
};

const unsigned long HIJAU_MINIMUM = 5000;  // hijau menyala paling sedikit 5 detik
const unsigned long LAMA_KUNING   = 2000;
const unsigned long LAMA_MERAH    = 6000;

StatePenyeberangan state = HIJAU;
unsigned long waktuMasukState = 0;
bool adaPermintaan = false;  // true = sudah ada penyeberang yang menekan tombol

// Debounce tombol S1 (sama seperti di BAB 9 dan BAB 16)
const unsigned long WAKTU_DEBOUNCE = 50;
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

// Mengembalikan true hanya sekali setiap S1 baru ditekan (sudah di-debounce)
bool s1BaruDitekan() {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = millis();
    bacaanSebelumnya = bacaan;
  }
  if ((millis() - waktuBerubah) > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    return statusStabil == HIGH;  // true hanya pada tepi naik (LOW → HIGH)
  }
  return false;
}

void aturLampu(bool merah, bool kuning, bool hijau, bool biru) {
  digitalWrite(LED_MERAH, merah ? HIGH : LOW);
  digitalWrite(LED_KUNING, kuning ? HIGH : LOW);
  digitalWrite(LED_HIJAU, hijau ? HIGH : LOW);
  digitalWrite(LED_BIRU, biru ? HIGH : LOW);
}

// Pindah ke state baru, lalu jalankan aksi masuknya (SEKALI)
void pindahState(StatePenyeberangan stateBaru) {
  state = stateBaru;
  waktuMasukState = millis();
  digitalWrite(BUZZER_PIN, LOW);  // buzzer selalu diam saat state berganti

  switch (state) {
    case HIJAU:
      aturLampu(false, false, true, false);
      adaPermintaan = false;      // permintaan sebelumnya sudah dilayani
      Serial.println("[HIJAU]  kendaraan jalan, menunggu tombol penyeberang");
      break;
    case KUNING:
      aturLampu(false, true, false, false);
      Serial.println("[KUNING] kendaraan bersiap berhenti");
      break;
    case MERAH:
      aturLampu(true, false, false, true);  // merah untuk kendaraan, biru untuk penyeberang
      Serial.println("[MERAH]  kendaraan berhenti, penyeberang boleh lewat");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_BIRU, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TOMBOL_S1, INPUT);

  Serial.println("BAB 17 - Program 2: Lampu Penyeberangan");
  Serial.println("Tekan S1 untuk meminta waktu menyeberang.");
  pindahState(HIJAU);
}

void loop() {
  // Baca event SEKALI di awal putaran, lalu setiap state memutuskan sendiri
  bool tombolDitekan = s1BaruDitekan();
  unsigned long lamaDiState = millis() - waktuMasukState;

  switch (state) {
    case HIJAU:
      // Event tombol: catat permintaan, tetapi belum pindah state
      if (tombolDitekan && !adaPermintaan) {
        adaPermintaan = true;
        Serial.println("Tombol S1 ditekan: permintaan menyeberang dicatat");
      }
      // Guard: pindah hanya kalau ada permintaan DAN hijau sudah cukup lama
      if (adaPermintaan && lamaDiState >= HIJAU_MINIMUM) {
        pindahState(KUNING);
      }
      break;

    case KUNING:
      if (tombolDitekan) {
        Serial.println("Tombol S1 diabaikan: lampu sedang berganti");
      }
      if (lamaDiState >= LAMA_KUNING) {
        pindahState(MERAH);
      }
      break;

    case MERAH:
      // Aksi selama di state: buzzer berbunyi "tik" 30 ms setiap 500 ms
      digitalWrite(BUZZER_PIN, (lamaDiState % 500) < 30 ? HIGH : LOW);
      if (tombolDitekan) {
        Serial.println("Tombol S1 diabaikan: penyeberang sedang lewat");
      }
      if (lamaDiState >= LAMA_MERAH) {
        pindahState(HIJAU);
      }
      break;
  }
}
