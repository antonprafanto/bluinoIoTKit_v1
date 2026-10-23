/*
 * BAB 52 - Program 2: Data DHT11 ke HP
 * ESP32 mengirim suhu dan kelembapan DHT11 ke HP lewat Bluetooth Classic
 * secara berkala, dan menjawab perintah teks dari HP:
 *   baca              → kirim data terbaru sekarang juga
 *   mulai / berhenti  → nyalakan / matikan pengiriman otomatis
 *   interval <detik>  → ubah jarak pengiriman otomatis (2-3600 detik)
 *   status            → tampilkan pengaturan saat ini
 *   bantuan           → tampilkan daftar perintah
 *
 * Rangkaian: DHT (header hijau) → IO27
 *            LED biru bawaan board (IO2) menyala selama HP tersambung
 * Library  : DHT sensor library + Adafruit Unified Sensor (Adafruit)
 * Board    : ESP32 Dev Module
 */

#include <BluetoothSerial.h>
#include <DHT.h>

#if !defined(CONFIG_BT_SPP_ENABLED)
#error "Bluetooth Classic (SPP) tidak tersedia. Pilih Tools > Board > ESP32 Dev Module."
#endif

#define DHT_PIN  27
#define DHT_TYPE DHT11
#define LED_BIRU 2

const char *NAMA_BT = "Bluino-Sensor";

BluetoothSerial SerialBT;
DHT dht(DHT_PIN, DHT_TYPE);

// Timer serbaguna dari BAB 16
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

// Bacaan valid terakhir dari DHT11
struct DataDHT {
  float suhu;               // °C
  float kelembapan;         // %RH
  unsigned long waktuBaca;  // millis() saat data didapat
  bool adaData;             // sudah pernah berhasil membaca?
};

Timer timerDHT = {2000, 0};    // baca DHT11 setiap 2 detik (jangan kurang dari 2000)
Timer timerKirim = {5000, 0};  // kirim ke HP setiap 5 detik (bisa diubah dari HP)

DataDHT dataTerakhir = {NAN, NAN, 0, false};
int gagalBeruntun = 0;         // berapa kali DHT11 gagal dibaca berturut-turut
bool kirimOtomatis = true;     // pengiriman berkala aktif?
bool adaKoneksi = false;       // keadaan koneksi pada putaran sebelumnya
PembacaBaris dariHP;

void bacaDHT(unsigned long sekarang) {
  dht.read(true);  // paksa membaca sensor sekarang (lihat BAB 26)
  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();

  if (isnan(kelembapan) || isnan(suhu)) {
    gagalBeruntun = gagalBeruntun + 1;
    if (gagalBeruntun == 3) {
      Serial.println("[DHT] Gagal membaca 3 kali berturut-turut. Cek kabel DHT -> IO27.");
    }
    return;  // dataTerakhir sengaja tidak diubah
  }

  gagalBeruntun = 0;
  dataTerakhir.suhu = suhu;
  dataTerakhir.kelembapan = kelembapan;
  dataTerakhir.waktuBaca = sekarang;
  dataTerakhir.adaData = true;
}

// Mengirim satu baris data ke HP, dan salinannya ke Serial Monitor
void kirimData(unsigned long sekarang) {
  if (!dataTerakhir.adaData) {
    SerialBT.println("Belum ada data DHT11. Cek kabel DHT -> IO27.");
    Serial.println("[Kirim] Belum ada data DHT11.");
    return;
  }

  char baris[80];
  int n = snprintf(baris, sizeof(baris), "Suhu: %.1f C | Kelembapan: %.0f %%",
                   dataTerakhir.suhu, dataTerakhir.kelembapan);

  // Tandai kalau data sudah lama (DHT11 gagal dibaca beberapa kali)
  unsigned long umurDetik = (sekarang - dataTerakhir.waktuBaca) / 1000;
  if (umurDetik >= 10 && n > 0 && n < (int)sizeof(baris)) {
    snprintf(baris + n, sizeof(baris) - n, " (data lama: %lu s)", umurDetik);
  }

  SerialBT.println(baris);
  Serial.printf("[Kirim] %s\n", baris);
}

void kirimBantuan() {
  SerialBT.println("Daftar perintah:");
  SerialBT.println("  baca              data terbaru sekarang");
  SerialBT.println("  mulai / berhenti  kirim otomatis on/off");
  SerialBT.println("  interval <detik>  2-3600, contoh: interval 10");
  SerialBT.println("  status            pengaturan saat ini");
  SerialBT.println("  bantuan           daftar ini");
}

// true kalau teks berisi bilangan bulat utuh, misalnya "10" (bukan "", "10x", atau "1.5")
bool keAngka(const String &teks, long &hasil) {
  if (teks.length() == 0) return false;
  char *sisa;
  hasil = strtol(teks.c_str(), &sisa, 10);
  return *sisa == '\0';
}

void prosesPerintah(const char *teks, unsigned long sekarang) {
  String baris = teks;
  baris.trim();          // buang spasi di awal dan akhir
  baris.toLowerCase();   // "BACA" dianggap sama dengan "baca"
  if (baris.length() == 0) return;

  Serial.printf("[HP] %s\n", baris.c_str());

  // Pisahkan kata pertama (perintah) dari sisanya (argumen), seperti BAB 22
  String perintah = baris;
  String argumen = "";
  int posisiSpasi = baris.indexOf(' ');
  if (posisiSpasi >= 0) {
    perintah = baris.substring(0, posisiSpasi);
    argumen = baris.substring(posisiSpasi + 1);
    argumen.trim();
  }

  if (perintah == "baca") {
    kirimData(sekarang);
  } else if (perintah == "mulai") {
    kirimOtomatis = true;
    timerKirim.waktuLalu = sekarang;  // hitung ulang jaraknya dari sekarang
    SerialBT.printf("OK: kirim otomatis setiap %lu detik.\r\n", timerKirim.interval / 1000);
  } else if (perintah == "berhenti") {
    kirimOtomatis = false;
    SerialBT.println("OK: kirim otomatis dihentikan. Ketik 'mulai' untuk melanjutkan.");
  } else if (perintah == "interval") {
    long detik;
    if (!keAngka(argumen, detik) || detik < 2 || detik > 3600) {
      SerialBT.println("Interval harus angka 2-3600 detik, contoh: interval 10");
    } else {
      timerKirim.interval = (unsigned long)detik * 1000UL;
      timerKirim.waktuLalu = sekarang;
      SerialBT.printf("OK: interval kirim %ld detik.\r\n", detik);
    }
  } else if (perintah == "status") {
    SerialBT.printf("Kirim otomatis: %s, interval %lu detik\r\n",
                    kirimOtomatis ? "ON" : "OFF", timerKirim.interval / 1000);
    SerialBT.printf("Waktu menyala: %lu detik\r\n", sekarang / 1000);
  } else if (perintah == "bantuan") {
    kirimBantuan();
  } else {
    SerialBT.println("Perintah tidak dikenal. Ketik: bantuan");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  dht.begin();

  Serial.println();
  Serial.println("BAB 52 - Program 2: Data DHT11 ke HP");

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

  // 1. Baca DHT11 setiap 2 detik, tersambung atau tidak
  if (timerDHT.sudahWaktunya(sekarang)) {
    bacaDHT(sekarang);
  }

  // 2. Pantau perubahan koneksi
  bool tersambung = SerialBT.hasClient();
  if (tersambung != adaKoneksi) {
    adaKoneksi = tersambung;
    digitalWrite(LED_BIRU, tersambung ? HIGH : LOW);
    if (tersambung) {
      Serial.println("[BT] Perangkat tersambung.");
      SerialBT.println("=== Bluino Sensor: DHT11 lewat Bluetooth ===");
      kirimBantuan();
      kirimData(sekarang);
      timerKirim.waktuLalu = sekarang;
    } else {
      Serial.println("[BT] Perangkat terputus.");
    }
  }

  // 3. Perintah dari HP
  if (dariHP.baca(SerialBT)) {
    prosesPerintah(dariHP.isi, sekarang);
  }

  // 4. Kirim data berkala, hanya kalau ada yang tersambung
  if (adaKoneksi && kirimOtomatis && timerKirim.sudahWaktunya(sekarang)) {
    kirimData(sekarang);
  }
}
