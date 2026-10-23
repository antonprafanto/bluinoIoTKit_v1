/*
 * BAB 21 - Program 1: Logger dengan Level
 * Memantau "level air tangki" (disimulasikan dengan potensiometer) dan
 * mencetak pesan ERROR/WARN/INFO/DEBUG. Pesan yang tampil diatur LEVEL_LOG.
 *
 * Rangkaian: VR10K (potensiometer) → IO34
 * Alat uji : Serial Monitor, baud 115200
 */

// ----- Level log: makin besar angkanya, makin banyak pesan yang tampil -----
#define LEVEL_TIDAK_ADA 0
#define LEVEL_ERROR     1
#define LEVEL_WARN      2
#define LEVEL_INFO      3
#define LEVEL_DEBUG     4

#define LEVEL_LOG LEVEL_INFO  // ← ubah di sini: LEVEL_DEBUG, LEVEL_WARN, LEVEL_TIDAK_ADA, ...

// Mencetak awalan setiap pesan log, misalnya: [2000 ms][INFO ]
void awalanLog(const char *namaLevel) {
  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms][");
  Serial.print(namaLevel);
  Serial.print("] ");
}

// Makro log. "..." menerima berapa pun parameter, lalu diteruskan ke Serial.printf().
// do { ... } while (0) membuat makro aman dipakai di dalam if-else.
#define LOG_ERROR(...) do { if (LEVEL_LOG >= LEVEL_ERROR) { awalanLog("ERROR"); Serial.printf(__VA_ARGS__); Serial.println(); } } while (0)
#define LOG_WARN(...)  do { if (LEVEL_LOG >= LEVEL_WARN)  { awalanLog("WARN "); Serial.printf(__VA_ARGS__); Serial.println(); } } while (0)
#define LOG_INFO(...)  do { if (LEVEL_LOG >= LEVEL_INFO)  { awalanLog("INFO "); Serial.printf(__VA_ARGS__); Serial.println(); } } while (0)
#define LOG_DEBUG(...) do { if (LEVEL_LOG >= LEVEL_DEBUG) { awalanLog("DEBUG"); Serial.printf(__VA_ARGS__); Serial.println(); } } while (0)

#define PIN_POT 34  // VR10K (potensiometer) → IO34

const unsigned long INTERVAL_BACA = 2000;  // ms: baca "tangki" setiap 2 detik
unsigned long waktuBaca = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BAB 21 - Program 1: Logger dengan Level");  // selalu tampil
  LOG_INFO("Level log aktif: %d (1=ERROR, 2=WARN, 3=INFO, 4=DEBUG)", LEVEL_LOG);
  LOG_DEBUG("Potensiometer dibaca dari IO%d", PIN_POT);
}

void loop() {
  if (millis() - waktuBaca < INTERVAL_BACA) {
    return;  // belum waktunya membaca
  }
  waktuBaca = millis();

  int mentah = analogRead(PIN_POT);           // 0 sampai 4095
  int persen = map(mentah, 0, 4095, 0, 100);  // ubah ke 0 sampai 100 %

  LOG_DEBUG("ADC mentah = %d, tegangan = %d mV", mentah, (int)analogReadMilliVolts(PIN_POT));
  LOG_INFO("Level tangki: %d %%", persen);

  if (persen < 10) {
    LOG_ERROR("Tangki hampir kosong (%d %%)! Segera isi ulang.", persen);
  } else if (persen < 25) {
    LOG_WARN("Tangki tinggal sedikit (%d %%)", persen);
  }
}
