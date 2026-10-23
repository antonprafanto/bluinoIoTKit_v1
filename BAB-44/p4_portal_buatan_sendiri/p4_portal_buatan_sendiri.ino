/*
 * BAB 44 - Program 4: Captive Portal Buatan Sendiri
 * Meniru cara kerja WiFiManager hanya dengan library bawaan esp32:
 *   WiFi        : hotspot portal dan koneksi ke WiFi rumah
 *   DNSServer   : menjawab SEMUA nama domain dengan alamat ESP32
 *   WebServer   : halaman pengaturan + pengalihan (redirect) ke halaman itu
 *   Preferences : menyimpan nama dan password WiFi ke NVS
 *
 * Rangkaian: tidak perlu (LED biru bawaan di IO2, tombol BOOT di IO0)
 * Alat uji : Serial Monitor (115200) + HP atau laptop
 * Hapus WiFi tersimpan: tahan tombol BOOT 3 detik setelah program berjalan.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

#define LED_BIRU     2
#define TOMBOL_BOOT  0

const char* PORTAL_SSID = "Bluino-DIY";
const char* PORTAL_PASS = "bluino123";              // minimal 8 karakter
const unsigned long BATAS_SAMBUNG_MS = 20000;       // coba sambung paling lama 20 detik
const unsigned long BATAS_PORTAL_MS  = 180000;      // portal ditutup setelah 3 menit diam

WebServer server(80);
DNSServer dns;
Preferences prefs;

bool modePortal = false;
String pilihanWiFi;                  // <option> hasil pemindaian WiFi
unsigned long waktuAktivitas = 0;    // kapan halaman portal terakhir dibuka
bool restartTerjadwal = false;
unsigned long waktuSimpan = 0;
unsigned long mulaiTekanBoot = 0;
unsigned long waktuLaporan = 0;

// Bagian awal dan akhir halaman pengaturan (HTML)
const char HTML_AWAL[] = R"rawliteral(<!DOCTYPE html>
<html lang="id"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pengaturan WiFi</title>
<style>
body{font-family:Arial,sans-serif;max-width:420px;margin:auto;padding:16px}
label{display:block;margin-top:14px}
select,input,button{width:100%;padding:10px;font-size:1em;box-sizing:border-box}
button{margin-top:20px;background:#1976d2;color:#fff;border:0;border-radius:6px}
</style></head><body>
<h2>Pengaturan WiFi</h2>
<p>Bluino IoT Kit. Hanya WiFi 2,4 GHz yang didukung.</p>
<form method="POST" action="/simpan">
<label for="ssid">Pilih WiFi</label>
<select id="ssid" name="ssid">)rawliteral";

const char HTML_AKHIR[] = R"rawliteral(</select>
<label for="ssid2">Atau ketik nama WiFi (kalau tidak ada di daftar)</label>
<input id="ssid2" name="ssid2" maxlength="32">
<label for="pass">Password WiFi</label>
<input id="pass" name="pass" type="password" maxlength="63">
<button type="submit">Simpan dan Sambungkan</button>
</form></body></html>)rawliteral";

// Ganti karakter khusus HTML supaya nama WiFi aneh tidak merusak halaman
String amankanHtml(String teks) {
  teks.replace("&", "&amp;");
  teks.replace("<", "&lt;");
  teks.replace(">", "&gt;");
  teks.replace("\"", "&quot;");
  return teks;
}

String halamanPesan(const String& isi) {
  return "<!DOCTYPE html><html lang=\"id\"><head><meta charset=\"UTF-8\">"
         "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
         "<title>Pengaturan WiFi</title></head>"
         "<body style=\"font-family:Arial,sans-serif;padding:16px\"><p>" + isi +
         "</p></body></html>";
}

// GET / : tampilkan halaman pengaturan
void tampilkanHalaman() {
  waktuAktivitas = millis();         // ada yang membuka halaman: hitungan 3 menit diulang
  String html = HTML_AWAL;
  html += pilihanWiFi;
  html += HTML_AKHIR;
  server.send(200, "text/html", html);
}

// POST /simpan : periksa isian, simpan ke NVS, lalu restart
void simpanWiFi() {
  String ssid = server.arg("ssid2");           // nama yang diketik diutamakan
  if (ssid.length() == 0) ssid = server.arg("ssid");
  String pass = server.arg("pass");

  if (ssid.length() == 0 || ssid.length() > 32) {
    server.send(400, "text/html",
                halamanPesan("Nama WiFi harus 1-32 karakter. <a href=\"/\">Kembali</a>"));
    return;
  }
  if (pass.length() > 0 && (pass.length() < 8 || pass.length() > 63)) {
    server.send(400, "text/html",
                halamanPesan("Password WiFi harus 8-63 karakter, atau kosong untuk WiFi "
                             "tanpa password. <a href=\"/\">Kembali</a>"));
    return;
  }

  prefs.begin("wifi-diy", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();

  Serial.printf("WiFi \"%s\" disimpan ke NVS. Restart dalam 2 detik...\n", ssid.c_str());
  server.send(200, "text/html",
              halamanPesan("Tersimpan! ESP32 akan restart dan menyambung ke WiFi <b>" +
                           amankanHtml(ssid) + "</b>. Kalau gagal, hotspot <b>" +
                           String(PORTAL_SSID) + "</b> akan muncul lagi."));
  restartTerjadwal = true;           // restart di loop(), setelah halaman terkirim
  waktuSimpan = millis();
}

// Alamat lain (termasuk alamat uji HP): alihkan ke halaman portal
void alihkanKePortal() {
  Serial.printf("[HTTP] %s%s -> dialihkan ke portal\n",
                server.hostHeader().c_str(), server.uri().c_str());
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void bukaPortal() {
  Serial.println("Membuka portal...");
  WiFi.disconnect();                 // hentikan percobaan menyambung yang gagal
  WiFi.mode(WIFI_AP_STA);            // AP untuk portal, STA untuk memindai WiFi
  if (!WiFi.softAP(PORTAL_SSID, PORTAL_PASS)) {
    Serial.println("Gagal membuat hotspot! Periksa panjang PORTAL_PASS (8-63 karakter).");
    return;
  }
  delay(500);

  // Pindai WiFi sekali saja, lalu simpan hasilnya sebagai <option>
  int n = WiFi.scanNetworks();
  Serial.printf("Memindai WiFi di sekitar... %d jaringan ditemukan.\n", n < 0 ? 0 : n);
  pilihanWiFi = "";
  for (int i = 0; i < n; i++) {
    String nama = WiFi.SSID(i);
    if (nama.length() == 0) continue;          // lewati WiFi tersembunyi
    String aman = amankanHtml(nama);
    pilihanWiFi += "<option value=\"" + aman + "\">" + aman + " (" +
                   String(WiFi.RSSI(i)) + " dBm)</option>";
  }
  WiFi.scanDelete();                           // bebaskan memori hasil pemindaian

  // DNS: jawab SEMUA nama domain ("*") dengan alamat IP ESP32
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, tampilkanHalaman);
  server.on("/simpan", HTTP_POST, simpanWiFi);
  server.onNotFound(alihkanKePortal);
  server.begin();

  modePortal = true;
  waktuAktivitas = millis();
  Serial.println("Portal siap:");
  Serial.printf("  Hotspot : %s (password: %s)\n", PORTAL_SSID, PORTAL_PASS);
  Serial.println("  Alamat  : http://" + WiFi.softAPIP().toString());
}

void periksaTombolBoot() {
  if (digitalRead(TOMBOL_BOOT) == HIGH) {
    mulaiTekanBoot = 0;
    return;
  }
  if (mulaiTekanBoot == 0) {
    mulaiTekanBoot = millis();
  }
  if (millis() - mulaiTekanBoot >= 3000) {
    Serial.println("Menghapus WiFi tersimpan, lalu restart...");
    prefs.begin("wifi-diy", false);
    prefs.clear();
    prefs.end();
    delay(500);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  pinMode(TOMBOL_BOOT, INPUT_PULLUP);

  Serial.println();
  Serial.println("BAB 44 - Program 4: Captive Portal Buatan Sendiri");

  // Kita menyimpan WiFi sendiri di NVS, jadi driver WiFi tidak perlu ikut
  // menyimpannya. Harus dipanggil sebelum fungsi WiFi lain.
  WiFi.persistent(false);

  prefs.begin("wifi-diy", false);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() == 0) {
    Serial.println("Belum ada WiFi tersimpan.");
    bukaPortal();
    return;
  }

  Serial.printf("Mencoba menyambung ke \"%s\"...\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - mulai < BATAS_SAMBUNG_MS) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Tersambung! Alamat IP: " + WiFi.localIP().toString());
    Serial.println("Tahan tombol BOOT 3 detik untuk menghapus WiFi tersimpan.");
  } else {
    Serial.println("Gagal tersambung dalam 20 detik.");
    bukaPortal();
  }
}

void loop() {
  if (modePortal) {
    server.handleClient();                       // layani HP
    digitalWrite(LED_BIRU, (millis() / 100) % 2);   // kedip cepat

    if (restartTerjadwal && millis() - waktuSimpan >= 2000) {
      ESP.restart();                             // halaman sukses sudah terkirim
    }
    if (!restartTerjadwal && millis() - waktuAktivitas >= BATAS_PORTAL_MS) {
      Serial.println("Portal 3 menit tanpa aktivitas. Restart untuk mencoba lagi...");
      delay(500);
      ESP.restart();
    }
  } else {
    digitalWrite(LED_BIRU, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
    if (millis() - waktuLaporan >= 10000) {
      waktuLaporan = millis();
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Masih tersambung, sinyal %d dBm\n", WiFi.RSSI());
      } else {
        Serial.println("WiFi terputus, ESP32 mencoba menyambung lagi...");
      }
    }
  }

  periksaTombolBoot();
}
