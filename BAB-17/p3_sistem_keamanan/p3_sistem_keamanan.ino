/*
 * BAB 17 - Program 3: Sistem Keamanan dengan Sensor PIR
 * State: NONAKTIF → PERSIAPAN (10 detik) → SIAGA → WASPADA (5 detik) → ALARM
 * Tombol S1 mengaktifkan sistem, atau mematikannya kapan saja.
 * ALARM berhenti sendiri setelah 20 detik, lalu sistem kembali SIAGA.
 *
 * Rangkaian:
 *   LED1 (merah)                → IO4
 *   LED2 (hijau)                → IO17
 *   BUZZ                        → IO13
 *   Tombol S1 (pin kuning ke-3) → IO15
 *   PIR (header hijau)          → IO33
 */

#define LED_MERAH  4
#define LED_HIJAU  17
#define BUZZER_PIN 13
#define TOMBOL_S1  15
#define PIR_PIN    33

enum StateKeamanan {
  NONAKTIF,   // sistem mati
  PERSIAPAN,  // hitung mundur sebelum aktif, supaya pemilik sempat keluar
  SIAGA,      // aktif, mengawasi gerakan
  WASPADA,    // ada gerakan, pemilik masih bisa mematikan sistem
  ALARM       // alarm berbunyi
};

const unsigned long LAMA_PERSIAPAN = 10000;  // ms
const unsigned long LAMA_WASPADA   = 5000;   // ms
const unsigned long LAMA_ALARM     = 20000;  // ms, setelah itu alarm berhenti sendiri

StateKeamanan state = NONAKTIF;
unsigned long waktuMasukState = 0;
int sisaDetikTercetak = -1;  // angka hitung mundur terakhir yang sudah dicetak

// Debounce tombol S1 (sama seperti di BAB 9 dan BAB 16)
const unsigned long WAKTU_DEBOUNCE = 50;
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

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

// Mengubah state menjadi teks untuk log di Serial Monitor
const char *namaState(StateKeamanan s) {
  switch (s) {
    case NONAKTIF:  return "NONAKTIF";
    case PERSIAPAN: return "PERSIAPAN";
    case SIAGA:     return "SIAGA";
    case WASPADA:   return "WASPADA";
    case ALARM:     return "ALARM";
  }
  return "?";
}

// Pindah state: tulis log, catat waktu, matikan semua output, lalu aksi masuk
void pindahState(StateKeamanan stateBaru, const char *alasan) {
  Serial.print(namaState(state));
  Serial.print(" -> ");
  Serial.print(namaState(stateBaru));
  Serial.print(" (");
  Serial.print(alasan);
  Serial.println(")");

  state = stateBaru;
  waktuMasukState = millis();
  sisaDetikTercetak = -1;

  // Mulai dari keadaan bersih
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Aksi masuk
  if (state == SIAGA) {
    digitalWrite(LED_HIJAU, HIGH);  // hijau menyala tetap = sistem sedang mengawasi
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TOMBOL_S1, INPUT);
  pinMode(PIR_PIN, INPUT);  // keluaran PIR: HIGH saat ada gerakan

  Serial.println("BAB 17 - Program 3: Sistem Keamanan");
  Serial.println("Tekan S1 untuk mengaktifkan atau mematikan sistem.");
  Serial.print("State awal: ");
  Serial.println(namaState(state));
}

void loop() {
  // Kumpulkan semua event SEKALI di awal putaran
  bool tombolDitekan = s1BaruDitekan();
  bool adaGerakan = (digitalRead(PIR_PIN) == HIGH);
  unsigned long lamaDiState = millis() - waktuMasukState;

  switch (state) {
    case NONAKTIF:
      if (tombolDitekan) {
        pindahState(PERSIAPAN, "tombol S1");
      }
      break;

    case PERSIAPAN:
      if (tombolDitekan) {
        pindahState(NONAKTIF, "dibatalkan dengan tombol S1");
      } else if (lamaDiState >= LAMA_PERSIAPAN) {
        pindahState(SIAGA, "waktu persiapan habis");
      } else {
        // Aksi selama di state: LED hijau berkedip lambat + hitung mundur
        digitalWrite(LED_HIJAU, (lamaDiState / 500) % 2 == 0 ? HIGH : LOW);
        int sisaDetik = (LAMA_PERSIAPAN - lamaDiState + 999) / 1000;  // dibulatkan ke atas
        if (sisaDetik != sisaDetikTercetak) {
          sisaDetikTercetak = sisaDetik;
          Serial.print("  Sistem aktif dalam ");
          Serial.print(sisaDetik);
          Serial.println(" detik");
        }
      }
      break;

    case SIAGA:
      if (tombolDitekan) {
        pindahState(NONAKTIF, "tombol S1");
      } else if (adaGerakan) {
        pindahState(WASPADA, "PIR mendeteksi gerakan");
      }
      break;

    case WASPADA:
      if (tombolDitekan) {
        pindahState(NONAKTIF, "dimatikan dengan tombol S1");
      } else if (lamaDiState >= LAMA_WASPADA) {
        pindahState(ALARM, "tidak dimatikan tepat waktu");
      } else {
        // LED merah berkedip cepat, buzzer "bip" 50 ms setiap detik
        digitalWrite(LED_MERAH, (lamaDiState / 100) % 2 == 0 ? HIGH : LOW);
        digitalWrite(BUZZER_PIN, (lamaDiState % 1000) < 50 ? HIGH : LOW);
      }
      break;

    case ALARM:
      if (tombolDitekan) {
        pindahState(NONAKTIF, "dimatikan dengan tombol S1");
      } else if (lamaDiState >= LAMA_ALARM) {
        pindahState(SIAGA, "alarm berhenti sendiri");
      } else {
        // LED merah dan buzzer menyala-mati bersamaan setiap 100 ms
        bool fase = (lamaDiState / 100) % 2 == 0;
        digitalWrite(LED_MERAH, fase ? HIGH : LOW);
        digitalWrite(BUZZER_PIN, fase ? HIGH : LOW);
      }
      break;
  }
}
