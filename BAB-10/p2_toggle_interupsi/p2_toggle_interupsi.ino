/*
 * BAB 10 - Program 2: Sakelar Toggle dengan Interupsi dan Debounce
 * Setiap kali S1 ditekan, LED merah berganti keadaan (nyala <-> mati).
 * ISR hanya mencatat waktu perubahan dan mengangkat bendera;
 * debounce dan aksi dikerjakan di loop().
 *
 * Rangkaian:
 *   SW1 (tombol S1, header kuning) → IO15
 *   LED1 (merah, header LED)       → IO4
 */

#define TOMBOL_S1 15
#define LED_MERAH 4

const unsigned long WAKTU_DEBOUNCE = 50;  // ms: tombol harus tenang selama ini

// Variabel bersama ISR <-> loop(): wajib volatile
volatile bool adaPerubahan = false;         // bendera: pin S1 berubah (ditekan/dilepas/memantul)
volatile unsigned long waktuPerubahan = 0;  // kapan perubahan terakhir terjadi (ms)

// Kunci bagian kritis: melindungi adaPerubahan dan waktuPerubahan
portMUX_TYPE kunciTombol = portMUX_INITIALIZER_UNLOCKED;

// Variabel yang hanya dipakai loop()
int statusStabil = LOW;  // keadaan tombol yang sudah lolos debounce
bool ledNyala = false;   // keadaan LED: true = nyala, false = mati
int jumlahTekan = 0;

// ISR: dipanggil di SETIAP perubahan IO15 (mode CHANGE), termasuk setiap pantulan
void IRAM_ATTR isrTombolS1() {
  portENTER_CRITICAL_ISR(&kunciTombol);
  waktuPerubahan = millis();  // "stopwatch" debounce diulang dari nol
  adaPerubahan = true;        // angkat bendera
  portEXIT_CRITICAL_ISR(&kunciTombol);
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);

  // CHANGE: ISR terpanggil saat tombol ditekan, dilepas, dan setiap pantulan
  attachInterrupt(digitalPinToInterrupt(TOMBOL_S1), isrTombolS1, CHANGE);

  Serial.println("BAB 10 - Program 2: Sakelar Toggle dengan Interupsi dan Debounce");
  Serial.println("Tekan S1 untuk menyalakan/mematikan LED merah.");
}

void loop() {
  bool sudahTenang = false;

  // 1. Periksa bendera dan waktunya di dalam bagian kritis, supaya ISR
  //    tidak mengubahnya di tengah-tengah pemeriksaan.
  portENTER_CRITICAL(&kunciTombol);
  if (adaPerubahan && (millis() - waktuPerubahan > WAKTU_DEBOUNCE)) {
    adaPerubahan = false;  // turunkan bendera
    sudahTenang = true;
  }
  portEXIT_CRITICAL(&kunciTombol);

  // 2. Tombol sudah tenang lebih dari 50 ms → baca keadaan sebenarnya
  if (sudahTenang) {
    int bacaan = digitalRead(TOMBOL_S1);

    if (bacaan != statusStabil) {
      statusStabil = bacaan;

      // 3. Hanya bereaksi saat tombol baru ditekan (LOW → HIGH)
      if (statusStabil == HIGH) {
        ledNyala = !ledNyala;  // balik keadaan LED
        digitalWrite(LED_MERAH, ledNyala ? HIGH : LOW);
        jumlahTekan++;

        Serial.print("Tekan ke-");
        Serial.print(jumlahTekan);
        Serial.print(" -> LED ");
        Serial.println(ledNyala ? "NYALA" : "MATI");
      }
    }
  }

  // Di sini loop() bebas mengerjakan hal lain, asalkan tanpa delay() yang panjang.
}
