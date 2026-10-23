/*
 * BAB 14 - Program 3: Sakelar Sentuh (Toggle LED)
 * Sentuh sekali = LED menyala, sentuh lagi = LED mati.
 * Ambang dihitung otomatis dari nilai tanpa sentuhan, memakai histeresis.
 *
 * Rangkaian:
 *   TOUCH (touch pad, header kuning) → IO33 (T8)
 *   LED1 (LED merah, header LED)     → IO4
 */

#define TOUCH_PIN T8  // T8 = IO33
#define LED_MERAH 4

// Ambang dalam persen dari nilai tanpa sentuhan (sesuaikan dengan hasil Program 2)
const uint32_t PERSEN_SENTUH = 70;  // nilai di bawah 70% → dianggap disentuh
const uint32_t PERSEN_LEPAS  = 85;  // nilai di atas 85% → dianggap dilepas

uint32_t ambangSentuh = 0;
uint32_t ambangLepas  = 0;

bool disentuh = false;   // status pad saat ini
bool ledNyala = false;   // status LED saat ini
int jumlahSentuhan = 0;

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

  Serial.println("BAB 14 - Program 3: Sakelar Sentuh");
  Serial.println("Kalibrasi... jangan sentuh pad selama 1 detik.");

  uint32_t nilaiLepas = bacaRataRata(50);
  ambangSentuh = nilaiLepas * PERSEN_SENTUH / 100;
  ambangLepas  = nilaiLepas * PERSEN_LEPAS / 100;

  Serial.print("Nilai tanpa sentuhan: ");
  Serial.print(nilaiLepas);
  Serial.print(" | ambang sentuh: ");
  Serial.print(ambangSentuh);
  Serial.print(" | ambang lepas: ");
  Serial.println(ambangLepas);
  Serial.println("Siap! Sentuh pad untuk menyalakan/mematikan LED.");
}

void loop() {
  uint32_t nilai = touchRead(TOUCH_PIN);

  if (!disentuh && nilai < ambangSentuh) {
    // Momen pad MULAI disentuh (tepi) → balik keadaan LED
    disentuh = true;
    ledNyala = !ledNyala;
    digitalWrite(LED_MERAH, ledNyala ? HIGH : LOW);

    jumlahSentuhan++;
    Serial.print("Sentuhan ke-");
    Serial.print(jumlahSentuhan);
    Serial.print(": LED ");
    Serial.println(ledNyala ? "NYALA" : "MATI");
  } else if (disentuh && nilai > ambangLepas) {
    // Jari sudah benar-benar lepas → siap menerima sentuhan berikutnya
    disentuh = false;
  }
  // Nilai di antara kedua ambang: status tidak berubah (histeresis)

  delay(20);
}
