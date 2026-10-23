/*
 * BAB 22 - Program 5: Tombol Jarak Jauh lewat UART
 * Paket berformat <JENIS:NILAI> dikirim lewat Serial2:
 *   Tombol S1 ditekan → kirim <LED:1> atau <LED:0> (bergantian)
 *   Tombol S2 ditekan → kirim <BIP:2>
 * Paket yang DITERIMA lewat Serial2 dijalankan:
 *   <LED:1> / <LED:0> → LED biru menyala / mati
 *   <BIP:n>           → buzzer berbunyi n kali (n = 1-5)
 * Teks yang diketik di Serial Monitor diteruskan apa adanya ke Serial2.
 *
 * Varian A (1 board): kabel jumper 32 → VN (loopback)
 * Varian B (2 board): 32 board A → VN board B, 32 board B → VN board A, G ↔ G
 *
 * Rangkaian kit (header → pin ESP32):
 *   LED3 (biru) → IO2,  BUZZ → IO13,  SW1 → IO15,  SW2 → IO25
 */

#define RX2_PIN    39   // RX Serial2 → IO39 (label VN)
#define TX2_PIN    32   // TX Serial2 → IO32 (label 32)
#define BAUD2      9600

#define LED_BIRU   2
#define BUZZER_PIN 13
#define TOMBOL_S1  15
#define TOMBOL_S2  25

const unsigned long JEDA_DEBOUNCE = 50;   // ms
const unsigned long JEDA_BIP = 100;       // ms, lama bunyi dan lama diam

// Tombol: deteksi saat baru ditekan, dengan debounce sederhana
const int PIN_TOMBOL[2] = {TOMBOL_S1, TOMBOL_S2};
int keadaanTombolLalu[2] = {LOW, LOW};
unsigned long waktuTombolBerubah[2] = {0, 0};

// Penerima paket
char paket[16];            // isi paket di antara '<' dan '>'
int panjangPaket = 0;
bool dalamPaket = false;   // true setelah '<' diterima

// Keadaan lain
bool ledLawanNyala = false;       // perintah LED terakhir yang KITA kirim
int sisaBip = 0;                  // sisa bunyi yang harus dibunyikan buzzer
unsigned long waktuBipLalu = 0;

// Hasilnya true SEKALI setiap kali tombol ke-i baru saja ditekan
bool baruDitekan(int i) {
  int keadaan = digitalRead(PIN_TOMBOL[i]);
  if (keadaan != keadaanTombolLalu[i] && millis() - waktuTombolBerubah[i] >= JEDA_DEBOUNCE) {
    keadaanTombolLalu[i] = keadaan;
    waktuTombolBerubah[i] = millis();
    return keadaan == HIGH;            // kit: ditekan = HIGH
  }
  return false;
}

void kirimPaket(const char *jenis, int nilai) {
  Serial2.printf("<%s:%d>\n", jenis, nilai);
  Serial.printf("Kirim paket : <%s:%d>\n", jenis, nilai);
}

void jalankanPaket(char *isi) {
  // Pisahkan JENIS dan NILAI di tanda ':'
  char *titikDua = strchr(isi, ':');
  if (titikDua == NULL) {
    Serial.printf("Paket ditolak: <%s> (tidak ada tanda ':')\n", isi);
    return;
  }
  *titikDua = '\0';                    // sekarang 'isi' hanya berisi JENIS
  char *nilai = titikDua + 1;          // teks sesudah ':'

  if (strcmp(isi, "LED") == 0 && (strcmp(nilai, "0") == 0 || strcmp(nilai, "1") == 0)) {
    bool nyala = (nilai[0] == '1');
    digitalWrite(LED_BIRU, nyala ? HIGH : LOW);
    Serial.printf("Terima paket: <LED:%s> -> LED biru %s\n", nilai, nyala ? "NYALA" : "MATI");
  } else if (strcmp(isi, "BIP") == 0 && strlen(nilai) == 1 && nilai[0] >= '1' && nilai[0] <= '5') {
    sisaBip = nilai[0] - '0';          // karakter '3' → angka 3
    Serial.printf("Terima paket: <BIP:%s> -> buzzer bip %d kali\n", nilai, sisaBip);
  } else {
    Serial.printf("Paket ditolak: <%s:%s> (jenis atau nilai tidak dikenal)\n", isi, nilai);
  }
}

// Mengumpulkan karakter di antara '<' dan '>' tanpa menunggu
void terimaPaket() {
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    if (c == '<') {                        // awal paket: mulai dari nol
      dalamPaket = true;
      panjangPaket = 0;
    } else if (c == '>' && dalamPaket) {   // akhir paket: jalankan isinya
      paket[panjangPaket] = '\0';
      dalamPaket = false;
      jalankanPaket(paket);
    } else if (dalamPaket) {
      if (panjangPaket < (int)sizeof(paket) - 1) {
        paket[panjangPaket] = c;
        panjangPaket++;
      } else {                             // terlalu panjang: buang, tunggu '<' berikutnya
        dalamPaket = false;
        Serial.println("Paket ditolak: terlalu panjang");
      }
    }
    // karakter di luar <...> (misalnya \r, \n, atau gangguan) diabaikan
  }
}

// Membunyikan buzzer sebanyak sisaBip tanpa menunggu (pola BAB 16)
void urusBuzzer() {
  if (sisaBip == 0) return;
  if (millis() - waktuBipLalu < JEDA_BIP) return;
  waktuBipLalu = millis();
  if (digitalRead(BUZZER_PIN) == HIGH) {   // sedang berbunyi → diamkan
    digitalWrite(BUZZER_PIN, LOW);
    sisaBip--;                             // satu bip selesai
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD2, SERIAL_8N1, RX2_PIN, TX2_PIN);   // urutan: baud, format, RX, TX

  pinMode(LED_BIRU, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  // Kit sudah punya pull-down 10 kΩ. INPUT_PULLDOWN menjaga pin tetap LOW
  // kalau program ini dipakai di board kedua yang tidak memakai kit.
  pinMode(TOMBOL_S1, INPUT_PULLDOWN);
  pinMode(TOMBOL_S2, INPUT_PULLDOWN);

  Serial.println("BAB 22 - Program 5: Tombol Jarak Jauh lewat UART");
  Serial.println("S1: LED biru lawan nyala/mati, S2: buzzer lawan bip 2x");
  Serial.println("Bisa juga ketik paket, misalnya <LED:1>, lalu tekan Enter.");
}

void loop() {
  // Tugas 1: tombol ditekan → kirim paket
  if (baruDitekan(0)) {
    ledLawanNyala = !ledLawanNyala;
    kirimPaket("LED", ledLawanNyala ? 1 : 0);
  }
  if (baruDitekan(1)) {
    kirimPaket("BIP", 2);
  }

  // Tugas 2: teks dari Serial Monitor → diteruskan apa adanya ke Serial2
  while (Serial.available() > 0) {
    Serial2.write(Serial.read());
  }

  // Tugas 3: paket dari Serial2 → periksa dan jalankan
  terimaPaket();

  // Tugas 4: bunyikan buzzer kalau masih ada sisa bip
  urusBuzzer();
}
