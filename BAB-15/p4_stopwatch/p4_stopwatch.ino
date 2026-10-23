/*
 * BAB 15 - Program 4: Stopwatch dengan Timer Hardware
 * Tekan S1: stopwatch mulai dari 0 (LED merah menyala).
 * Tekan S1 lagi: stopwatch berhenti dan hasilnya ditampilkan.
 * Timer hardware dipakai sebagai "jam" presisi, tanpa ISR.
 *
 * Rangkaian:
 *   SW1 (tombol S1, header kuning) → IO15
 *   LED1 (LED merah, header LED)   → IO4
 */

#define TOMBOL_S1 15
#define LED_MERAH 4

const unsigned long WAKTU_DEBOUNCE = 50;  // ms, sama seperti BAB 9

hw_timer_t *timer = NULL;
bool berjalan = false;         // apakah stopwatch sedang berjalan
unsigned long waktuLapor = 0;  // untuk laporan setiap 1 detik

// Variabel debounce (pola dari BAB 9)
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

void tombolDitekan() {
  if (!berjalan) {
    timerWrite(timer, 0);  // counter diisi 0: mulai dari nol
    timerStart(timer);
    berjalan = true;
    waktuLapor = millis();
    digitalWrite(LED_MERAH, HIGH);
    Serial.println("Mulai!");
  } else {
    timerStop(timer);      // counter berhenti; nilainya tetap tersimpan
    berjalan = false;
    digitalWrite(LED_MERAH, LOW);

    uint64_t jumlahTick = timerRead(timer);  // 1 tick = 1 µs
    double detik = timerReadSeconds(timer);  // sudah dalam detik
    Serial.print("Berhenti! Waktu: ");
    Serial.print(detik, 3);  // 3 angka di belakang titik desimal
    Serial.print(" detik (");
    Serial.print(jumlahTick);
    Serial.println(" tick)");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);  // pull-down 10 kΩ di PCB: ditekan = HIGH
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 15 - Program 4: Stopwatch dengan Timer Hardware");

  timer = timerBegin(1000000);  // 1 tick = 1 µs
  if (timer == NULL) {
    Serial.println("Gagal membuat timer! Program berhenti.");
    while (true) {
      delay(1000);
    }
  }
  timerStop(timer);       // timerBegin() langsung berjalan; hentikan dulu
  timerWrite(timer, 0);

  Serial.println("Tekan S1 untuk mulai, tekan lagi untuk berhenti.");
}

void loop() {
  // 1. Baca S1 dengan debounce; bereaksi saat tombol baru ditekan
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = millis();
    bacaanSebelumnya = bacaan;
  }
  if ((millis() - waktuBerubah) > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    if (statusStabil == HIGH) {
      tombolDitekan();
    }
  }

  // 2. Selama berjalan, laporkan waktu setiap 1 detik
  if (berjalan && millis() - waktuLapor >= 1000) {
    waktuLapor += 1000;
    Serial.print("  berjalan: ");
    Serial.print(timerReadMillis(timer));
    Serial.println(" ms");
  }
}
