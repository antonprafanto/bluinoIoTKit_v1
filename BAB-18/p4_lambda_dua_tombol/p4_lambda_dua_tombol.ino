/*
 * BAB 18 - Program 4: Lambda untuk Dua Tombol
 * Struct Tombol yang sama dipakai untuk S1 dan S2. Callback-nya disimpan
 * di std::function dan ditulis sebagai lambda; satu lambda membawa data (capture).
 *
 * Rangkaian:
 *   Tombol S1 (SW1, pin kuning ke-3) → IO15
 *   Tombol S2 (SW2, pin kuning ke-4) → IO25
 *   LED1 (merah)                     → IO4
 *   BUZZ (buzzer aktif)              → IO13
 */

#include <functional>  // std::function

#define TOMBOL_S1  15
#define TOMBOL_S2  25
#define LED_MERAH  4
#define BUZZER_PIN 13

using Callback = std::function<void()>;  // wadah untuk fungsi apa pun berbentuk void()

// ============================================================
// Tombol: sama dengan TombolPintar di Program 2, tetapi callback-nya
// disimpan di std::function sehingga bisa diisi lambda ber-capture.
// ============================================================
struct Tombol {
  int pin = -1;
  Callback saatDitekan;  // kosong sampai diisi

  int bacaanSebelumnya = LOW;
  int statusStabil = LOW;
  unsigned long waktuBerubah = 0;

  void begin(int nomorPin) {
    pin = nomorPin;
    pinMode(pin, INPUT);  // tombol kit: ditekan = HIGH
  }

  void update() {
    int bacaan = digitalRead(pin);
    if (bacaan != bacaanSebelumnya) {
      waktuBerubah = millis();
      bacaanSebelumnya = bacaan;
    }
    if (millis() - waktuBerubah > 50 && bacaan != statusStabil) {
      statusStabil = bacaan;
      if (statusStabil == HIGH && saatDitekan) {  // std::function bernilai true kalau sudah diisi
        saatDitekan();
      }
    }
  }
};

// ============================================================
// Bip non-blocking: bunyi beberapa kali, setiap bunyi dan jeda 80 ms
// ============================================================
const unsigned long LAMA_BUNYI = 80;  // ms
int sisaBip = 0;                      // sisa bunyi yang belum selesai
bool buzzerBunyi = false;
unsigned long waktuBuzzer = 0;

void mulaiBip(int kali) {
  sisaBip = kali;
  buzzerBunyi = true;
  digitalWrite(BUZZER_PIN, HIGH);  // bunyi pertama langsung dimulai
  waktuBuzzer = millis();
}

void urusBuzzer() {  // dipanggil di setiap putaran loop()
  if (sisaBip == 0) return;
  if (millis() - waktuBuzzer < LAMA_BUNYI) return;
  waktuBuzzer = millis();
  if (buzzerBunyi) {  // satu bunyi selesai → diam
    digitalWrite(BUZZER_PIN, LOW);
    buzzerBunyi = false;
    sisaBip--;
  } else {            // jeda selesai → bunyi lagi
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerBunyi = true;
  }
}

// ============================================================
// "Pabrik" callback: membuat lambda yang membawa salinan nama dan kali
// ============================================================
Callback buatAksiBip(const char *nama, int kali) {
  return [nama, kali]() {  // capture by value: lambda menyimpan salinannya sendiri
    Serial.print("[");
    Serial.print(nama);
    Serial.print("] Bip ");
    Serial.print(kali);
    Serial.println(" kali");
    mulaiBip(kali);
  };
}

bool ledNyala = false;
Tombol tombolS1;
Tombol tombolS2;

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  tombolS1.begin(TOMBOL_S1);
  tombolS2.begin(TOMBOL_S2);

  // S1: lambda tanpa capture, ditulis langsung di tempat
  tombolS1.saatDitekan = []() {
    ledNyala = !ledNyala;  // variabel global: boleh dipakai tanpa capture
    digitalWrite(LED_MERAH, ledNyala ? HIGH : LOW);
    Serial.print("[S1] LED merah ");
    Serial.println(ledNyala ? "NYALA" : "MATI");
  };

  // S2: lambda buatan "pabrik" yang membawa datanya sendiri
  tombolS2.saatDitekan = buatAksiBip("S2", 3);

  Serial.println("BAB 18 - Program 4: Lambda untuk Dua Tombol");
  Serial.println("S1 = toggle LED merah, S2 = bip 3 kali");
}

void loop() {
  tombolS1.update();
  tombolS2.update();
  urusBuzzer();
}
