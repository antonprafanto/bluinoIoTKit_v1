/*
 * BAB 15 - Program 1: LED Berkedip dengan Timer Hardware
 * Timer hardware memanggil ISR setiap 500 ms. ISR membalik LED merah,
 * menghitung jumlah alarm, dan mencatat waktunya. loop() hanya mencetak.
 *
 * Rangkaian: LED1 (LED merah, header LED) → IO4
 */

#define LED_MERAH 4

const uint64_t INTERVAL_US = 500000;  // 500.000 µs = 500 ms

hw_timer_t *timer = NULL;                                // pegangan timer
portMUX_TYPE kunciTimer = portMUX_INITIALIZER_UNLOCKED;  // kunci critical section

volatile bool ledNyala = false;     // keadaan LED, diubah di ISR
volatile uint32_t jumlahAlarm = 0;  // berapa kali alarm sudah berbunyi
volatile uint32_t waktuAlarm = 0;   // millis() saat alarm terakhir

uint32_t sudahDicetak = 0;          // jumlah alarm yang sudah dilaporkan loop()

// ISR: dipanggil timer hardware setiap 500 ms
void IRAM_ATTR saatWaktuHabis() {
  ledNyala = !ledNyala;
  digitalWrite(LED_MERAH, ledNyala ? HIGH : LOW);

  portENTER_CRITICAL_ISR(&kunciTimer);
  jumlahAlarm = jumlahAlarm + 1;  // cara menambah variabel volatile tanpa peringatan compiler
  waktuAlarm = millis();
  portEXIT_CRITICAL_ISR(&kunciTimer);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 15 - Program 1: LED Berkedip dengan Timer Hardware");

  timer = timerBegin(1000000);  // 1 MHz → 1 tick = 1 µs; timer langsung berjalan
  if (timer == NULL) {
    Serial.println("Gagal membuat timer! Program berhenti.");
    while (true) {
      delay(1000);
    }
  }
  timerAttachInterrupt(timer, &saatWaktuHabis);  // pasang ISR
  timerAlarm(timer, INTERVAL_US, true, 0);        // alarm berulang setiap 500 ms

  Serial.println("Timer berjalan: alarm setiap 500 ms.");
}

void loop() {
  // Salin data milik ISR di dalam critical section
  portENTER_CRITICAL(&kunciTimer);
  uint32_t jumlah = jumlahAlarm;
  uint32_t waktu = waktuAlarm;
  portEXIT_CRITICAL(&kunciTimer);

  // Lapor hanya kalau ada alarm baru
  if (jumlah != sudahDicetak) {
    sudahDicetak = jumlah;
    Serial.print("Alarm ke-");
    Serial.print(jumlah);
    Serial.print(" pada ");
    Serial.print(waktu);
    Serial.println(" ms");
  }
}
