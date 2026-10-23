/*
 * BAB 16 - Program 3: Beberapa Tugas Sekaligus
 * Tugas 1: LED merah ganti keadaan setiap 500 ms
 * Tugas 2: potensiometer dibaca setiap 50 ms
 * Tugas 3: LED kuning berkedip, intervalnya 100–1000 ms sesuai potensiometer
 * Tugas 4: laporan ke Serial Monitor setiap 1 detik
 * Tugas 5: LED hijau mengikuti tombol S1 di setiap putaran loop()
 *
 * Rangkaian:
 *   LED1 (merah)                          → IO4
 *   LED2 (hijau)                          → IO17
 *   LED4 (kuning)                         → IO16
 *   Tombol S1 (pin kuning ke-3)           → IO15
 *   Potensiometer VR10K (pin kuning ke-5) → IO34
 */

#define LED_MERAH  4
#define LED_HIJAU  17
#define LED_KUNING 16
#define TOMBOL_S1  15
#define POT_PIN    34

// Interval tugas yang tetap (ms)
const unsigned long INTERVAL_MERAH   = 500;
const unsigned long INTERVAL_POT     = 50;
const unsigned long INTERVAL_LAPORAN = 1000;

// "Jam weker" setiap tugas: kapan terakhir dijalankan
unsigned long waktuMerahLalu   = 0;
unsigned long waktuPotLalu     = 0;
unsigned long waktuKuningLalu  = 0;
unsigned long waktuLaporanLalu = 0;

bool merahNyala  = false;
bool kuningNyala = false;
int nilaiPot = 0;                    // hasil analogRead() terakhir (0–4095)
unsigned long intervalKuning = 500;  // berubah mengikuti potensiometer

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(TOMBOL_S1, INPUT);
  // POT_PIN (IO34) cukup dibaca dengan analogRead(), tidak perlu pinMode()

  Serial.println("BAB 16 - Program 3: Beberapa Tugas Sekaligus");
  Serial.println("Putar potensiometer untuk mengubah kecepatan LED kuning.");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: LED merah — ganti keadaan setiap 500 ms
  if (sekarang - waktuMerahLalu >= INTERVAL_MERAH) {
    waktuMerahLalu = sekarang;
    merahNyala = !merahNyala;
    digitalWrite(LED_MERAH, merahNyala ? HIGH : LOW);
  }

  // Tugas 2: baca potensiometer setiap 50 ms, lalu hitung interval LED kuning
  if (sekarang - waktuPotLalu >= INTERVAL_POT) {
    waktuPotLalu = sekarang;
    nilaiPot = analogRead(POT_PIN);
    intervalKuning = map(nilaiPot, 0, 4095, 100, 1000);
  }

  // Tugas 3: LED kuning — ganti keadaan sesuai interval dari potensiometer
  if (sekarang - waktuKuningLalu >= intervalKuning) {
    waktuKuningLalu = sekarang;
    kuningNyala = !kuningNyala;
    digitalWrite(LED_KUNING, kuningNyala ? HIGH : LOW);
  }

  // Tugas 4: laporan ke Serial Monitor setiap 1 detik
  if (sekarang - waktuLaporanLalu >= INTERVAL_LAPORAN) {
    waktuLaporanLalu = sekarang;
    Serial.print("Potensiometer: ");
    Serial.print(nilaiPot);
    Serial.print(" | Interval LED kuning: ");
    Serial.print(intervalKuning);
    Serial.println(" ms");
  }

  // Tugas 5: LED hijau mengikuti S1 — di setiap putaran, tanpa jadwal
  digitalWrite(LED_HIJAU, digitalRead(TOMBOL_S1));
}
