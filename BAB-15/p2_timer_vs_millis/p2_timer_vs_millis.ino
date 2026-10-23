/*
 * BAB 15 - Program 2: Timer Hardware vs millis()
 * LED merah dikedipkan timer hardware, LED kuning dikedipkan millis() di loop().
 * Setiap 5 detik loop() "sibuk" 2 detik: LED kuning macet, LED merah tetap berkedip.
 *
 * Rangkaian:
 *   LED1 (LED merah, header LED)  → IO4
 *   LED4 (LED kuning, header LED) → IO16
 */

#define LED_MERAH  4
#define LED_KUNING 16

const unsigned long INTERVAL_KUNING = 250;  // ms, sama dengan LED merah

hw_timer_t *timer = NULL;
volatile bool merahNyala = false;  // keadaan LED merah, diubah di ISR

bool kuningNyala = false;
unsigned long waktuKuning = 0;  // terakhir kali LED kuning dibalik
unsigned long waktuSibuk = 0;   // terakhir kali loop() selesai "sibuk"

// ISR: membalik LED merah setiap 250 ms, apa pun yang sedang dikerjakan loop()
void IRAM_ATTR saatWaktuHabis() {
  merahNyala = !merahNyala;
  digitalWrite(LED_MERAH, merahNyala ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_KUNING, LOW);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 15 - Program 2: Timer Hardware vs millis()");

  timer = timerBegin(1000000);  // 1 tick = 1 µs
  if (timer == NULL) {
    Serial.println("Gagal membuat timer! Program berhenti.");
    while (true) {
      delay(1000);
    }
  }
  timerAttachInterrupt(timer, &saatWaktuHabis);
  timerAlarm(timer, 250000, true, 0);  // 250.000 µs = 250 ms, berulang

  Serial.println("LED merah: timer hardware | LED kuning: millis()");
}

void loop() {
  // LED kuning: pola millis(). Hanya dibalik kalau loop() sempat memeriksa jam.
  if (millis() - waktuKuning >= INTERVAL_KUNING) {
    waktuKuning = millis();
    kuningNyala = !kuningNyala;
    digitalWrite(LED_KUNING, kuningNyala ? HIGH : LOW);
  }

  // Setiap 5 detik, pura-pura mengerjakan tugas berat selama 2 detik
  if (millis() - waktuSibuk >= 5000) {
    Serial.println("loop() sibuk 2 detik... LED kuning macet, LED merah tetap berkedip");
    delay(2000);  // simulasi tugas berat, misalnya menunggu sensor yang lambat
    Serial.println("loop() selesai sibuk");
    waktuSibuk = millis();
  }
}
