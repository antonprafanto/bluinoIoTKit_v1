/*
 * BAB 38 - Program 2: Kalibrasi Titik Ujung Servo (Tanpa Library)
 * Mengirim lebar pulsa pilihanmu ke servo memakai LEDC secara langsung.
 * Ketik di Serial Monitor (line ending: New Line), lalu tekan Enter:
 *   angka 500-2500 → kirim pulsa selebar itu (dalam µs), misalnya 1500
 *   +              → tambah 10 µs
 *   -              → kurangi 10 µs
 *
 * Rangkaian: SRV (header hijau) → IO32, servo di header servo kit (S V G)
 */

#define PIN_SERVO 32  // SRV → IO32

const uint32_t FREKUENSI_HZ = 50;      // 50 Hz → satu pulsa setiap 20 ms
const uint8_t RESOLUSI_BIT = 14;       // 2^14 = 16384 langkah per periode
const uint32_t PERIODE_US = 20000;     // 1 ÷ 50 Hz = 20.000 µs
const uint32_t LANGKAH_PENUH = 16384;  // 2^RESOLUSI_BIT

const int BATAS_BAWAH_US = 500;  // rentang aman yang boleh diketik
const int BATAS_ATAS_US = 2500;
const int LANGKAH_US = 10;       // besar perubahan untuk perintah + dan -

int pulsaUs = 1500;     // lebar pulsa sekarang; mulai di tengah
String penampung = "";  // karakter yang sudah tiba, menunggu akhir baris

// Mengubah lebar pulsa (µs) menjadi nilai duty LEDC 14 bit, dibulatkan
uint32_t pulsaKeDuty(int us) {
  return ((uint32_t)us * LANGKAH_PENUH + PERIODE_US / 2) / PERIODE_US;
}

void kirimPulsa(int us) {
  pulsaUs = constrain(us, BATAS_BAWAH_US, BATAS_ATAS_US);
  uint32_t duty = pulsaKeDuty(pulsaUs);
  ledcWrite(PIN_SERVO, duty);
  Serial.printf("Pulsa: %4d us | duty: %4lu dari %lu (%.2f%%)\n", pulsaUs,
                (unsigned long)duty, (unsigned long)LANGKAH_PENUH,
                duty * 100.0 / LANGKAH_PENUH);
}

void prosesPerintah(String perintah) {
  perintah.trim();                     // buang spasi dan karakter '\r'
  if (perintah.length() == 0) return;  // baris kosong: abaikan

  Serial.print("> ");
  Serial.println(perintah);

  if (perintah == "+") {
    kirimPulsa(pulsaUs + LANGKAH_US);
  } else if (perintah == "-") {
    kirimPulsa(pulsaUs - LANGKAH_US);
  } else {
    int angka = perintah.toInt();  // teks yang bukan angka menjadi 0
    if (angka >= BATAS_BAWAH_US && angka <= BATAS_ATAS_US) {
      kirimPulsa(angka);
    } else {
      Serial.println("Ketik angka 500-2500, atau + atau -");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 38 - Program 2: Kalibrasi Titik Ujung Servo");

  // IO32 menjadi PWM 50 Hz dengan resolusi 14 bit
  if (!ledcAttach(PIN_SERVO, FREKUENSI_HZ, RESOLUSI_BIT)) {
    Serial.println("Gagal mengatur PWM di IO32!");
    while (true) {
      delay(1000);  // program berhenti di sini
    }
  }

  Serial.println("Ketik lebar pulsa 500-2500 (us), atau + / -, lalu tekan Enter.");
  kirimPulsa(pulsaUs);  // mulai dari tengah (1500 µs)
}

void loop() {
  // Ambil hanya karakter yang sudah tiba, tanpa menunggu (pola BAB 16)
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      prosesPerintah(penampung);
      penampung = "";
    } else if (penampung.length() < 10) {
      penampung += c;
    }
  }
}
