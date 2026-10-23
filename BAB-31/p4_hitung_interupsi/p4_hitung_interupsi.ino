/*
 * BAB 31 - Program 4: Penghitung Gerakan dengan Interupsi
 * ISR menghitung setiap tepi naik keluaran PIR (gerakan mulai) dan mencatat
 * waktunya. Setiap 10 detik loop() membuat laporan, lalu pura-pura mengirim
 * data selama 4 detik. Gerakan selama loop() sibuk tetap terhitung.
 *
 * Rangkaian: PIR (header hijau, pin ke-5 dari atas) → IO33
 * Catatan  : IO33 satu-satunya pin berinterupsi di kelompok IO32–IO39
 *            (errata ESP32 GPIO-3.14, lihat BAB 10).
 */

#define PIN_PIR  33
#define LED_BIRU 2  // LED biru bawaan board: menyala saat "mengirim"

const unsigned long INTERVAL_LAPORAN = 10000;  // ms: buat laporan setiap 10 detik
const unsigned long LAMA_KIRIM       = 4000;   // ms: pura-pura mengirim data (lambat)

// Variabel bersama ISR <-> loop(): wajib volatile
volatile unsigned long jumlahGerakan = 0;         // gerakan sejak laporan terakhir
volatile unsigned long waktuGerakanTerakhir = 0;  // kapan gerakan terakhir mulai (ms)

// Kunci bagian kritis untuk kedua variabel di atas
portMUX_TYPE kunciPir = portMUX_INITIALIZER_UNLOCKED;

// Variabel yang hanya dipakai loop()
unsigned long totalGerakan = 0;
unsigned long nomorLaporan = 0;
unsigned long waktuLaporanLalu = 0;

// ISR: dipanggil hardware setiap keluaran PIR naik dari LOW ke HIGH
void IRAM_ATTR isrPir() {
  portENTER_CRITICAL_ISR(&kunciPir);
  jumlahGerakan = jumlahGerakan + 1;
  waktuGerakanTerakhir = millis();
  portEXIT_CRITICAL_ISR(&kunciPir);
}

// Tugas lambat: loop() tidak memeriksa PIR sama sekali selama ini
void kirimLaporan() {
  Serial.println("  Mengirim laporan... (loop() sibuk 4 detik)");
  digitalWrite(LED_BIRU, HIGH);
  delay(LAMA_KIRIM);
  digitalWrite(LED_BIRU, LOW);
  Serial.println("  Selesai mengirim.");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  pinMode(LED_BIRU, OUTPUT);

  // RISING: ISR dipanggil saat gerakan mulai terdeteksi
  attachInterrupt(digitalPinToInterrupt(PIN_PIR), isrPir, RISING);

  Serial.println("BAB 31 - Program 4: Penghitung Gerakan dengan Interupsi");
  Serial.println("Laporan setiap 10 detik. Saat LED biru menyala, loop() sedang sibuk.");
}

void loop() {
  unsigned long sekarang = millis();
  if (sekarang - waktuLaporanLalu < INTERVAL_LAPORAN) {
    return;  // belum waktunya melapor
  }
  waktuLaporanLalu = sekarang;

  // Ambil catatan ISR lalu kosongkan, tanpa bisa disela ISR di tengah-tengahnya
  portENTER_CRITICAL(&kunciPir);
  unsigned long jumlah = jumlahGerakan;
  unsigned long waktuTerakhir = waktuGerakanTerakhir;
  jumlahGerakan = 0;
  portEXIT_CRITICAL(&kunciPir);

  totalGerakan = totalGerakan + jumlah;
  nomorLaporan = nomorLaporan + 1;

  Serial.print("Laporan #");
  Serial.print(nomorLaporan);
  Serial.print(" (detik ke-");
  Serial.print(sekarang / 1000.0, 1);
  Serial.print("): ");
  Serial.print(jumlah);
  Serial.print(" gerakan baru, total ");
  Serial.println(totalGerakan);
  if (jumlah > 0) {
    Serial.print("  Gerakan terakhir mulai pada detik ke-");
    Serial.println(waktuTerakhir / 1000.0, 1);
  }

  kirimLaporan();
}
