/*
 * BAB 52 - Program 3: Remote Relay dengan Pengaman
 * Mengendalikan relay kit dari HP lewat Bluetooth Classic.
 * Perintah (dikirim dari aplikasi Serial Bluetooth Terminal):
 *   nyala           → relay aktif, mati sendiri setelah 60 detik
 *   nyala <detik>   → relay aktif selama 1-3600 detik
 *   mati            → relay mati
 *   status          → keadaan relay dan sisa waktunya
 *   bantuan         → daftar perintah
 * Pengaman:
 *   1. Relay selalu mati saat ESP32 menyala atau di-reset.
 *   2. Relay langsung mati kalau tidak ada perangkat yang tersambung.
 *   3. Relay mati sendiri setelah batas waktunya habis.
 *   4. Relay baru boleh dinyalakan lagi 1 detik setelah perubahan terakhir.
 *
 * Rangkaian: RELAY (header biru, kanan BUZZ) → IO26 (aktif HIGH)
 *            LED biru bawaan board (IO2) menyala selama HP tersambung
 * JANGAN menyambung listrik PLN 220 V ke relay kit (lihat BAB 37).
 */

#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_SPP_ENABLED)
#error "Bluetooth Classic (SPP) tidak tersedia. Pilih Tools > Board > ESP32 Dev Module."
#endif

#define RELAY_PIN 26  // RELAY → IO26
#define LED_BIRU  2

const char *NAMA_BT = "Bluino-Relay";
const long BATAS_BAWAAN_DETIK = 60;      // lama nyala kalau tidak disebutkan
const long BATAS_MAKS_DETIK = 3600;      // lama nyala paling panjang (1 jam)
const unsigned long JEDA_MIN_MS = 1000;  // jarak minimum sebelum relay dinyalakan lagi

BluetoothSerial SerialBT;

// Bendera dari callback Bluetooth (dijalankan task lain) untuk loop()
volatile bool adaEventKoneksi = false;

// Pembaca baris tanpa menunggu (lihat 52.6)
struct PembacaBaris {
  char isi[64];
  int panjang = 0;

  bool baca(Stream &sumber) {
    while (sumber.available() > 0) {
      char c = sumber.read();
      if (c == '\r' || c == '\n') {
        if (panjang == 0) continue;
        isi[panjang] = '\0';
        panjang = 0;
        return true;
      }
      if (panjang < (int)sizeof(isi) - 1) {
        isi[panjang] = c;
        panjang = panjang + 1;
      }
    }
    return false;
  }
};

bool relayAktif = false;
unsigned long waktuBerubah = 0;  // millis() saat relay terakhir berganti keadaan
unsigned long waktuMulai = 0;    // millis() saat hitungan batas waktu dimulai
unsigned long lamaNyalaMs = 0;   // batas lama nyala yang diminta
bool adaKoneksi = false;
PembacaBaris dariHP;

// Callback: dipanggil task Bluetooth setiap ada event SPP.
// Isinya hanya mengangkat bendera — pekerjaan lain dilakukan di loop().
void saatEventSPP(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT || event == ESP_SPP_CLOSE_EVT) {
    adaEventKoneksi = true;
  }
}

// Satu-satunya tempat yang menyentuh pin relay (pola BAB 37)
void aturRelay(bool aktif, const char *alasan) {
  relayAktif = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  waktuBerubah = millis();
  Serial.printf("[RELAY] %s - %s\n", aktif ? "ON" : "OFF", alasan);
  if (SerialBT.hasClient()) {
    SerialBT.printf("Relay %s - %s\r\n", aktif ? "ON" : "OFF", alasan);
  }
}

void kirimBantuan() {
  SerialBT.println("Daftar perintah:");
  SerialBT.println("  nyala            ON selama 60 detik");
  SerialBT.println("  nyala <detik>    ON selama 1-3600 detik");
  SerialBT.println("  mati             OFF");
  SerialBT.println("  status           keadaan relay");
  SerialBT.println("  bantuan          daftar ini");
}

void kirimStatus(unsigned long sekarang) {
  if (relayAktif) {
    unsigned long sisaMs = lamaNyalaMs - (sekarang - waktuMulai);
    SerialBT.printf("Relay ON, mati otomatis dalam %lu detik\r\n", (sisaMs + 999) / 1000);
  } else {
    SerialBT.println("Relay OFF");
  }
}

// true kalau teks berisi bilangan bulat utuh, misalnya "30"
bool keAngka(const String &teks, long &hasil) {
  if (teks.length() == 0) return false;
  char *sisa;
  hasil = strtol(teks.c_str(), &sisa, 10);
  return *sisa == '\0';
}

void prosesPerintah(const char *teks, unsigned long sekarang) {
  String baris = teks;
  baris.trim();
  baris.toLowerCase();
  if (baris.length() == 0) return;

  Serial.printf("[HP] %s\n", baris.c_str());

  String perintah = baris;
  String argumen = "";
  int posisiSpasi = baris.indexOf(' ');
  if (posisiSpasi >= 0) {
    perintah = baris.substring(0, posisiSpasi);
    argumen = baris.substring(posisiSpasi + 1);
    argumen.trim();
  }

  if (perintah == "nyala") {
    long detik = BATAS_BAWAAN_DETIK;
    if (argumen.length() > 0) {
      if (!keAngka(argumen, detik) || detik < 1 || detik > BATAS_MAKS_DETIK) {
        SerialBT.println("Lama nyala harus 1-3600 detik, contoh: nyala 30");
        return;
      }
    }
    if (!relayAktif && sekarang - waktuBerubah < JEDA_MIN_MS) {
      SerialBT.println("Terlalu cepat. Tunggu 1 detik, lalu coba lagi.");
      return;
    }
    waktuMulai = sekarang;
    lamaNyalaMs = (unsigned long)detik * 1000UL;
    if (relayAktif) {
      SerialBT.printf("Relay tetap ON, batas waktu diatur ulang: %ld detik\r\n", detik);
    } else {
      aturRelay(true, "perintah dari HP");
      SerialBT.printf("Mati otomatis dalam %ld detik.\r\n", detik);
    }
  } else if (perintah == "mati") {
    if (relayAktif) {
      aturRelay(false, "perintah dari HP");  // mematikan selalu boleh, tanpa jeda
    } else {
      SerialBT.println("Relay memang sudah OFF.");
    }
  } else if (perintah == "status") {
    kirimStatus(sekarang);
  } else if (perintah == "bantuan") {
    kirimBantuan();
  } else {
    SerialBT.println("Perintah tidak dikenal. Ketik: bantuan");
  }
}

void setup() {
  Serial.begin(115200);

  // Pengaman 1: pin dijadikan output dulu, lalu relay langsung dimatikan
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);

  Serial.println();
  Serial.println("BAB 52 - Program 3: Remote Relay dengan Pengaman");
  aturRelay(false, "ESP32 baru menyala");

  SerialBT.register_callback(saatEventSPP);  // daftarkan SEBELUM begin()
  if (!SerialBT.begin(NAMA_BT)) {
    Serial.println("Bluetooth gagal dinyalakan. Tekan tombol EN untuk mencoba lagi.");
    while (true) {
      delay(1000);
    }
  }
  Serial.printf("Bluetooth siap. Nama: %s\n", NAMA_BT);
}

void loop() {
  unsigned long sekarang = millis();

  // 1. Bel pintu dari callback: periksa keadaan koneksi yang sebenarnya
  if (adaEventKoneksi) {
    adaEventKoneksi = false;  // turunkan bendera DULU, baru periksa keadaan
    bool tersambung = SerialBT.hasClient();
    if (tersambung != adaKoneksi) {
      adaKoneksi = tersambung;
      digitalWrite(LED_BIRU, tersambung ? HIGH : LOW);
      if (tersambung) {
        Serial.println("[BT] Perangkat tersambung.");
        SerialBT.println("=== Bluino Relay: kendali relay lewat Bluetooth ===");
        kirimBantuan();
        kirimStatus(sekarang);
      } else {
        Serial.println("[BT] Perangkat terputus.");
      }
    }
  }

  // 2. Pengaman 2: tanpa perangkat tersambung, relay tidak boleh aktif
  if (relayAktif && !SerialBT.hasClient()) {
    aturRelay(false, "koneksi Bluetooth terputus");
  }

  // 3. Pengaman 3: batas waktu nyala
  if (relayAktif && sekarang - waktuMulai >= lamaNyalaMs) {
    aturRelay(false, "batas waktu habis");
  }

  // 4. Perintah dari HP
  if (dariHP.baca(SerialBT)) {
    prosesPerintah(dariHP.isi, sekarang);
  }
}
