/*
 * BAB 43 - Program 3: Hotspot yang Bisa Diatur dari Serial Monitor
 * Pengaturan hotspot diubah lewat perintah di Serial Monitor
 * (line ending: New Line), disimpan di NVS, lalu dipakai lagi
 * setiap kali ESP32 menyala.
 */

#include <WiFi.h>
#include <Preferences.h>

// ── Nilai bawaan (dipakai kalau NVS masih kosong) ──────────────────
const char *SSID_BAWAAN  = "Bluino-AP-43";
const char *SANDI_BAWAAN = "bluino123";
const int CHANNEL_BAWAAN = 1;
const int MAKS_BAWAAN    = 4;

// ── Pengaturan yang sedang dipakai ─────────────────────────────────
String apSsid;
String apSandi;
int apChannel;
bool apSembunyi;
int apMaksPerangkat;

Preferences prefs;
String bufferPerintah = "";

// Baca pengaturan dari NVS (namespace "hotspot")
void muatPengaturan() {
  prefs.begin("hotspot", true);  // true = hanya baca
  apSsid          = prefs.getString("ssid", SSID_BAWAAN);
  apSandi         = prefs.getString("sandi", SANDI_BAWAAN);
  apChannel       = prefs.getInt("channel", CHANNEL_BAWAAN);
  apSembunyi      = prefs.getBool("sembunyi", false);
  apMaksPerangkat = prefs.getInt("maks", MAKS_BAWAAN);
  prefs.end();

  // Jaga-jaga kalau isi NVS tidak masuk akal
  if (apSsid.length() < 1 || apSsid.length() > 32) apSsid = SSID_BAWAAN;
  if (apSandi.length() < 8 || apSandi.length() > 63) apSandi = SANDI_BAWAAN;
  if (apChannel < 1 || apChannel > 11) apChannel = CHANNEL_BAWAAN;
  if (apMaksPerangkat < 1 || apMaksPerangkat > 10) apMaksPerangkat = MAKS_BAWAAN;
}

// Tulis pengaturan ke NVS
void simpanPengaturan() {
  prefs.begin("hotspot", false);  // false = baca-tulis
  prefs.putString("ssid", apSsid);
  prefs.putString("sandi", apSandi);
  prefs.putInt("channel", apChannel);
  prefs.putBool("sembunyi", apSembunyi);
  prefs.putInt("maks", apMaksPerangkat);
  prefs.end();
}

// Hapus semua isi namespace "hotspot" (kembali ke nilai bawaan)
void hapusPengaturan() {
  prefs.begin("hotspot", false);
  prefs.clear();
  prefs.end();
}

void tampilkanStatus() {
  Serial.println("----- Pengaturan hotspot -----");
  Serial.printf("SSID            : %s\n", apSsid.c_str());
  Serial.printf("Kata sandi      : %s\n", apSandi.c_str());
  Serial.printf("Channel         : %d\n", apChannel);
  Serial.printf("Sembunyikan SSID: %s\n", apSembunyi ? "ya" : "tidak");
  Serial.printf("Maks. perangkat : %d\n", apMaksPerangkat);
  Serial.printf("IP hotspot      : %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("Tersambung      : %d perangkat\n", WiFi.softAPgetStationNum());
  Serial.println("------------------------------");
}

void tampilkanBantuan() {
  Serial.println("Daftar perintah:");
  Serial.println("  status              tampilkan pengaturan");
  Serial.println("  ssid <nama>         ganti nama hotspot (1-32 karakter)");
  Serial.println("  sandi <kata sandi>  ganti kata sandi (8-63 karakter)");
  Serial.println("  channel <1-11>      ganti channel");
  Serial.println("  maks <1-10>         jumlah perangkat maksimum");
  Serial.println("  sembunyi <ya/tidak> sembunyikan nama hotspot");
  Serial.println("  simpan              simpan ke NVS lalu restart");
  Serial.println("  reset               kembali ke pengaturan bawaan lalu restart");
}

void prosesPerintah(String baris) {
  baris.trim();
  if (baris.length() == 0) return;

  // Pisahkan kata pertama (perintah) dari sisanya (nilai)
  String perintah = baris;
  String nilai = "";
  int posisiSpasi = baris.indexOf(' ');
  if (posisiSpasi > 0) {
    perintah = baris.substring(0, posisiSpasi);
    nilai = baris.substring(posisiSpasi + 1);
    nilai.trim();
  }
  perintah.toLowerCase();  // perintah tidak peka huruf besar-kecil

  if (perintah == "status") {
    tampilkanStatus();

  } else if (perintah == "ssid") {
    if (nilai.length() < 1 || nilai.length() > 32) {
      Serial.println("Gagal: SSID harus 1-32 karakter.");
    } else {
      apSsid = nilai;
      Serial.printf("SSID -> \"%s\". Ketik simpan agar berlaku.\n", apSsid.c_str());
    }

  } else if (perintah == "sandi") {
    if (nilai.length() < 8 || nilai.length() > 63) {
      Serial.println("Gagal: kata sandi harus 8-63 karakter.");
    } else {
      apSandi = nilai;
      Serial.println("Kata sandi diganti. Ketik simpan agar berlaku.");
    }

  } else if (perintah == "channel") {
    int ch = nilai.toInt();  // teks bukan angka -> 0
    if (ch < 1 || ch > 11) {
      Serial.println("Gagal: channel harus 1-11.");
    } else {
      apChannel = ch;
      Serial.printf("Channel -> %d. Ketik simpan agar berlaku.\n", apChannel);
    }

  } else if (perintah == "maks") {
    int n = nilai.toInt();
    if (n < 1 || n > 10) {
      Serial.println("Gagal: maks harus 1-10.");
    } else {
      apMaksPerangkat = n;
      Serial.printf("Maks. perangkat -> %d. Ketik simpan agar berlaku.\n", apMaksPerangkat);
    }

  } else if (perintah == "sembunyi") {
    nilai.toLowerCase();
    if (nilai == "ya" || nilai == "tidak") {
      apSembunyi = (nilai == "ya");
      Serial.printf("Sembunyikan SSID -> %s. Ketik simpan agar berlaku.\n", nilai.c_str());
    } else {
      Serial.println("Gagal: tulis sembunyi ya atau sembunyi tidak.");
    }

  } else if (perintah == "simpan") {
    simpanPengaturan();
    Serial.println("Pengaturan disimpan. ESP32 restart dalam 1 detik...");
    delay(1000);
    ESP.restart();

  } else if (perintah == "reset") {
    hapusPengaturan();
    Serial.println("Pengaturan dihapus. ESP32 restart dalam 1 detik...");
    delay(1000);
    ESP.restart();

  } else if (perintah == "bantuan") {
    tampilkanBantuan();

  } else {
    Serial.printf("Perintah \"%s\" tidak dikenal. Ketik bantuan.\n", perintah.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== BAB 43 Program 3: Hotspot yang Bisa Diatur ===");

  muatPengaturan();

  WiFi.mode(WIFI_AP);
  bool berhasil = WiFi.softAP(apSsid.c_str(), apSandi.c_str(), apChannel,
                              apSembunyi ? 1 : 0, apMaksPerangkat);
  if (berhasil) {
    Serial.println("Hotspot aktif.");
  } else {
    Serial.println("Gagal membuat hotspot! Ketik reset untuk kembali ke pengaturan bawaan.");
  }

  tampilkanStatus();
  Serial.println("Ketik bantuan untuk melihat daftar perintah.");
}

void loop() {
  // Kumpulkan huruf sampai Enter, lalu proses (pola BAB 16)
  while (Serial.available() > 0) {
    char huruf = Serial.read();
    if (huruf == '\n') {
      prosesPerintah(bufferPerintah);
      bufferPerintah = "";
    } else if (huruf != '\r' && bufferPerintah.length() < 100) {
      bufferPerintah += huruf;
    }
  }
}
