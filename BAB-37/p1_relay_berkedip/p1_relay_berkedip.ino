/*
 * BAB 37 - Program 1: Relay Berkedip Tanpa delay()
 * Relay aktif 2 detik, lalu mati 2 detik, berulang terus.
 * Waktu diatur dengan millis(), jadi loop() tidak pernah menunggu.
 *
 * Rangkaian:
 *   RELAY (header biru, kanan BUZZ) → IO26 (aktif HIGH, lewat transistor BC547)
 *   Beban (opsional)                → terminal COM dan NO (lihat 37.7)
 */

#define RELAY_PIN 26  // RELAY → IO26

const unsigned long LAMA_AKTIF = 2000;  // ms: relay aktif selama 2 detik
const unsigned long LAMA_MATI  = 2000;  // ms: relay mati selama 2 detik

bool relayAktif = false;         // keadaan relay saat ini
unsigned long waktuBerubah = 0;  // kapan relay terakhir berganti keadaan

// Satu-satunya tempat yang menyentuh pin relay
void aturRelay(bool aktif) {
  relayAktif = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);  // di core 3.x, pinMode() wajib dipanggil dulu
  aturRelay(false);            // lalu langsung pastikan relay mati

  Serial.println("BAB 37 - Program 1: Relay Berkedip Tanpa delay()");
  Serial.println("Relay aktif 2 detik, mati 2 detik, berulang.");
  Serial.println("Keadaan awal: relay OFF");
  waktuBerubah = millis();
}

void loop() {
  unsigned long sekarang = millis();  // lirik jam sekali di awal loop()

  // Lama keadaan sekarang: 2 detik kalau sedang aktif, 2 detik kalau sedang mati
  unsigned long lamaKeadaan = relayAktif ? LAMA_AKTIF : LAMA_MATI;

  if (sekarang - waktuBerubah >= lamaKeadaan) {
    waktuBerubah = sekarang;  // catat waktu pergantian ini
    aturRelay(!relayAktif);   // balik keadaan: aktif ↔ mati

    Serial.printf("[%6.1f s] Relay %s\n", sekarang / 1000.0,
                  relayAktif ? "ON  -> COM tersambung ke NO" : "OFF -> COM terputus dari NO");
  }

  // Tugas lain bisa ditambahkan di sini, tanpa terganggu oleh relay
}
