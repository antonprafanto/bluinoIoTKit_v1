/*
 * BAB 16 - Program 2: LED Berkedip Tanpa delay()
 * LED merah berkedip setiap 1 detik memakai millis(),
 * sementara LED hijau langsung mengikuti tombol S1.
 *
 * Rangkaian:
 *   LED1 (merah)                → IO4
 *   LED2 (hijau)                → IO17
 *   Tombol S1 (pin kuning ke-3) → IO15
 */

#define LED_MERAH 4
#define LED_HIJAU 17
#define TOMBOL_S1 15

const unsigned long INTERVAL_KEDIP = 1000;  // ms: LED merah ganti keadaan setiap 1 detik

unsigned long waktuKedipLalu = 0;  // kapan LED merah terakhir ganti keadaan
bool merahNyala = false;           // keadaan LED merah saat ini

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(TOMBOL_S1, INPUT);

  Serial.println("BAB 16 - Program 2: LED Berkedip Tanpa delay()");
}

void loop() {
  unsigned long sekarang = millis();  // lirik jam sekali di awal loop()

  // Tugas 1: LED merah berkedip — hanya bertindak kalau sudah waktunya
  if (sekarang - waktuKedipLalu >= INTERVAL_KEDIP) {
    waktuKedipLalu = sekarang;          // catat waktu tindakan ini
    merahNyala = !merahNyala;           // balik keadaan LED
    digitalWrite(LED_MERAH, merahNyala ? HIGH : LOW);

    Serial.print("LED merah: ");
    Serial.println(merahNyala ? "NYALA" : "MATI");
  }

  // Tugas 2: LED hijau mengikuti S1 — diperiksa di SETIAP putaran loop()
  digitalWrite(LED_HIJAU, digitalRead(TOMBOL_S1));
}
