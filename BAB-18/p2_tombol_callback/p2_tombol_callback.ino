/*
 * BAB 18 - Program 2: Tombol Pintar dengan Callback
 * TombolPintar mengurus debounce dan memanggil callback saat tombol
 * ditekan dan saat dilepas. Aksinya ditentukan dari luar struct.
 *
 * Rangkaian:
 *   Tombol S1 (SW1, pin kuning ke-3) → IO15
 *   LED1 (merah)                     → IO4
 *   BUZZ (buzzer aktif)              → IO13
 */

#define TOMBOL_S1  15
#define LED_MERAH  4
#define BUZZER_PIN 13

// Tipe callback: fungsi tanpa parameter dan tanpa nilai kembali
typedef void (*CallbackTombol)();

// ============================================================
// TombolPintar: debounce (seperti BAB 9) + memanggil callback.
// Kode di dalam struct ini TIDAK tahu apa yang akan terjadi saat
// tombol ditekan — itu urusan fungsi yang didaftarkan dari luar.
// ============================================================
struct TombolPintar {
  int pin = -1;
  CallbackTombol saatDitekan = nullptr;  // callback: tombol baru ditekan
  CallbackTombol saatDilepas = nullptr;  // callback: tombol baru dilepas

  // Data internal untuk debounce
  int bacaanSebelumnya = LOW;
  int statusStabil = LOW;
  unsigned long waktuBerubah = 0;

  void begin(int nomorPin) {
    pin = nomorPin;
    pinMode(pin, INPUT);  // tombol kit: pull-down 10 kΩ di PCB, ditekan = HIGH
  }

  // Panggil sesering mungkin di loop()
  void update() {
    int bacaan = digitalRead(pin);
    if (bacaan != bacaanSebelumnya) {  // bacaan mentah berubah: ulang "stopwatch"
      waktuBerubah = millis();
      bacaanSebelumnya = bacaan;
    }
    if (millis() - waktuBerubah > 50 && bacaan != statusStabil) {
      statusStabil = bacaan;  // perubahan sudah stabil 50 ms: anggap sungguhan
      if (statusStabil == HIGH) {
        if (saatDitekan != nullptr) saatDitekan();  // event: tombol ditekan
      } else {
        if (saatDilepas != nullptr) saatDilepas();  // event: tombol dilepas
      }
    }
  }
};

// ============================================================
// Bip non-blocking: callback hanya MEMULAI bunyi,
// loop() yang mematikannya setelah LAMA_BIP.
// ============================================================
const unsigned long LAMA_BIP = 50;  // ms
bool bipAktif = false;
unsigned long waktuMulaiBip = 0;

void mulaiBip() {
  digitalWrite(BUZZER_PIN, HIGH);
  bipAktif = true;
  waktuMulaiBip = millis();
}

void urusBip() {
  if (bipAktif && millis() - waktuMulaiBip >= LAMA_BIP) {
    digitalWrite(BUZZER_PIN, LOW);
    bipAktif = false;
  }
}

// ============================================================
// Fungsi-fungsi callback (aksi)
// ============================================================
bool ledNyala = false;
int jumlahTekan = 0;

void toggleLed() {
  ledNyala = !ledNyala;
  digitalWrite(LED_MERAH, ledNyala ? HIGH : LOW);
  jumlahTekan++;
  Serial.print("Ditekan (ke-");
  Serial.print(jumlahTekan);
  Serial.print(") -> LED merah ");
  Serial.println(ledNyala ? "NYALA" : "MATI");
}

void bipSaatDilepas() {
  mulaiBip();
  Serial.println("Dilepas -> bip!");
}

TombolPintar tombolS1;

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  tombolS1.begin(TOMBOL_S1);
  tombolS1.saatDitekan = toggleLed;       // daftarkan callback (tanpa tanda kurung)
  tombolS1.saatDilepas = bipSaatDilepas;  // daftarkan callback kedua

  Serial.println("BAB 18 - Program 2: Tombol Pintar dengan Callback");
  Serial.println("Tekan S1: LED merah toggle | Lepas S1: bip");
}

void loop() {
  tombolS1.update();  // memeriksa tombol; memanggil callback kalau ada event
  urusBip();          // mematikan buzzer kalau waktu bip sudah habis
}
