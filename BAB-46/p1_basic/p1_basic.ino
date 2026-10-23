/*
 * BAB 46 - Program 1: Web Server Pertama
 * - http://<IP>/            -> halaman beranda (HTML tetap, disimpan di flash)
 * - http://<IP>/halo?nama=X -> halaman sapaan yang membaca parameter "nama"
 * - alamat lain             -> halaman 404
 * Setiap request dicatat di Serial Monitor.
 * Rangkaian: tidak perlu (cukup ESP32 dan WiFi 2,4 GHz)
 */

#include <WiFi.h>
#include <WebServer.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

WebServer server(80);  // port 80 = port bawaan HTTP

// Halaman beranda. Karena "const", teks ini tetap di flash, bukan di RAM.
const char HALAMAN_BERANDA[] = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Bluino</title>
<style>
  body { font-family: Arial, sans-serif; background: #f2f4f7; color: #222;
         max-width: 480px; margin: 0 auto; padding: 16px; }
  .kartu { background: #fff; border-radius: 12px; padding: 16px;
           box-shadow: 0 1px 4px rgba(0, 0, 0, 0.15); }
  h1 { font-size: 1.4em; color: #1565c0; margin-top: 0; }
  input, button { font-size: 1em; padding: 10px; border-radius: 8px; }
  input { border: 1px solid #999; width: 60%; }
  button { border: none; background: #1565c0; color: #fff; }
  a { color: #1565c0; }
</style>
</head>
<body>
<div class="kartu">
  <h1>Halo dari ESP32!</h1>
  <p>Halaman ini dikirim oleh web server di kit Bluino.</p>
  <form action="/halo" method="GET">
    <p><label for="nama">Siapa namamu?</label></p>
    <input id="nama" name="nama" maxlength="30" required>
    <button type="submit">Kirim</button>
  </form>
  <p>Atau coba buka <a href="/tidak-ada">alamat yang tidak ada</a>.</p>
</div>
</body>
</html>
)rawliteral";

// Mengganti karakter khusus HTML supaya teks dari pengguna
// tampil apa adanya dan tidak dianggap sebagai kode HTML
String amankanHtml(String teks) {
  teks.replace("&", "&amp;");
  teks.replace("<", "&lt;");
  teks.replace(">", "&gt;");
  teks.replace("\"", "&quot;");
  return teks;
}

// Mencetak satu baris catatan request di Serial Monitor
void catatRequest(int kodeStatus) {
  Serial.printf("[HTTP] %s %s -> %d (dari %s)\n",
                http_method_str(server.method()), server.uri().c_str(),
                kodeStatus, server.client().remoteIP().toString().c_str());
}

// Handler untuk GET /
void halamanBeranda() {
  server.send(200, "text/html", HALAMAN_BERANDA);
  catatRequest(200);
}

// Handler untuk GET /halo?nama=...
void halamanHalo() {
  String nama = "tamu";  // dipakai kalau parameter "nama" tidak ada atau kosong
  if (server.hasArg("nama") && server.arg("nama").length() > 0) {
    nama = server.arg("nama").substring(0, 30);  // ambil paling banyak 30 karakter
  }

  // Halaman kecil yang isinya berubah-ubah disusun dengan String
  String html;
  html.reserve(500);  // pesan memori sekaligus supaya tidak berkali-kali
  html += "<!DOCTYPE html><html lang=\"id\"><head><meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<title>Halo</title></head>";
  html += "<body style=\"font-family: Arial, sans-serif; padding: 16px;\">";
  html += "<h1>Halo, ";
  html += amankanHtml(nama);
  html += "!</h1><p>ESP32 sudah menyala selama ";
  html += String(millis() / 1000);
  html += " detik.</p><p><a href=\"/\">Kembali ke beranda</a></p></body></html>";

  server.send(200, "text/html", html);
  catatRequest(200);
  Serial.printf("       nama = %s\n", nama.c_str());
}

// Handler untuk semua alamat yang tidak didaftarkan
void halaman404() {
  String html;
  html.reserve(400);
  html += "<!DOCTYPE html><html lang=\"id\"><head><meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<title>404</title></head>";
  html += "<body style=\"font-family: Arial, sans-serif; padding: 16px;\">";
  html += "<h1>404 - Tidak Ditemukan</h1><p>Alamat <b>";
  html += amankanHtml(server.uri());
  html += "</b> tidak ada di ESP32 ini.</p><p><a href=\"/\">Kembali ke beranda</a></p></body></html>";

  server.send(404, "text/html", html);
  catatRequest(404);
}

void sambungkanWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(NAMA_WIFI, PASSWORD_WIFI);
  Serial.printf("Menyambung ke WiFi \"%s\"", NAMA_WIFI);

  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - mulai > 20000) {  // menyerah setelah 20 detik
      Serial.println("\nGagal tersambung. Periksa nama/password WiFi dan pastikan 2,4 GHz.");
      Serial.println("ESP32 restart dalam 5 detik...");
      delay(5000);
      ESP.restart();
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println(" tersambung!");
  WiFi.setSleep(false);  // matikan hemat daya WiFi supaya halaman lebih cepat dijawab
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor bersiap
  Serial.println("\nBAB 46 - Program 1: Web Server Pertama");

  sambungkanWiFi();

  // Daftar route: alamat + method + fungsi handler
  server.on("/", HTTP_GET, halamanBeranda);
  server.on("/halo", HTTP_GET, halamanHalo);
  server.onNotFound(halaman404);
  server.begin();

  Serial.print("Web server siap. Buka di browser: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  server.handleClient();  // layani request yang masuk
}
