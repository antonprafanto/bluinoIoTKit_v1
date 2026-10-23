/*
 * BAB 14 - Program 4: Interrupt Sentuh
 * Driver touch memanggil fungsi kita saat pad disentuh DAN saat dilepas.
 * loop() hanya bereaksi kalau ada kejadian; selebihnya bebas mengerjakan hal lain.
 *
 * Rangkaian:
 *   TOUCH (touch pad, header kuning) → IO33 (T8)
 *   LED1 (LED merah, header LED)     → IO4
 */

#define TOUCH_PIN T8  // T8 = IO33
#define LED_MERAH 4

const uint32_t PERSEN_AMBANG = 70;  // disentuh jika nilai < 70% nilai tanpa sentuhan

volatile bool adaKejadian = false;  // diubah oleh callback, dibaca oleh loop()
unsigned long waktuPesan = 0;       // untuk pesan berkala di loop()

// Callback: dipanggil driver touch saat pad disentuh atau dilepas.
// Perlakukan seperti ISR: singkat, tanpa Serial.print() dan delay().
void IRAM_ATTR saatPadBerubah() {
  adaKejadian = true;
}

// Rata-rata beberapa bacaan supaya derau teredam
uint32_t bacaRataRata(int jumlah) {
  uint32_t total = 0;
  for (int i = 0; i < jumlah; i++) {
    total += touchRead(TOUCH_PIN);
    delay(20);  // driver memperbarui nilai sekitar setiap 10 ms
  }
  return total / jumlah;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 14 - Program 4: Interrupt Sentuh");
  Serial.println("Kalibrasi... jangan sentuh pad selama 1 detik.");

  uint32_t nilaiLepas = bacaRataRata(50);
  uint32_t ambang = nilaiLepas * PERSEN_AMBANG / 100;

  Serial.print("Nilai tanpa sentuhan: ");
  Serial.print(nilaiLepas);
  Serial.print(" | ambang: ");
  Serial.println(ambang);

  // Daftarkan callback dengan ambang hasil kalibrasi
  touchAttachInterrupt(TOUCH_PIN, saatPadBerubah, ambang);
  Serial.println("Siap! Sentuh dan lepaskan pad.");
}

void loop() {
  if (adaKejadian) {
    adaKejadian = false;

    // Cari tahu: kejadian ini "disentuh" atau "dilepas"?
    if (touchInterruptGetLastStatus(TOUCH_PIN)) {
      digitalWrite(LED_MERAH, HIGH);
      Serial.println("Pad DISENTUH -> LED nyala");
    } else {
      digitalWrite(LED_MERAH, LOW);
      Serial.println("Pad DILEPAS  -> LED mati");
    }
  }

  // Contoh tugas lain: loop() tetap bebas bekerja tanpa memeriksa pad
  if (millis() - waktuPesan >= 5000) {
    waktuPesan = millis();
    Serial.println("(loop tetap berjalan...)");
  }
}
