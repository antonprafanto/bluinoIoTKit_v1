/*
 * BAB 45 - Program 3: Ganti Nama dan Cari Perangkat Lain
 * Perintah lewat Serial Monitor (line ending: New Line):
 *   bantuan          -> daftar perintah
 *   status           -> nama, alamat IP, status WiFi
 *   nama <nama-baru> -> ganti nama mDNS (disimpan di NVS)
 *   cari             -> cari perangkat yang punya web server (_http._tcp)
 *   ip <nama>        -> cari alamat IP dari sebuah nama .local
 *
 * Rangkaian: tidak perlu (LED biru bawaan board di IO2).
 *   Opsional: LED3 (biru) → IO2.
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Preferences.h>

const char* WIFI_SSID     = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char* WIFI_PASSWORD = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

const char* NAMA_BAWAAN = "bluino";  // dipakai kalau belum ada nama tersimpan

#define LED_BIRU 2  // LED biru bawaan board (dan LED3 kit) → IO2

WebServer server(80);
Preferences prefs;

String namaMdns;          // nama yang diminta (tersimpan di NVS)
bool mdnsAktif = false;
String bufferPerintah;    // menampung huruf dari Serial Monitor sampai Enter

// Nama yang benar-benar dipakai mDNS (bisa "...-2" kalau namanya kembar)
String namaMdnsAktif() {
  char nama[MDNS_NAME_BUF_LEN];
  if (mdns_hostname_get(nama) == ESP_OK) {
    return String(nama);
  }
  return namaMdns;
}

// Aturan nama: 1-31 karakter, hanya a-z, 0-9, dan '-',
// tidak boleh diawali atau diakhiri '-'
bool namaValid(const String& nama) {
  if (nama.length() < 1 || nama.length() > 31) return false;
  if (nama.startsWith("-") || nama.endsWith("-")) return false;
  for (unsigned int i = 0; i < nama.length(); i++) {
    char c = nama[i];
    bool boleh = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    if (!boleh) return false;
  }
  return true;
}

// Rapikan masukan: buang spasi di tepi, jadikan huruf kecil, buang ".local"
String rapikanNama(String nama) {
  nama.trim();
  nama.toLowerCase();
  if (nama.endsWith(".local")) {
    nama.remove(nama.length() - 6);
  }
  return nama;
}

// (Ulang) jalankan mDNS dengan nama di variabel namaMdns
bool mulaiMdns() {
  MDNS.end();  // hentikan dulu kalau sudah berjalan (aman walau belum pernah dimulai)
  if (!MDNS.begin(namaMdns.c_str())) {
    return false;
  }
  MDNS.addService("http", "tcp", 80);  // layanan harus diumumkan ulang setelah end()
  MDNS.addServiceTxt("http", "tcp", "path", "/");
  return true;
}

// Halaman web sederhana (detail web server ada di BAB 46)
void handleRoot() {
  String html;
  html += "<!DOCTYPE html><html lang=\"id\"><head><meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<title>ESP32 Bluino</title></head>";
  html += "<body style=\"font-family:sans-serif;max-width:480px;margin:auto;padding:16px\">";
  html += "<h1>Halo dari " + namaMdnsAktif() + ".local!</h1>";
  html += "<p>Alamat IP: <b>" + WiFi.localIP().toString() + "</b></p>";
  html += "<p>Nama ini bisa diganti lewat Serial Monitor dengan perintah <code>nama</code>.</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void tampilkanBantuan() {
  Serial.println("Perintah yang tersedia:");
  Serial.println("  bantuan          -> tampilkan daftar perintah ini");
  Serial.println("  status           -> tampilkan nama, alamat IP, dan status WiFi");
  Serial.println("  nama <nama-baru> -> ganti nama mDNS, contoh: nama bluino-andi");
  Serial.println("  cari             -> cari perangkat yang punya web server (_http._tcp)");
  Serial.println("  ip <nama>        -> cari alamat IP sebuah nama, contoh: ip bluino-budi");
}

void tampilkanStatus() {
  Serial.printf("Nama tersimpan : %s.local\n", namaMdns.c_str());
  if (mdnsAktif) {
    Serial.printf("Nama aktif     : %s.local\n", namaMdnsAktif().c_str());
  } else {
    Serial.println("Nama aktif     : (mDNS tidak aktif)");
  }
  Serial.printf("WiFi           : %s\n", (WiFi.status() == WL_CONNECTED) ? "terhubung" : "terputus");
  Serial.printf("Alamat IP      : %s\n", WiFi.localIP().toString().c_str());
}

void gantiNama(const String& masukan) {
  String namaBaru = rapikanNama(masukan);
  if (namaBaru.length() == 0) {
    Serial.printf("Nama sekarang: %s.local. Untuk mengganti: nama <nama-baru>\n", namaMdns.c_str());
    return;
  }
  if (!namaValid(namaBaru)) {
    Serial.println("Nama tidak valid! Pakai 1-31 huruf kecil, angka, atau tanda hubung (-),");
    Serial.println("dan jangan diawali/diakhiri tanda hubung. Contoh: nama bluino-andi");
    return;
  }

  namaMdns = namaBaru;
  prefs.putString("nama", namaMdns);  // simpan ke NVS supaya tetap ada setelah restart
  mdnsAktif = mulaiMdns();
  if (mdnsAktif) {
    Serial.printf("Nama diganti dan disimpan. Sekarang buka: http://%s.local\n", namaMdns.c_str());
    Serial.println("(Nama di daftar perangkat router baru ikut berubah setelah ESP32 restart.)");
  } else {
    Serial.println("Gagal memulai mDNS dengan nama baru!");
  }
}

void cariLayanan() {
  if (!mdnsAktif) {
    Serial.println("mDNS tidak aktif, tidak bisa mencari.");
    return;
  }
  Serial.println("Mencari perangkat dengan layanan _http._tcp (tunggu 3 detik)...");
  int jumlah = MDNS.queryService("http", "tcp");  // ESP32 menunggu ±3 detik di sini
  if (jumlah == 0) {
    Serial.println("Tidak ditemukan. (ESP32 ini sendiri tidak ikut terhitung.)");
    return;
  }
  Serial.printf("Ditemukan %d perangkat:\n", jumlah);
  for (int i = 0; i < jumlah; i++) {
    Serial.printf("  %d. %s.local | IP: %s | port: %u\n", i + 1,
                  MDNS.hostname(i).c_str(),
                  MDNS.address(i).toString().c_str(),
                  (unsigned int)MDNS.port(i));
  }
}

void cariIp(const String& masukan) {
  String nama = rapikanNama(masukan);
  if (nama.length() == 0) {
    Serial.println("Tulis namanya, contoh: ip bluino-budi");
    return;
  }
  if (!mdnsAktif) {
    Serial.println("mDNS tidak aktif, tidak bisa mencari.");
    return;
  }
  Serial.printf("Mencari %s.local (tunggu sampai 2 detik)...\n", nama.c_str());
  IPAddress ip = MDNS.queryHost(nama, 2000);  // nama TANPA ".local", tunggu maks. 2000 ms
  if (ip == INADDR_NONE) {                    // 0.0.0.0 = tidak ada yang menjawab
    Serial.println("Tidak ditemukan.");
  } else {
    Serial.printf("%s.local -> %s\n", nama.c_str(), ip.toString().c_str());
  }
}

// Pisahkan baris menjadi perintah + isi, lalu jalankan
void prosesPerintah(String baris) {
  baris.trim();
  if (baris.length() == 0) return;  // baris kosong (misalnya sisa '\r') diabaikan
  Serial.print("> ");
  Serial.println(baris);

  String perintah = baris;
  String isi = "";
  int spasi = baris.indexOf(' ');
  if (spasi > 0) {
    perintah = baris.substring(0, spasi);
    isi = baris.substring(spasi + 1);
  }
  perintah.toLowerCase();

  if (perintah == "bantuan") {
    tampilkanBantuan();
  } else if (perintah == "status") {
    tampilkanStatus();
  } else if (perintah == "nama") {
    gantiNama(isi);
  } else if (perintah == "cari") {
    cariLayanan();
  } else if (perintah == "ip") {
    cariIp(isi);
  } else {
    Serial.println("Perintah tidak dikenal. Ketik: bantuan");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  delay(500);
  Serial.println();
  Serial.println("BAB 45 - Program 3: Ganti Nama dan Cari Perangkat Lain");

  // 1. Muat nama tersimpan dari NVS (lihat BAB 39)
  prefs.begin("bab45", false);  // namespace "bab45", mode baca-tulis
  namaMdns = NAMA_BAWAAN;
  if (prefs.isKey("nama")) {
    namaMdns = prefs.getString("nama");
  }
  if (!namaValid(namaMdns)) {
    namaMdns = NAMA_BAWAAN;  // jaga-jaga kalau isi NVS tidak sesuai aturan
  }
  Serial.printf("Nama tersimpan: %s\n", namaMdns.c_str());

  // 2. Sambungkan ke WiFi
  WiFi.setHostname(namaMdns.c_str());  // SEBELUM WiFi.mode()
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");

  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - mulai >= 20000) {  // menyerah setelah 20 detik
      Serial.println();
      Serial.println("Gagal terhubung! Periksa nama dan password WiFi (harus 2,4 GHz).");
      Serial.println("ESP32 restart dalam 5 detik...");
      delay(5000);
      ESP.restart();
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Terhubung! Alamat IP: ");
  Serial.println(WiFi.localIP());

  // 3. Mulai mDNS
  mdnsAktif = mulaiMdns();
  if (mdnsAktif) {
    Serial.printf("mDNS aktif: %s.local\n", namaMdns.c_str());
  } else {
    Serial.println("Gagal memulai mDNS!");
  }

  // 4. Jalankan web server
  server.on("/", handleRoot);
  server.begin();
  Serial.printf("Web server berjalan. Buka di browser: http://%s.local\n", namaMdns.c_str());

  bufferPerintah.reserve(64);
  tampilkanBantuan();
}

void loop() {
  server.handleClient();

  // Baca perintah dari Serial Monitor huruf demi huruf sampai Enter
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      prosesPerintah(bufferPerintah);
      bufferPerintah = "";
    } else if (bufferPerintah.length() < 64) {
      bufferPerintah += c;
    }
  }

  // LED biru menyala selama WiFi terhubung dan mDNS aktif
  digitalWrite(LED_BIRU, (WiFi.status() == WL_CONNECTED && mdnsAktif) ? HIGH : LOW);
}
