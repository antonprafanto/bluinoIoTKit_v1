/*
 * BAB 52 - Program 4: Pairing dengan Konfirmasi Tombol
 * Pairing baru hanya diterima kalau angka 6 digit di HP sama dengan angka
 * di Serial Monitor DAN tombol S1 di kit ditekan dalam 30 detik
 * (SSP numeric comparison). Setelah tersambung, ESP32 membalas teks dari HP.
 *
 * Perintah di Serial Monitor (line ending New Line):
 *   daftar  → tampilkan perangkat yang tersimpan (bonded) di ESP32
 *   hapus   → hapus semua perangkat yang tersimpan di ESP32
 *
 * Rangkaian: SW1 (header kuning) → IO15, ditekan = HIGH
 *            LED biru bawaan board (IO2): berkedip saat menunggu S1,
 *            menyala tetap saat ada perangkat tersambung
 */

#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_SPP_ENABLED)
#error "Bluetooth Classic (SPP) tidak tersedia. Pilih Tools > Board > ESP32 Dev Module."
#endif

#define TOMBOL_S1 15  // SW1 → IO15
#define LED_BIRU  2

const char *NAMA_BT = "Bluino-Aman";
const unsigned long BATAS_KONFIRMASI_MS = 30000;  // waktu untuk menekan S1
const int MAKS_PERANGKAT = 20;                    // ukuran array daftar perangkat

BluetoothSerial SerialBT;

// Diisi oleh callback (task Bluetooth), dibaca oleh loop()
volatile bool adaPermintaan = false;  // ada permintaan pairing baru?
volatile uint32_t angkaPairing = 0;   // angka 6 digit yang harus dicocokkan
volatile int hasilAutentikasi = 0;    // 0 = belum ada, 1 = berhasil, 2 = gagal

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

bool sedangPairing = false;       // sedang ada proses pairing baru?
bool menungguS1 = false;          // sedang menunggu tombol S1?
unsigned long waktuMinta = 0;     // millis() saat permintaan pairing datang
unsigned long waktuKedip = 0;
bool ledKedip = false;
bool adaKoneksi = false;
PembacaBaris dariHP;
PembacaBaris dariLaptop;

// ---- Callback dari task Bluetooth: hanya mencatat ----
void saatMintaKonfirmasi(uint32_t angka) {
  angkaPairing = angka;
  adaPermintaan = true;
}

void saatAutentikasiSelesai(boolean berhasil) {
  hasilAutentikasi = berhasil ? 1 : 2;
}

// ---- Daftar perangkat yang tersimpan di ESP32 ----
void tampilkanDaftar() {
  esp_bd_addr_t daftar[MAKS_PERANGKAT];
  int jumlah = SerialBT.getBondedDevices(MAKS_PERANGKAT, daftar);
  Serial.printf("Perangkat tersimpan di ESP32: %d\n", jumlah);
  for (int i = 0; i < jumlah; i++) {
    Serial.printf("  %d. %02X:%02X:%02X:%02X:%02X:%02X\n", i + 1,
                  daftar[i][0], daftar[i][1], daftar[i][2],
                  daftar[i][3], daftar[i][4], daftar[i][5]);
  }
}

void hapusSemua() {
  esp_bd_addr_t daftar[MAKS_PERANGKAT];
  int jumlah = SerialBT.getBondedDevices(MAKS_PERANGKAT, daftar);
  for (int i = 0; i < jumlah; i++) {
    SerialBT.deleteBondedDevice(daftar[i]);
  }
  Serial.printf("%d perangkat dihapus dari ESP32.\n", jumlah);
  Serial.println("Jangan lupa: hapus juga ESP32 ini di menu Bluetooth HP (Lupakan/Unpair).");
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT_PULLDOWN);  // kit sudah punya pull-down; yang internal hanya cadangan
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);

  Serial.println();
  Serial.println("BAB 52 - Program 4: Pairing dengan Konfirmasi Tombol");

  // Semua pengaturan pairing WAJIB sebelum begin()
  SerialBT.enableSSP(true, true);  // numeric comparison: bandingkan angka 6 digit
  SerialBT.onConfirmRequest(saatMintaKonfirmasi);
  SerialBT.onAuthComplete(saatAutentikasiSelesai);

  if (!SerialBT.begin(NAMA_BT)) {
    Serial.println("Bluetooth gagal dinyalakan. Tekan tombol EN untuk mencoba lagi.");
    while (true) {
      delay(1000);
    }
  }
  Serial.printf("Bluetooth siap. Nama: %s\n", NAMA_BT);
  tampilkanDaftar();
  Serial.println("Ketik 'daftar' atau 'hapus' di Serial Monitor.");
}

void loop() {
  unsigned long sekarang = millis();

  // 1. Permintaan pairing baru dari callback
  if (adaPermintaan) {
    adaPermintaan = false;
    sedangPairing = true;
    menungguS1 = true;
    waktuMinta = sekarang;
    Serial.printf("[PAIRING] Permintaan pairing baru. Angka: %06lu\n", (unsigned long)angkaPairing);
    Serial.println("[PAIRING] Kalau angka di HP sama, tekan S1 dalam 30 detik. Kalau tidak, biarkan saja.");
  }

  // 2. Menunggu tombol S1 (ditekan = HIGH)
  if (menungguS1) {
    if (digitalRead(TOMBOL_S1) == HIGH) {
      menungguS1 = false;
      SerialBT.confirmReply(true);
      Serial.println("[PAIRING] S1 ditekan - ESP32 menyetujui pairing.");
    } else if (sekarang - waktuMinta >= BATAS_KONFIRMASI_MS) {
      menungguS1 = false;
      SerialBT.confirmReply(false);
      Serial.println("[PAIRING] Waktu habis - pairing ditolak.");
    }
  }

  // 3. Hasil autentikasi dari callback
  if (hasilAutentikasi != 0) {
    int hasil = hasilAutentikasi;
    hasilAutentikasi = 0;
    if (sedangPairing) {
      Serial.println(hasil == 1 ? "[PAIRING] Berhasil. Perangkat ini sekarang tersimpan di ESP32."
                                : "[PAIRING] Gagal atau dibatalkan.");
      sedangPairing = false;
      menungguS1 = false;
    } else if (hasil == 2) {
      Serial.println("[BT] Autentikasi gagal. Lupakan ESP32 di HP, ketik 'hapus', lalu pairing ulang.");
    }
  }

  // 4. LED biru: berkedip saat menunggu S1, menyala saat tersambung
  bool tersambung = SerialBT.hasClient();
  if (menungguS1) {
    if (sekarang - waktuKedip >= 125) {
      waktuKedip = sekarang;
      ledKedip = !ledKedip;
      digitalWrite(LED_BIRU, ledKedip ? HIGH : LOW);
    }
  } else {
    digitalWrite(LED_BIRU, tersambung ? HIGH : LOW);
  }

  // 5. Perubahan koneksi
  if (tersambung != adaKoneksi) {
    adaKoneksi = tersambung;
    if (tersambung) {
      Serial.println("[BT] Perangkat tersambung.");
      SerialBT.println("Halo! Kamu tersambung ke ESP32 dengan pairing aman.");
    } else {
      Serial.println("[BT] Perangkat terputus.");
    }
  }

  // 6. Teks dari HP: balas (echo)
  if (dariHP.baca(SerialBT)) {
    Serial.printf("[HP] %s\n", dariHP.isi);
    SerialBT.printf("ESP32 menerima: %s\r\n", dariHP.isi);
  }

  // 7. Perintah dari Serial Monitor
  if (dariLaptop.baca(Serial)) {
    String perintah = dariLaptop.isi;
    perintah.trim();
    perintah.toLowerCase();
    if (perintah == "daftar") {
      tampilkanDaftar();
    } else if (perintah == "hapus") {
      hapusSemua();
    } else {
      Serial.println("Perintah Serial Monitor: daftar | hapus");
    }
  }
}
