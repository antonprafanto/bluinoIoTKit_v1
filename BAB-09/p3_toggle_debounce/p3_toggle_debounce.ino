/*
 * BAB 9 - Program 3: Sakelar Toggle dengan Debounce
 * Setiap kali S1 ditekan, LED merah berganti keadaan (nyala <-> mati).
 * Debounce memakai millis(), jadi satu tekanan = satu kali hitung.
 *
 * Rangkaian:
 *   Tombol S1 (pin kuning ke-3) → IO15
 *   LED1 (merah)                → IO4
 */

#define TOMBOL_S1 15
#define LED_MERAH 4

const unsigned long WAKTU_DEBOUNCE = 50;  // ms: bacaan harus stabil selama ini

int bacaanSebelumnya = LOW;      // bacaan mentah pada putaran loop() sebelumnya
int statusStabil = LOW;          // keadaan tombol yang sudah lolos debounce
unsigned long waktuBerubah = 0;  // kapan bacaan mentah terakhir kali berubah

bool ledNyala = false;  // keadaan LED: true = nyala, false = mati
int jumlahTekan = 0;

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);

  Serial.println("BAB 9 - Program 3: Sakelar Toggle dengan Debounce");
  Serial.println("Tekan S1 untuk menyalakan/mematikan LED merah.");
}

void loop() {
  int bacaan = digitalRead(TOMBOL_S1);

  // 1. Bacaan mentah berubah (karena ditekan, dilepas, atau memantul)?
  //    Catat waktunya: "stopwatch" diulang dari nol.
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = millis();
    bacaanSebelumnya = bacaan;
  }

  // 2. Bacaan tidak berubah lagi selama lebih dari WAKTU_DEBOUNCE
  //    dan berbeda dari status stabil → perubahan yang sungguhan.
  if ((millis() - waktuBerubah) > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;

    // 3. Edge detection: hanya bereaksi saat tombol baru ditekan (LOW → HIGH)
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
