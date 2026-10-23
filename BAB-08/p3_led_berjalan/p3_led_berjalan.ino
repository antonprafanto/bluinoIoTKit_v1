/*
 * BAB 8 - Program 3: LED Berjalan (Knight Rider)
 * Satu LED menyala bergantian maju-mundur.
 *
 * Rangkaian sama dengan Program 2:
 *   LED1 (merah) → IO4, LED2 (hijau) → IO17,
 *   LED3 (biru)  → IO2, LED4 (kuning) → IO16
 */

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2

// Urutan sesuai posisi di kit (kiri → kanan): merah, hijau, biru, kuning
const int LED_PINS[] = {LED_MERAH, LED_HIJAU, LED_BIRU, LED_KUNING};
const int JUMLAH_LED = sizeof(LED_PINS) / sizeof(LED_PINS[0]);
const int JEDA_MS = 150;  // lama setiap LED menyala; makin kecil makin cepat

// Matikan semua LED
void matikanSemua() {
  for (int i = 0; i < JUMLAH_LED; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

// Nyalakan hanya LED dengan indeks tertentu, lalu tunggu sebentar
void nyalakanSatu(int indeks) {
  matikanSemua();
  digitalWrite(LED_PINS[indeks], HIGH);
  delay(JEDA_MS);
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
  matikanSemua();

  Serial.println("BAB 8 - Program 3: LED Berjalan (Knight Rider)");
}

void loop() {
  // Maju: indeks 0 → 1 → 2 → 3 (merah → hijau → biru → kuning)
  for (int i = 0; i < JUMLAH_LED; i++) {
    nyalakanSatu(i);
  }

  // Mundur: indeks 2 → 1 (kedua ujung tidak diulang supaya gerakannya mulus)
  for (int i = JUMLAH_LED - 2; i > 0; i--) {
    nyalakanSatu(i);
  }
}
