/*
 * BAB 18 - Program 6: Interrupt dan Callback yang Aman
 * S1 dan S2 memakai interrupt. Satu fungsi ISR melayani kedua tombol
 * lewat attachInterruptArg(). ISR hanya mencatat; debounce dan
 * callback dijalankan di loop().
 *
 * Rangkaian:
 *   Tombol S1 (SW1, pin kuning ke-3) → IO15
 *   Tombol S2 (SW2, pin kuning ke-4) → IO25
 *   LED1 (merah)                     → IO4
 *   BUZZ (buzzer aktif)              → IO13
 */

#include <functional>

#define TOMBOL_S1  15
#define TOMBOL_S2  25
#define LED_MERAH  4
#define BUZZER_PIN 13

const unsigned long WAKTU_TENANG = 50;  // ms tanpa interrupt baru = sinyal sudah stabil

// ============================================================
// Data satu tombol. Anggota volatile diisi oleh ISR.
// ============================================================
struct TombolInterrupt {
  int pin;
  std::function<void()> saatDitekan;  // callback, dijalankan di loop()

  volatile bool adaPerubahan = false;          // bendera dari ISR
  volatile unsigned long waktuPerubahan = 0;   // kapan interrupt terakhir terjadi
  volatile unsigned long jumlahInterrupt = 0;  // berapa kali ISR dipanggil
  int statusStabil = LOW;                      // hanya dipakai di loop()
};

TombolInterrupt tombolS1 = {TOMBOL_S1};
TombolInterrupt tombolS2 = {TOMBOL_S2};

// ============================================================
// SATU fungsi ISR untuk kedua tombol.
// "arg" berisi alamat data tombol yang memicu interrupt.
// ============================================================
void IRAM_ATTR isrTombol(void *arg) {
  TombolInterrupt *t = (TombolInterrupt *)arg;
  t->adaPerubahan = true;  // cukup mencatat — singkat!
  t->waktuPerubahan = millis();
  t->jumlahInterrupt = t->jumlahInterrupt + 1;  // bukan ++ (usang untuk volatile di C++20)
}

// ============================================================
// Dipanggil di loop() untuk setiap tombol: debounce + callback
// ============================================================
void periksaTombol(TombolInterrupt &t) {
  if (!t.adaPerubahan) return;  // belum ada interrupt baru

  unsigned long waktu = t.waktuPerubahan;       // salin dulu: ISR bisa mengubahnya kapan saja
  if (millis() - waktu < WAKTU_TENANG) return;  // sinyal belum tenang selama 50 ms

  t.adaPerubahan = false;           // turunkan bendera DULU,
  int status = digitalRead(t.pin);  // baru baca keadaan tombol yang sudah stabil
  if (status == t.statusStabil) return;
  t.statusStabil = status;

  if (status == HIGH && t.saatDitekan) {
    t.saatDitekan();  // berjalan di loop(), bukan di ISR: boleh Serial, bip, dan lain-lain
  }
}

// ============================================================
// Bip non-blocking (sama seperti Program 4)
// ============================================================
const unsigned long LAMA_BUNYI = 80;  // ms
int sisaBip = 0;
bool buzzerBunyi = false;
unsigned long waktuBuzzer = 0;

void mulaiBip(int kali) {
  sisaBip = kali;
  buzzerBunyi = true;
  digitalWrite(BUZZER_PIN, HIGH);
  waktuBuzzer = millis();
}

void urusBuzzer() {
  if (sisaBip == 0) return;
  if (millis() - waktuBuzzer < LAMA_BUNYI) return;
  waktuBuzzer = millis();
  if (buzzerBunyi) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerBunyi = false;
    sisaBip--;
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerBunyi = true;
  }
}

bool ledNyala = false;

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);  // tombol kit: pull-down 10 kΩ di PCB
  pinMode(TOMBOL_S2, INPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Daftarkan callback — nanti dipanggil dari loop(), jadi boleh memakai Serial
  tombolS1.saatDitekan = []() {
    ledNyala = !ledNyala;
    digitalWrite(LED_MERAH, ledNyala ? HIGH : LOW);
    Serial.print("[S1] LED merah ");
    Serial.print(ledNyala ? "NYALA" : "MATI");
    Serial.print(" | total interrupt S1: ");
    Serial.println(tombolS1.jumlahInterrupt);
  };
  tombolS2.saatDitekan = []() {
    mulaiBip(2);
    Serial.print("[S2] bip 2 kali");
    Serial.print(" | total interrupt S2: ");
    Serial.println(tombolS2.jumlahInterrupt);
  };

  // Satu ISR untuk dua tombol: argumen ketiga = alamat data tombolnya
  attachInterruptArg(digitalPinToInterrupt(TOMBOL_S1), isrTombol, &tombolS1, CHANGE);
  attachInterruptArg(digitalPinToInterrupt(TOMBOL_S2), isrTombol, &tombolS2, CHANGE);

  Serial.println("BAB 18 - Program 6: Interrupt dan Callback yang Aman");
  Serial.println("S1 = toggle LED merah, S2 = bip 2 kali");
}

void loop() {
  periksaTombol(tombolS1);
  periksaTombol(tombolS2);
  urusBuzzer();
}
