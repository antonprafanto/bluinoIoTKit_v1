/*
 * BAB 42 - Program 2: Koneksi Tangguh dengan LED Status
 * Menyambung ke WiFi tanpa memblokir loop(), memakai state machine,
 * event WiFi, sambung ulang otomatis, dan jeda bertahap (backoff).
 *
 * Rangkaian (kabel jumper female-female):
 *   LED1 (merah) → IO4   menyala saat istirahat sebelum mencoba lagi
 *   LED2 (hijau) → IO17  menyala saat tersambung
 *   LED3 (biru)  → IO2   berkedip saat menyambung (opsional, LED bawaan board juga di IO2)
 * Alat uji : Serial Monitor, baud 115200.
 */

#include <WiFi.h>

const char *WIFI_SSID = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char *WIFI_PASS = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

#define LED_MERAH 4    // LED1
#define LED_HIJAU 17   // LED2
#define LED_BIRU  2    // LED3 dan LED biru bawaan board

const unsigned long BATAS_MENYAMBUNG = 20000;  // satu percobaan paling lama 20 detik
const unsigned long JEDA_AWAL        = 5000;   // istirahat pertama 5 detik
const unsigned long JEDA_MAKS        = 60000;  // istirahat paling lama 60 detik

// ---------- Timer non-blocking (BAB 16) ----------
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // millis() saat terakhir dijalankan

  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerKedip = {250, 0};     // kedipan LED biru saat menyambung
Timer timerLapor = {10000, 0};   // laporan status setiap 10 detik

// ---------- State machine (BAB 17) ----------
enum StateWiFi { MENYAMBUNG, TERSAMBUNG, ISTIRAHAT };

StateWiFi state = MENYAMBUNG;
unsigned long waktuMasukState = 0;
unsigned long jedaIstirahat = JEDA_AWAL;
unsigned int percobaanKe = 1;

// Ditulis oleh callback event (task lain), dibaca oleh loop() → volatile
volatile uint8_t alasanTerakhir = 0;

const char *namaState(StateWiFi s) {
  switch (s) {
    case MENYAMBUNG: return "MENYAMBUNG";
    case TERSAMBUNG: return "TERSAMBUNG";
    case ISTIRAHAT:  return "ISTIRAHAT";
  }
  return "?";
}

// Saran perbaikan berdasarkan kode alasan terputus (tabel di 42.6.4)
const char *saranDariAlasan(uint8_t alasan) {
  switch (alasan) {
    case WIFI_REASON_NO_AP_FOUND:
      return "Nama WiFi tidak ditemukan. Cek huruf besar-kecil, pita 2,4 GHz, dan jarak ke router.";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
      return "Kemungkinan besar password salah (ingat: peka huruf besar-kecil).";
    case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
    case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
      return "Jenis keamanan jaringan tidak cocok (WEP/WPA lama, atau jaringan terbuka diberi password).";
    case WIFI_REASON_BEACON_TIMEOUT:
      return "Sinyal router hilang. Router mati atau terlalu jauh.";
    default:
      return "Periksa router/hotspot, lalu tunggu percobaan berikutnya.";
  }
}

// Dipanggil otomatis oleh task arduino_events setiap ada kejadian WiFi.
// Isinya dibuat singkat: mencatat dan mencetak pesan pendek saja.
void saatEventWiFi(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("[EVENT] Diterima router, menunggu alamat IP...");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("[EVENT] Mendapat alamat IP %s\n", WiFi.localIP().toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      uint8_t alasan = info.wifi_sta_disconnected.reason;
      alasanTerakhir = alasan;
      Serial.printf("[EVENT] Terputus, alasan %u (%s)\n", (unsigned)alasan,
                    WiFi.disconnectReasonName((wifi_err_reason_t)alasan));
      break;
    }
    default:
      break;
  }
}

// Pindah state + aksi masuk: atur ketiga LED sesuai state baru
void pindahState(StateWiFi stateBaru) {
  state = stateBaru;
  waktuMasukState = millis();
  Serial.printf("[STATE] %s\n", namaState(state));

  digitalWrite(LED_MERAH, state == ISTIRAHAT ? HIGH : LOW);
  digitalWrite(LED_HIJAU, state == TERSAMBUNG ? HIGH : LOW);
  digitalWrite(LED_BIRU,  state == MENYAMBUNG ? HIGH : LOW);  // dikedipkan di loop()
}

void mulaiMenyambung() {
  Serial.printf("Percobaan ke-%u: menyambung ke \"%s\"...\n", percobaanKe, WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  pindahState(MENYAMBUNG);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_BIRU, LOW);

  Serial.println();
  Serial.println("BAB 42 - Program 2: Koneksi Tangguh dengan LED Status");

  WiFi.onEvent(saatEventWiFi);    // daftarkan callback SEBELUM WiFi.begin()
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);    // bawaannya sudah true; ditulis supaya jelas
  mulaiMenyambung();
}

void loop() {
  unsigned long sekarang = millis();
  unsigned long lamaDiState = sekarang - waktuMasukState;
  bool tersambung = (WiFi.status() == WL_CONNECTED);

  switch (state) {
    case MENYAMBUNG:
      if (tersambung) {
        Serial.printf("Tersambung! IP %s, RSSI %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        jedaIstirahat = JEDA_AWAL;   // gangguan berikutnya mulai lagi dari jeda pendek
        percobaanKe = 1;
        pindahState(TERSAMBUNG);
      } else if (lamaDiState >= BATAS_MENYAMBUNG) {
        Serial.printf("Belum tersambung setelah %lu detik.\n", BATAS_MENYAMBUNG / 1000);
        Serial.printf("Saran: %s\n", saranDariAlasan(alasanTerakhir));
        WiFi.disconnect();           // hentikan percobaan driver selama istirahat
        Serial.printf("Istirahat %lu detik sebelum mencoba lagi.\n", jedaIstirahat / 1000);
        pindahState(ISTIRAHAT);
      } else if (timerKedip.sudahWaktunya(sekarang)) {
        digitalWrite(LED_BIRU, !digitalRead(LED_BIRU));   // kedipkan LED biru
      }
      break;

    case TERSAMBUNG:
      if (!tersambung) {
        Serial.println("Koneksi terputus! Driver WiFi mencoba menyambung ulang otomatis.");
        pindahState(MENYAMBUNG);
      }
      break;

    case ISTIRAHAT:
      if (lamaDiState >= jedaIstirahat) {
        jedaIstirahat = jedaIstirahat * 2;       // jeda berikutnya dua kali lebih lama
        if (jedaIstirahat > JEDA_MAKS) {
          jedaIstirahat = JEDA_MAKS;
        }
        percobaanKe = percobaanKe + 1;
        mulaiMenyambung();
      }
      break;
  }

  // Laporan berkala — di sinilah program aslimu (baca sensor, dsb.) bisa ditaruh
  if (timerLapor.sudahWaktunya(sekarang)) {
    if (tersambung) {
      Serial.printf("[LAPORAN] %s | IP %s | RSSI %d dBm | menyala %lu detik\n",
                    namaState(state), WiFi.localIP().toString().c_str(),
                    WiFi.RSSI(), sekarang / 1000);
    } else {
      Serial.printf("[LAPORAN] %s | status WiFi %d | menyala %lu detik\n",
                    namaState(state), (int)WiFi.status(), sekarang / 1000);
    }
  }
}
