/*
 * BAB 15 - Program 3: Lampu Tangga dengan Alarm Sekali Jalan
 * Tekan S1 → LED merah menyala 5 detik, lalu mati sendiri.
 * Menekan S1 lagi saat LED masih menyala mengulang hitungan 5 detik dari awal.
 *
 * Rangkaian:
 *   SW1 (tombol S1, header kuning) → IO15
 *   LED1 (LED merah, header LED)   → IO4
 */

#define TOMBOL_S1 15
#define LED_MERAH 4

const uint64_t LAMA_NYALA_US = 5000000;   // 5 detik = 5.000.000 µs
const unsigned long WAKTU_DEBOUNCE = 50;  // ms, sama seperti BAB 9

hw_timer_t *timer = NULL;
volatile bool waktuHabis = false;  // diisi true oleh ISR

bool lampuNyala = false;

// Variabel debounce (pola dari BAB 9)
int bacaanSebelumnya = LOW;      // bacaan mentah pada putaran loop() sebelumnya
int statusStabil = LOW;          // keadaan tombol yang sudah lolos debounce
unsigned long waktuBerubah = 0;  // kapan bacaan mentah terakhir kali berubah

// ISR: alarm sekali jalan berbunyi → cukup beri tanda untuk loop()
void IRAM_ATTR saatWaktuHabis() {
  waktuHabis = true;
}

void nyalakanLampu() {
  timerRestart(timer);                         // counter kembali ke 0
  timerAlarm(timer, LAMA_NYALA_US, false, 0);  // alarm SEKALI JALAN, 5 detik dari 0
  waktuHabis = false;                          // abaikan tanda alarm lama (kalau ada)
  timerStart(timer);                           // jalankan (tidak berpengaruh kalau sudah berjalan)

  digitalWrite(LED_MERAH, HIGH);
  if (lampuNyala) {
    Serial.println("S1 ditekan lagi: hitungan 5 detik diulang dari awal");
  } else {
    Serial.println("S1 ditekan: lampu NYALA selama 5 detik");
  }
  lampuNyala = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);  // pull-down 10 kΩ di PCB: ditekan = HIGH
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 15 - Program 3: Lampu Tangga (alarm sekali jalan)");

  timer = timerBegin(1000000);  // 1 tick = 1 µs; timer langsung berjalan...
  if (timer == NULL) {
    Serial.println("Gagal membuat timer! Program berhenti.");
    while (true) {
      delay(1000);
    }
  }
  timerStop(timer);             // ...jadi hentikan dulu sampai S1 ditekan
  timerAttachInterrupt(timer, &saatWaktuHabis);

  Serial.println("Tekan S1 untuk menyalakan lampu.");
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
      nyalakanLampu();
    }
  }

  // 2. Alarm sekali jalan sudah berbunyi? Matikan lampu.
  if (waktuHabis) {
    waktuHabis = false;
    timerStop(timer);  // timer tidak perlu berjalan lagi sampai S1 ditekan
    digitalWrite(LED_MERAH, LOW);
    lampuNyala = false;
    Serial.println("Waktu habis: lampu MATI");
  }
}
