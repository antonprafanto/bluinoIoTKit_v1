/*
 * BAB 42 - Program 5: Ganti WiFi Lewat Serial Monitor
 * Nama dan password WiFi disimpan di NVS (Preferences), bukan di kode.
 *
 * Ketik perintah di Serial Monitor (line ending: New Line):
 *   ssid <nama>    → simpan nama WiFi (boleh mengandung spasi)
 *   pass <sandi>   → simpan password WiFi ("pass" saja = WiFi tanpa password)
 *   sambung        → sambungkan memakai data tersimpan
 *   putus          → putuskan koneksi
 *   status         → tampilkan data tersimpan dan status koneksi
 *   lupa           → hapus nama dan password dari NVS
 *   bantuan        → daftar perintah
 *
 * Rangkaian: tidak perlu. LED biru bawaan board (IO2) menyala saat tersambung.
 * Alat uji : Serial Monitor, baud 115200, line ending New Line.
 */

#include <WiFi.h>
#include <Preferences.h>

#define NAMESPACE_WIFI "wifi-kit"   // nama namespace NVS, maksimal 15 karakter
#define LED_BIRU 2                  // LED biru bawaan board

const unsigned long BATAS_MENYAMBUNG = 20000;   // 20 detik

Preferences prefs;
String ssidTersimpan;   // salinan di RAM; program bekerja dengan variabel ini
String passTersimpan;

bool sedangMenyambung = false;        // baru saja memberi perintah sambung?
unsigned long waktuMulaiSambung = 0;
bool tersambungSebelumnya = false;    // untuk mendeteksi perubahan status

// ---------- Pembaca baris tanpa menunggu (sama seperti BAB 22 dan BAB 39) ----------
const int PANJANG_MAKS = 100;   // termasuk 1 tempat untuk '\0'
char baris[PANJANG_MAKS];
int panjang = 0;

bool bacaBaris() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {      // karakter akhir baris
      if (panjang == 0) continue;      // baris kosong: lewati
      baris[panjang] = '\0';
      panjang = 0;
      return true;
    }
    if (panjang < PANJANG_MAKS - 1) {  // kalau sudah penuh, karakter dibuang
      baris[panjang] = c;
      panjang++;
    }
  }
  return false;
}

// ---------- Fungsi NVS ----------
void muatKredensial() {
  prefs.begin(NAMESPACE_WIFI, false);   // baca-tulis: namespace dibuat kalau belum ada
  ssidTersimpan = prefs.getString("ssid", "");
  passTersimpan = prefs.getString("pass", "");
  prefs.end();
}

// Menyimpan teks ke NVS. Teks kosong disimpan dengan menghapus key-nya,
// karena getString() mengembalikan nilai bawaan "" untuk key yang tidak ada.
bool simpanTeks(const char *kunci, const String &nilai) {
  if (!prefs.begin(NAMESPACE_WIFI, false)) {
    return false;
  }
  bool berhasil = true;
  if (nilai.length() == 0) {
    prefs.remove(kunci);
  } else {
    berhasil = prefs.putString(kunci, nilai) > 0;
  }
  prefs.end();
  return berhasil;
}

// ---------- Fungsi WiFi ----------
void mulaiMenyambung() {
  if (ssidTersimpan.length() == 0) {
    Serial.println("Belum ada nama WiFi. Ketik: ssid <nama WiFi>");
    return;
  }
  Serial.printf("Menyambung ke \"%s\"...\n", ssidTersimpan.c_str());
  WiFi.disconnect();   // hentikan koneksi/percobaan sebelumnya (WiFi tetap menyala)
  WiFi.begin(ssidTersimpan.c_str(), passTersimpan.c_str());
  sedangMenyambung = true;
  waktuMulaiSambung = millis();
}

void tampilkanStatus() {
  Serial.printf("Nama WiFi tersimpan : \"%s\"\n", ssidTersimpan.c_str());
  if (passTersimpan.length() == 0) {
    Serial.println("Password tersimpan  : (kosong)");
  } else {
    Serial.printf("Password tersimpan  : %u karakter (tidak ditampilkan)\n",
                  passTersimpan.length());
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Status koneksi      : tersambung");
    Serial.printf("Alamat IP           : %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Kekuatan sinyal     : %d dBm\n", WiFi.RSSI());
  } else {
    Serial.printf("Status koneksi      : tidak tersambung (status %d)\n", (int)WiFi.status());
  }
}

void tampilkanBantuan() {
  Serial.println("Perintah (akhiri dengan Enter):");
  Serial.println("  ssid <nama>   simpan nama WiFi (peka huruf besar-kecil)");
  Serial.println("  pass <sandi>  simpan password WiFi");
  Serial.println("  sambung       sambungkan memakai data tersimpan");
  Serial.println("  putus         putuskan koneksi");
  Serial.println("  status        tampilkan data tersimpan dan status koneksi");
  Serial.println("  lupa          hapus nama dan password dari NVS");
  Serial.println("  bantuan       tampilkan daftar ini");
}

// ---------- Pengolah perintah ----------
void prosesPerintah(String teks) {
  // Pisahkan kata pertama (perintah) dari sisanya (argumen)
  String perintah = teks;
  String argumen = "";
  int spasi = teks.indexOf(' ');
  if (spasi >= 0) {
    perintah = teks.substring(0, spasi);
    argumen = teks.substring(spasi + 1);
    while (argumen.startsWith(" ")) {
      argumen.remove(0, 1);   // buang spasi berlebih di depan argumen
    }
  }
  perintah.toLowerCase();   // perintah tidak peka huruf besar-kecil, argumen tetap apa adanya

  if (perintah == "ssid") {
    if (argumen.length() < 1 || argumen.length() > 32) {
      Serial.println("Nama WiFi harus 1-32 karakter. Contoh: ssid RumahBudi");
    } else if (simpanTeks("ssid", argumen)) {
      ssidTersimpan = argumen;
      Serial.printf("OK: nama WiFi disimpan: \"%s\"\n", ssidTersimpan.c_str());
    } else {
      Serial.println("Gagal menyimpan ke NVS!");
    }

  } else if (perintah == "pass") {
    if (argumen.length() > 0 && (argumen.length() < 8 || argumen.length() > 64)) {
      Serial.println("Password WPA2 harus 8-64 karakter (atau kosong untuk WiFi terbuka).");
    } else if (simpanTeks("pass", argumen)) {
      passTersimpan = argumen;
      Serial.printf("OK: password disimpan (%u karakter)\n", passTersimpan.length());
    } else {
      Serial.println("Gagal menyimpan ke NVS!");
    }

  } else if (perintah == "sambung") {
    mulaiMenyambung();

  } else if (perintah == "putus") {
    WiFi.disconnect();          // WiFi tetap menyala, auto reconnect tidak berjalan
    sedangMenyambung = false;
    Serial.println("Koneksi diputus. Ketik sambung untuk menyambung lagi.");

  } else if (perintah == "status") {
    tampilkanStatus();

  } else if (perintah == "lupa") {
    prefs.begin(NAMESPACE_WIFI, false);
    prefs.clear();              // hapus semua key di namespace wifi-kit
    prefs.end();
    ssidTersimpan = "";
    passTersimpan = "";
    WiFi.disconnect(false, true);   // putus + hapus salinan milik driver WiFi
    sedangMenyambung = false;
    Serial.println("Nama dan password WiFi sudah dihapus dari NVS.");

  } else if (perintah == "bantuan") {
    tampilkanBantuan();

  } else {
    Serial.printf("Perintah \"%s\" tidak dikenal. Ketik bantuan.\n", perintah.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);

  Serial.println();
  Serial.println("BAB 42 - Program 5: Ganti WiFi Lewat Serial Monitor");

  WiFi.mode(WIFI_STA);
  muatKredensial();

  if (ssidTersimpan.length() > 0) {
    Serial.printf("Data tersimpan: nama WiFi \"%s\", password %u karakter.\n",
                  ssidTersimpan.c_str(), passTersimpan.length());
    mulaiMenyambung();
  } else {
    Serial.println("Belum ada nama WiFi tersimpan.");
    Serial.println("Ketik: ssid <nama WiFi>, lalu pass <password>, lalu sambung");
    Serial.println("Ketik bantuan untuk daftar perintah.");
  }
}

void loop() {
  // 1) Perintah dari Serial Monitor
  if (bacaBaris()) {
    String teks = baris;
    if (teks.length() > 5 && teks.substring(0, 5).equalsIgnoreCase("pass ")) {
      Serial.println("> pass ********");   // jangan menampilkan password
    } else {
      Serial.printf("> %s\n", baris);      // tampilkan ulang perintah yang diketik
    }
    prosesPerintah(teks);
  }

  // 2) Pantau perubahan status koneksi
  bool tersambung = (WiFi.status() == WL_CONNECTED);
  if (tersambung != tersambungSebelumnya) {
    tersambungSebelumnya = tersambung;
    digitalWrite(LED_BIRU, tersambung ? HIGH : LOW);
    if (tersambung) {
      sedangMenyambung = false;
      Serial.printf("Tersambung! IP %s, RSSI %d dBm\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
      Serial.println("WiFi terputus.");
    }
  }

  // 3) Batas waktu menyambung: beri petunjuk, driver tetap mencoba di latar belakang
  if (sedangMenyambung && millis() - waktuMulaiSambung >= BATAS_MENYAMBUNG) {
    sedangMenyambung = false;
    Serial.println("Belum tersambung setelah 20 detik. Cek nama dan password (ketik status).");
    Serial.println("Ganti dengan ssid/pass lalu sambung. Sementara itu ESP32 tetap mencoba.");
  }
}
