/*
 * BAB 12 - Program 6: Fade Otomatis dengan Hardware
 * LED merah menerang dan meredup sendiri oleh hardware LEDC (ledcFade),
 * sementara loop() bebas mengedipkan LED biru setiap 250 ms.
 *
 * Rangkaian:
 *   LED1 (merah) → IO4
 *   LED3 (biru)  → IO2
 */

#define LED_MERAH 4
#define LED_BIRU  2

const uint32_t FREKUENSI = 5000;          // 5 kHz
const uint8_t RESOLUSI = 10;              // 10 bit → nilai duty 0–1023
const uint32_t DUTY_MAKS = 1023;
const int LAMA_FADE_MS = 2000;            // lama maksimum satu kali fade
const unsigned long JEDA_KEDIP_MS = 250;  // LED biru berganti keadaan setiap 250 ms

volatile bool fadeSelesai = false;  // diubah oleh fungsi callback, dibaca di loop()
bool arahNaik = true;               // arah fade berikutnya: true = makin terang
bool biruMenyala = false;
unsigned long waktuKedipTerakhir = 0;

// Dipanggil otomatis oleh LEDC saat fade selesai.
// Berjalan seperti ISR (BAB 10): harus singkat, tanpa Serial dan tanpa delay().
void IRAM_ATTR saatFadeSelesai() {
  fadeSelesai = true;
}

// Memulai fade baru sesuai arah saat ini
void mulaiFade() {
  bool berhasil;
  if (arahNaik) {
    berhasil = ledcFadeWithInterrupt(LED_MERAH, 0, DUTY_MAKS, LAMA_FADE_MS, saatFadeSelesai);
    Serial.println("Fade naik: 0 -> 1023");
  } else {
    berhasil = ledcFadeWithInterrupt(LED_MERAH, DUTY_MAKS, 0, LAMA_FADE_MS, saatFadeSelesai);
    Serial.println("Fade turun: 1023 -> 0");
  }
  if (!berhasil) {
    Serial.println("Gagal memulai fade!");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 12 - Program 6: Fade Otomatis dengan Hardware");

  pinMode(LED_BIRU, OUTPUT);  // LED biru memakai output digital biasa

  if (!ledcAttach(LED_MERAH, FREKUENSI, RESOLUSI)) {
    Serial.println("Gagal mengatur PWM di IO4! Periksa frekuensi dan resolusi.");
    while (true) {
      delay(1000);
    }
  }

  mulaiFade();  // fade pertama: makin terang
}

void loop() {
  // Tugas 1: kalau fade sudah selesai, mulai fade berikutnya dengan arah sebaliknya
  if (fadeSelesai) {
    fadeSelesai = false;
    arahNaik = !arahNaik;
    mulaiFade();
  }

  // Tugas 2: sementara itu, kedipkan LED biru tanpa delay()
  if (millis() - waktuKedipTerakhir >= JEDA_KEDIP_MS) {
    waktuKedipTerakhir = millis();
    biruMenyala = !biruMenyala;
    digitalWrite(LED_BIRU, biruMenyala);
  }
}
