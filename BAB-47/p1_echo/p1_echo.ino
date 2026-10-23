/*
 * BAB 47 - Program 1: Echo WebSocket
 * - GET /           -> halaman uji (HTML + JavaScript), lewat port 80
 * - ws://<IP>:81/   -> WebSocket: setiap teks yang diterima dikirim balik (echo)
 * LED biru (IO2) menyala selama ada minimal satu browser yang tersambung.
 * Rangkaian: tidak perlu (LED biru bawaan board di IO2).
 *   Opsional: LED3 (biru) -> IO2 supaya LED biru di kit ikut menyala.
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu
const char *NAMA_MDNS = "bluino";                // nama .local, tanpa ".local" (BAB 45)

#define LED_BIRU 2  // LED biru bawaan board (dan LED3 kit) -> IO2

WebServer server(80);     // port 80: mengirim halaman web
WebSocketsServer ws(81);  // port 81: pesan WebSocket dua arah

// Halaman uji: HTML + CSS + JavaScript dalam satu raw string literal (BAB 46).
// Tidak memakai CDN, jadi tetap berjalan tanpa internet.
const char HALAMAN_ECHO[] = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Echo WebSocket</title>
<style>
  * { box-sizing: border-box; }
  body { font-family: Arial, Helvetica, sans-serif; background: #f2f4f7;
         color: #222; margin: 0; padding: 16px; }
  .wadah { max-width: 520px; margin: 0 auto; }
  header { display: flex; justify-content: space-between; align-items: center; }
  h1 { font-size: 1.3em; margin: 0; color: #1565c0; }
  .koneksi { font-size: 0.85em; padding: 4px 10px; border-radius: 999px;
             background: #e0e0e0; color: #555; }
  .koneksi.ok { background: #e8f5e9; color: #2e7d32; }
  .koneksi.putus { background: #ffebee; color: #c62828; }
  #log { background: #fff; border-radius: 12px; padding: 12px; height: 260px;
         overflow-y: auto; margin: 16px 0 10px; font-family: monospace;
         font-size: 0.9em; box-shadow: 0 1px 3px rgba(0, 0, 0, 0.12); }
  .kirim { color: #1565c0; }
  .terima { color: #2e7d32; }
  .sistem { color: #888; font-style: italic; }
  .baris { display: flex; gap: 8px; }
  input { flex: 1; min-width: 0; font-size: 1em; padding: 10px;
          border: 1px solid #bbb; border-radius: 8px; }
  button { font-size: 1em; padding: 10px 16px; border: none; border-radius: 8px;
           background: #1565c0; color: #fff; cursor: pointer; }
  button:disabled { background: #90a4ae; cursor: not-allowed; }
</style>
</head>
<body>
<div class="wadah">
  <header>
    <h1>Echo WebSocket</h1>
    <span id="koneksi" class="koneksi">Menyambung...</span>
  </header>
  <div id="log"></div>
  <div class="baris">
    <input id="teks" type="text" maxlength="100" placeholder="Ketik pesan, lalu Enter">
    <button id="tombol" onclick="kirim()" disabled>Kirim</button>
  </div>
</div>

<script>
function el(id) { return document.getElementById(id); }
let ws;

// Menambah satu baris (dengan jam) ke kotak log
function catat(teks, kelas) {
  const baris = document.createElement("div");
  baris.className = kelas;
  baris.textContent = new Date().toLocaleTimeString("id-ID") + "  " + teks;
  el("log").appendChild(baris);
  el("log").scrollTop = el("log").scrollHeight;
}

function aturKoneksi(ok) {
  el("koneksi").textContent = ok ? "● Terhubung" : "● Terputus";
  el("koneksi").className = "koneksi " + (ok ? "ok" : "putus");
  el("tombol").disabled = !ok;
}

// Membuka WebSocket ke port 81 di alamat yang sama dengan halaman ini
function sambungkan() {
  ws = new WebSocket("ws://" + location.hostname + ":81/");
  ws.onopen = function () {
    aturKoneksi(true);
    catat("Terhubung ke ESP32", "sistem");
  };
  ws.onmessage = function (e) {
    catat("◀ " + e.data, "terima");
  };
  ws.onclose = function () {
    aturKoneksi(false);
    catat("Koneksi terputus. Mencoba lagi dalam 3 detik...", "sistem");
    setTimeout(sambungkan, 3000);  // sambung ulang otomatis
  };
}

function kirim() {
  const teks = el("teks").value.trim();
  if (teks === "" || ws.readyState !== WebSocket.OPEN) return;
  ws.send(teks);
  catat("▶ " + teks, "kirim");
  el("teks").value = "";
}

el("teks").addEventListener("keydown", function (e) {
  if (e.key === "Enter") kirim();
});

sambungkan();
</script>
</body>
</html>
)rawliteral";

// LED biru menyala kalau ada minimal satu browser yang tersambung
void perbaruiLed() {
  digitalWrite(LED_BIRU, ws.connectedClients() > 0 ? HIGH : LOW);
}

// Dipanggil library (dari dalam ws.loop()) setiap ada kejadian WebSocket
void tanganiWebSocket(uint8_t nomor, WStype_t jenis, uint8_t *data, size_t panjang) {
  switch (jenis) {
    case WStype_CONNECTED: {
      Serial.printf("[WS] Klien #%u terhubung dari %s (total %d klien)\n", nomor,
                    ws.remoteIP(nomor).toString().c_str(), ws.connectedClients());
      char sapaan[48];
      snprintf(sapaan, sizeof(sapaan), "Halo! Kamu klien nomor %u", nomor);
      ws.sendTXT(nomor, sapaan);  // sapa browser yang baru tersambung saja
      perbaruiLed();
      break;
    }

    case WStype_DISCONNECTED:
      Serial.printf("[WS] Klien #%u terputus (sisa %d klien)\n", nomor, ws.connectedClients());
      perbaruiLed();
      break;

    case WStype_TEXT: {
      if (panjang == 0) break;  // abaikan pesan kosong
      // data sudah diakhiri '\0' oleh library, jadi aman dicetak sebagai teks
      Serial.printf("[WS] Pesan dari klien #%u: %s\n", nomor, (const char *)data);
      String balasan = "ESP32 menerima: ";
      balasan += (const char *)data;
      ws.sendTXT(nomor, balasan);  // echo: kirim balik HANYA ke pengirimnya
      break;
    }

    default:  // PONG, PING, BIN, dan lain-lain tidak dipakai di program ini
      break;
  }
}

// Handler untuk GET /
void halamanUtama() {
  server.send(200, "text/html", HALAMAN_ECHO);
}

void halaman404() {
  server.send(404, "text/plain", "404 - alamat tidak ditemukan");
}

void sambungkanWiFi() {
  WiFi.setHostname(NAMA_MDNS);  // nama di daftar perangkat router — SEBELUM WiFi.mode()
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
  WiFi.setSleep(false);  // matikan hemat daya WiFi supaya pesan lebih cepat sampai
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBAB 47 - Program 1: Echo WebSocket");

  pinMode(LED_BIRU, OUTPUT);  // pinMode dulu, baru digitalWrite (core 3.x)
  digitalWrite(LED_BIRU, LOW);

  sambungkanWiFi();
  bool mdnsAktif = MDNS.begin(NAMA_MDNS);
  if (mdnsAktif) {
    MDNS.addService("http", "tcp", 80);
  }

  server.on("/", HTTP_GET, halamanUtama);
  server.onNotFound(halaman404);
  server.begin();  // port 80

  ws.begin();  // port 81
  ws.onEvent(tanganiWebSocket);

  String ip = WiFi.localIP().toString();
  Serial.printf("Halaman web : http://%s/\n", ip.c_str());
  if (mdnsAktif) {
    Serial.printf("              atau http://%s.local/\n", NAMA_MDNS);
  }
  Serial.printf("WebSocket   : ws://%s:81/\n", ip.c_str());
}

void loop() {
  server.handleClient();  // layani halaman web (port 80)
  ws.loop();              // layani WebSocket (port 81): koneksi baru dan pesan masuk
}
