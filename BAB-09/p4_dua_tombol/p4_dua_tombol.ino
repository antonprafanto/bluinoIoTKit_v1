/*
 * BAB 9 - Program 4: Dua Tombol Mengatur Jumlah LED
 * S1 menambah satu LED yang menyala, S2 mengurangi satu LED.
 * LED menyala berurutan dari kiri ke kanan seperti indikator level.
 *
 * Rangkaian:
 *   Tombol S1 (pin kuning ke-3) → IO15
 *   Tombol S2 (pin kuning ke-4) → IO25
 *   LED1 (merah) → IO4,  LED2 (hijau)  → IO17
 *   LED3 (biru)  → IO2,  LED4 (kuning) → IO16
 */

#define TOMBOL_S1 15
#define TOMBOL_S2 25

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2

// LED diurutkan sesuai posisinya di kit, dari kiri ke kanan
const int LED_PINS[] = {LED_MERAH, LED_HIJAU, LED_BIRU, LED_KUNING};
const int JUMLAH_LED = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

// Data debounce untuk 2 tombol: indeks 0 = S1, indeks 1 = S2
const int TOMBOL_PINS[] = {TOMBOL_S1, TOMBOL_S2};
const int JUMLAH_TOMBOL = 2;
const unsigned long WAKTU_DEBOUNCE = 50;  // ms

int bacaanSebelumnya[JUMLAH_TOMBOL] = {LOW, LOW};
int statusStabil[JUMLAH_TOMBOL] = {LOW, LOW};
unsigned long waktuBerubah[JUMLAH_TOMBOL] = {0, 0};

int ledMenyala = 0;  // berapa LED yang sedang menyala (0 sampai 4)

// Mengembalikan true hanya satu kali setiap tombol ke-i baru ditekan (sudah di-debounce).
// Panggil fungsi ini di setiap putaran loop() supaya tidak ada perubahan yang terlewat.
bool baruDitekan(int i) {
  int bacaan = digitalRead(TOMBOL_PINS[i]);

  if (bacaan != bacaanSebelumnya[i]) {
    waktuBerubah[i] = millis();
    bacaanSebelumnya[i] = bacaan;
  }

  if ((millis() - waktuBerubah[i]) > WAKTU_DEBOUNCE && bacaan != statusStabil[i]) {
    statusStabil[i] = bacaan;
    return statusStabil[i] == HIGH;  // true hanya pada tepi naik (LOW → HIGH)
  }
  return false;
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < JUMLAH_TOMBOL; i++) {
    pinMode(TOMBOL_PINS[i], INPUT);  // tombol kit: pull-down 10 kΩ sudah ada
  }
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  Serial.println("BAB 9 - Program 4: Dua Tombol Mengatur Jumlah LED");
  Serial.println("S1 = tambah LED, S2 = kurangi LED");
}

void loop() {
  if (baruDitekan(0)) {  // S1 baru ditekan
    if (ledMenyala < JUMLAH_LED) {
      digitalWrite(LED_PINS[ledMenyala], HIGH);
      ledMenyala++;
      Serial.print("S1: tambah -> LED menyala: ");
      Serial.println(ledMenyala);
    } else {
      Serial.println("S1: sudah penuh (4 LED menyala)");
    }
  }

  if (baruDitekan(1)) {  // S2 baru ditekan
    if (ledMenyala > 0) {
      ledMenyala--;
      digitalWrite(LED_PINS[ledMenyala], LOW);
      Serial.print("S2: kurangi -> LED menyala: ");
      Serial.println(ledMenyala);
    } else {
      Serial.println("S2: sudah kosong (0 LED menyala)");
    }
  }
}
