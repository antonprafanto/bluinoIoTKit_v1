/*
 * BAB 22 - Program 4: Uji Loopback Serial2
 * Setiap 1 detik ESP32 mengirim "PING <nomor>" lewat TX Serial2 (IO32).
 * Satu kabel jumper membawa sinyal itu kembali ke RX Serial2 (IO39/VN),
 * lalu program membandingkan teks yang diterima dengan yang dikirim.
 *
 * Rangkaian: 1 kabel jumper dari label 32 ke label VN (deretan atas).
 *            Cabut dulu kabel SRV → 32 kalau ada.
 * Alat uji : Serial Monitor, baud 115200
 */

#define RX2_PIN 39       // RX Serial2 → IO39 (label VN), pin input saja
#define TX2_PIN 32       // TX Serial2 → IO32 (label 32)
#define BAUD2   9600     // baud rate Serial2 (boleh berbeda dari Serial)

const unsigned long INTERVAL_KIRIM = 1000;   // ms

unsigned long waktuKirimLalu = 0;
unsigned long nomorPesan = 0;
bool menungguBalasan = false;   // sudah mengirim, tetapi balasannya belum datang

char pesanTerakhir[16];         // pesan terakhir yang dikirim, untuk dibandingkan
char balasan[32];               // tempat mengumpulkan baris yang diterima
int panjangBalasan = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD2, SERIAL_8N1, RX2_PIN, TX2_PIN);   // urutan: baud, format, RX, TX

  Serial.println("BAB 22 - Program 4: Uji Loopback Serial2");
  Serial.printf("Serial2: %d baud, RX = IO%d, TX = IO%d\n", BAUD2, RX2_PIN, TX2_PIN);
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: setiap 1 detik, kirim satu pesan lewat Serial2
  if (sekarang - waktuKirimLalu >= INTERVAL_KIRIM) {
    waktuKirimLalu = sekarang;
    if (menungguBalasan) {
      Serial.println("  -> Tidak ada balasan! Periksa kabel 32 -> VN.");
    }
    nomorPesan++;
    snprintf(pesanTerakhir, sizeof(pesanTerakhir), "PING %lu", nomorPesan);
    Serial2.println(pesanTerakhir);           // terkirim: teks + \r\n
    Serial.printf("Kirim  : %s\n", pesanTerakhir);
    menungguBalasan = true;
  }

  // Tugas 2: kumpulkan byte yang tiba di RX Serial2, baris demi baris
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    if (c == '\r') continue;                  // abaikan CR dari println()
    if (c == '\n') {                          // satu baris lengkap
      balasan[panjangBalasan] = '\0';
      panjangBalasan = 0;
      if (strcmp(balasan, pesanTerakhir) == 0) {
        Serial.printf("Terima : %s  -> OK, sama persis\n", balasan);
        menungguBalasan = false;
      } else {
        Serial.printf("Terima : %s  -> BEDA dengan yang dikirim!\n", balasan);
      }
    } else if (panjangBalasan < (int)sizeof(balasan) - 1) {
      balasan[panjangBalasan] = c;
      panjangBalasan++;
    }
  }
}
