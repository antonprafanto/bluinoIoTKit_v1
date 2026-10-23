/*
 * BAB 16 - Program 6: Perintah Serial Tanpa Menunggu
 * Ketik perintah di Serial Monitor (line ending: New Line):
 *   nyala  → LED merah menyala
 *   mati   → LED merah mati
 *   kedip  → LED merah berkedip setiap 300 ms
 *   status → tampilkan mode, keadaan LED, dan lama program berjalan
 *
 * Rangkaian: LED1 (merah) → IO4
 */

#define LED_MERAH 4

const unsigned long INTERVAL_KEDIP = 300;  // ms
const unsigned int PANJANG_MAKS = 20;      // batas panjang satu perintah

String penampung = "";  // karakter yang sudah tiba, menunggu akhir baris
bool modeKedip = false;
bool merahNyala = false;
unsigned long waktuKedipLalu = 0;

void aturLed(bool nyala) {
  merahNyala = nyala;
  digitalWrite(LED_MERAH, nyala ? HIGH : LOW);
}

void prosesPerintah(String perintah) {
  perintah.trim();                      // buang spasi di awal dan akhir
  perintah.toLowerCase();               // "NYALA" dianggap sama dengan "nyala"
  if (perintah.length() == 0) return;   // baris kosong: abaikan

  Serial.print("> ");
  Serial.println(perintah);

  if (perintah == "nyala") {
    modeKedip = false;
    aturLed(true);
    Serial.println("LED merah NYALA");
  } else if (perintah == "mati") {
    modeKedip = false;
    aturLed(false);
    Serial.println("LED merah MATI");
  } else if (perintah == "kedip") {
    modeKedip = true;
    Serial.println("LED merah BERKEDIP setiap 300 ms");
  } else if (perintah == "status") {
    Serial.print("Mode: ");
    Serial.print(modeKedip ? "kedip" : "tetap");
    Serial.print(" | LED: ");
    Serial.print(merahNyala ? "NYALA" : "MATI");
    Serial.print(" | Program berjalan: ");
    Serial.print(millis() / 1000);
    Serial.println(" detik");
  } else {
    Serial.println("Perintah tidak dikenal. Pilihan: nyala, mati, kedip, status");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  aturLed(false);
  penampung.reserve(PANJANG_MAKS);  // siapkan memori sekali di awal

  Serial.println("BAB 16 - Program 6: Perintah Serial Tanpa Menunggu");
  Serial.println("Ketik: nyala, mati, kedip, atau status (line ending: New Line)");
}

void loop() {
  // Tugas 1: ambil karakter yang SUDAH tiba saja, tanpa menunggu karakter berikutnya
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {      // akhir baris → perintah lengkap
      prosesPerintah(penampung);
      penampung = "";
    } else if (penampung.length() < PANJANG_MAKS) {
      penampung += c;                  // tambahkan karakter ke penampung
    }
  }

  // Tugas 2: mode kedip, memakai pola blink without delay
  unsigned long sekarang = millis();
  if (modeKedip && sekarang - waktuKedipLalu >= INTERVAL_KEDIP) {
    waktuKedipLalu = sekarang;
    aturLed(!merahNyala);
  }
}
