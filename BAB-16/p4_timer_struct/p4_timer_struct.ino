/*
 * BAB 16 - Program 4: Empat LED dengan Interval Berbeda
 * Setiap LED punya timer sendiri, disimpan dalam array berisi struct.
 * Setiap 5 detik, Serial Monitor menampilkan berapa kali setiap LED
 * sudah berganti keadaan.
 *
 * Rangkaian (header LED di kit → pin ESP32):
 *   LED1 (merah) → IO4,  LED2 (hijau)  → IO17
 *   LED3 (biru)  → IO2,  LED4 (kuning) → IO16
 */

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2

// Timer serbaguna: interval + catatan waktu + fungsi pemeriksa
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

// Satu LED berkedip = pin + nama + timer + keadaan + penghitung
struct LedKedip {
  int pin;
  const char *nama;
  Timer timer;
  bool nyala;
  int jumlahGanti;  // berapa kali LED ini sudah berganti keadaan
};

LedKedip daftarLed[] = {
  // pin,      nama,     {interval, waktuLalu}, nyala, jumlahGanti
  {LED_MERAH,  "merah",  {250, 0},              false, 0},
  {LED_HIJAU,  "hijau",  {500, 0},              false, 0},
  {LED_BIRU,   "biru",   {1000, 0},             false, 0},
  {LED_KUNING, "kuning", {2000, 0},             false, 0},
};
const int JUMLAH_LED = sizeof(daftarLed) / sizeof(daftarLed[0]);  // = 4

Timer timerLaporan = {5000, 0};  // laporan setiap 5 detik
unsigned long waktuMulai = 0;    // kapan semua timer mulai menghitung

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(daftarLed[i].pin, OUTPUT);
    digitalWrite(daftarLed[i].pin, LOW);
  }

  // Semua timer mulai menghitung dari saat yang sama
  waktuMulai = millis();
  for (int i = 0; i < JUMLAH_LED; i++) {
    daftarLed[i].timer.waktuLalu = waktuMulai;
  }
  timerLaporan.waktuLalu = waktuMulai;

  Serial.println("BAB 16 - Program 4: Empat LED dengan Interval Berbeda");
}

void loop() {
  unsigned long sekarang = millis();

  // Satu perulangan untuk semua LED
  for (int i = 0; i < JUMLAH_LED; i++) {
    if (daftarLed[i].timer.sudahWaktunya(sekarang)) {
      daftarLed[i].nyala = !daftarLed[i].nyala;
      digitalWrite(daftarLed[i].pin, daftarLed[i].nyala ? HIGH : LOW);
      daftarLed[i].jumlahGanti++;
    }
  }

  // Laporan setiap 5 detik
  if (timerLaporan.sudahWaktunya(sekarang)) {
    Serial.print("Detik ke-");
    Serial.print((sekarang - waktuMulai) / 1000);
    for (int i = 0; i < JUMLAH_LED; i++) {
      Serial.print(" | ");
      Serial.print(daftarLed[i].nama);
      Serial.print(": ");
      Serial.print(daftarLed[i].jumlahGanti);
      Serial.print("x");
    }
    Serial.println();
  }
}
