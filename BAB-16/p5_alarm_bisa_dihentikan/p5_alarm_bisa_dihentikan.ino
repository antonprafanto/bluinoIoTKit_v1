/*
 * BAB 16 - Program 5: Alarm yang Bisa Dihentikan Kapan Saja
 * Tekan S1 → alarm berbunyi 10 kali (buzzer + LED merah).
 * Tekan S1 lagi di tengah alarm → alarm langsung berhenti.
 * LED hijau berkedip pelan sebagai tanda program tidak pernah macet.
 *
 * Rangkaian:
 *   BUZZ                        → IO13
 *   LED1 (merah)                → IO4
 *   LED2 (hijau)                → IO17
 *   Tombol S1 (pin kuning ke-3) → IO15
 */

#define BUZZER_PIN 13
#define LED_MERAH  4
#define LED_HIJAU  17
#define TOMBOL_S1  15

// Pengaturan alarm
const int JUMLAH_BIP = 10;
const unsigned long LAMA_BUNYI = 200;  // ms buzzer berbunyi
const unsigned long LAMA_DIAM  = 200;  // ms buzzer diam di antara bunyi

// Penanda langkah alarm
bool alarmAktif = false;
bool sedangBunyi = false;        // true = langkah "bunyi", false = langkah "diam"
int bipKe = 0;                   // bip ke berapa (1 sampai JUMLAH_BIP)
unsigned long waktuLangkah = 0;  // kapan langkah sekarang dimulai

// Debounce tombol S1 (sama seperti Program 3 di BAB 9)
const unsigned long WAKTU_DEBOUNCE = 50;
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

// LED hijau "detak jantung" (heartbeat)
const unsigned long INTERVAL_DETAK = 500;
unsigned long waktuDetakLalu = 0;
bool hijauNyala = false;

// Mengembalikan true hanya sekali setiap S1 baru ditekan (sudah di-debounce)
bool s1BaruDitekan() {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = millis();
    bacaanSebelumnya = bacaan;
  }
  if ((millis() - waktuBerubah) > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    return statusStabil == HIGH;  // true hanya pada tepi naik (LOW → HIGH)
  }
  return false;
}

// Menyalakan atau mematikan buzzer dan LED merah bersamaan
void bunyikan(bool nyala) {
  digitalWrite(BUZZER_PIN, nyala ? HIGH : LOW);
  digitalWrite(LED_MERAH, nyala ? HIGH : LOW);
}

// Memulai satu bip baru (langkah "bunyi")
void mulaiBip(unsigned long sekarang) {
  bipKe++;
  sedangBunyi = true;
  waktuLangkah = sekarang;
  bunyikan(true);
  Serial.print("Bip ke-");
  Serial.print(bipKe);
  Serial.print(" dari ");
  Serial.println(JUMLAH_BIP);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(TOMBOL_S1, INPUT);
  bunyikan(false);

  Serial.println("BAB 16 - Program 5: Alarm yang Bisa Dihentikan Kapan Saja");
  Serial.println("Tekan S1 untuk menyalakan alarm, tekan lagi untuk menghentikannya.");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: S1 menyalakan atau menghentikan alarm
  if (s1BaruDitekan()) {
    if (!alarmAktif) {
      alarmAktif = true;
      bipKe = 0;
      Serial.println("Alarm MULAI");
      mulaiBip(sekarang);
    } else {
      alarmAktif = false;
      bunyikan(false);
      Serial.print("Alarm DIHENTIKAN pada bip ke-");
      Serial.println(bipKe);
    }
  }

  // Tugas 2: urutan bunyi-diam alarm, tanpa delay()
  if (alarmAktif) {
    if (sedangBunyi && sekarang - waktuLangkah >= LAMA_BUNYI) {
      // Langkah "bunyi" selesai → pindah ke langkah "diam"
      sedangBunyi = false;
      waktuLangkah = sekarang;
      bunyikan(false);
      if (bipKe >= JUMLAH_BIP) {  // bip terakhir sudah selesai
        alarmAktif = false;
        Serial.println("Alarm SELESAI");
      }
    } else if (!sedangBunyi && sekarang - waktuLangkah >= LAMA_DIAM) {
      // Langkah "diam" selesai → mulai bip berikutnya
      mulaiBip(sekarang);
    }
  }

  // Tugas 3: LED hijau berkedip pelan sebagai tanda program tetap berjalan
  if (sekarang - waktuDetakLalu >= INTERVAL_DETAK) {
    waktuDetakLalu = sekarang;
    hijauNyala = !hijauNyala;
    digitalWrite(LED_HIJAU, hijauNyala ? HIGH : LOW);
  }
}
