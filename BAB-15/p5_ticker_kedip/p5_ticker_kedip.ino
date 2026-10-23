/*
 * BAB 15 - Program 5: LED Berkedip dengan Ticker
 * Ticker berulang   : LED merah berkedip setiap 500 ms.
 * Ticker sekali jalan: tekan S1 → LED kuning menyala, lalu mati sendiri setelah 2 detik.
 *
 * Rangkaian:
 *   LED1 (LED merah, header LED)   → IO4
 *   LED4 (LED kuning, header LED)  → IO16
 *   SW1 (tombol S1, header kuning) → IO15
 */

#include <Ticker.h>  // library bawaan board package esp32

#define LED_MERAH  4
#define LED_KUNING 16
#define TOMBOL_S1  15

const unsigned long WAKTU_DEBOUNCE = 50;  // ms, sama seperti BAB 9

Ticker tickerKedip;   // untuk tugas berulang
Ticker tickerKuning;  // untuk tugas sekali jalan

bool merahNyala = false;

// Variabel debounce (pola dari BAB 9)
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

// Dipanggil Ticker setiap 500 ms (bukan ISR, jadi tidak perlu IRAM_ATTR)
void kedipMerah() {
  merahNyala = !merahNyala;
  digitalWrite(LED_MERAH, merahNyala ? HIGH : LOW);
}

// Dipanggil Ticker sekali, 2 detik setelah S1 ditekan
void matikanKuning() {
  digitalWrite(LED_KUNING, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(TOMBOL_S1, INPUT);  // pull-down 10 kΩ di PCB: ditekan = HIGH
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_KUNING, LOW);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 15 - Program 5: LED Berkedip dengan Ticker");

  tickerKedip.attach_ms(500, kedipMerah);  // panggil kedipMerah() setiap 500 ms
  Serial.println("LED merah berkedip. Tekan S1 untuk menyalakan LED kuning 2 detik.");
}

void loop() {
  // Baca S1 dengan debounce; bereaksi saat tombol baru ditekan
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = millis();
    bacaanSebelumnya = bacaan;
  }
  if ((millis() - waktuBerubah) > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    if (statusStabil == HIGH) {
      tickerKuning.once_ms(2000, matikanKuning);  // jadwalkan: matikan 2 detik lagi
      digitalWrite(LED_KUNING, HIGH);             // lalu nyalakan LED kuning
      Serial.println("S1 ditekan: LED kuning menyala 2 detik");
    }
  }
}
