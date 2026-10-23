/*
 * BAB 46 - Program 3: Dashboard IoT Interaktif
 * - GET  /             -> halaman dashboard (HTML + CSS + JavaScript, dikirim sekali)
 * - GET  /api/status   -> data sensor + keadaan LED dan relay (JSON)
 * - POST /api/kontrol  -> perintah JSON, misalnya {"led":true} atau {"relay":false}
 * Halaman memperbarui angkanya sendiri dengan fetch() setiap 2 detik.
 * Rangkaian: DHT -> IO27, VR10K -> IO34, LED1 (merah) -> IO4, RELAY -> IO26
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

#define PIN_DHT 27
#define TIPE_DHT DHT11
#define PIN_POT 34    // potensiometer VR10K (ADC1)
#define PIN_LED 4     // LED1 (merah), aktif HIGH
#define PIN_RELAY 26  // RELAY, aktif HIGH

// struct Timer dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

WebServer server(80);
DHT dht(PIN_DHT, TIPE_DHT);
Timer timerDHT = {2000, 0};  // baca DHT11 setiap 2 detik

float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;
bool ledNyala = false;
bool relayNyala = false;

// Halaman dashboard: HTML + CSS + JavaScript dalam satu raw string literal.
// Tidak memakai CDN, jadi tetap berjalan tanpa internet.
const char HALAMAN_DASHBOARD[] = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Dashboard Bluino</title>
<style>
  * { box-sizing: border-box; }
  body { font-family: Arial, Helvetica, sans-serif; background: #f2f4f7;
         color: #222; margin: 0; padding: 16px; }
  .wadah { max-width: 520px; margin: 0 auto; }
  header { display: flex; justify-content: space-between; align-items: center; }
  h1 { font-size: 1.3em; margin: 0; color: #1565c0; }
  h2 { font-size: 1em; color: #555; margin: 20px 0 8px; }
  .koneksi { font-size: 0.85em; padding: 4px 10px; border-radius: 999px;
             background: #e0e0e0; color: #555; }
  .koneksi.ok { background: #e8f5e9; color: #2e7d32; }
  .koneksi.putus { background: #ffebee; color: #c62828; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
          gap: 10px; }
  .kartu { background: #fff; border-radius: 12px; padding: 14px;
           box-shadow: 0 1px 3px rgba(0, 0, 0, 0.12); }
  .label { font-size: 0.9em; color: #666; }
  .nilai { font-size: 2em; font-weight: bold; margin-top: 4px; }
  .satuan { font-size: 0.5em; color: #666; font-weight: normal; }
  .batang { height: 8px; background: #e0e0e0; border-radius: 4px;
            margin-top: 8px; overflow: hidden; }
  .isi { height: 100%; width: 0; background: #1565c0; }
  .baris { display: flex; justify-content: space-between; align-items: center;
           gap: 10px; margin-bottom: 10px; }
  .lencana { display: inline-block; margin-top: 6px; font-size: 0.85em;
             padding: 2px 10px; border-radius: 999px; background: #eceff1; color: #546e7a; }
  .lencana.nyala { background: #fff3e0; color: #e65100; }
  button { font-size: 1em; padding: 12px 16px; min-width: 120px; border: none;
           border-radius: 8px; background: #1565c0; color: #fff; cursor: pointer; }
  button.matikan { background: #455a64; }
  button:disabled { background: #90a4ae; cursor: wait; }
  .peringatan { color: #c62828; font-size: 0.9em; min-height: 1.2em; margin: 8px 0 0; }
  .info { font-size: 0.85em; color: #666; line-height: 1.6; }
</style>
</head>
<body>
<div class="wadah">
  <header>
    <h1>Dashboard Bluino</h1>
    <span id="koneksi" class="koneksi">Memuat...</span>
  </header>

  <h2>Sensor</h2>
  <div class="grid">
    <div class="kartu">
      <div class="label">Suhu</div>
      <div class="nilai"><span id="suhu">--</span> <span class="satuan">°C</span></div>
    </div>
    <div class="kartu">
      <div class="label">Kelembapan</div>
      <div class="nilai"><span id="lembap">--</span> <span class="satuan">%RH</span></div>
    </div>
    <div class="kartu">
      <div class="label">Potensiometer</div>
      <div class="nilai"><span id="pot">--</span> <span class="satuan">%</span></div>
      <div class="batang"><div id="potBatang" class="isi"></div></div>
    </div>
  </div>
  <p id="dhtPesan" class="peringatan"></p>

  <h2>Kontrol</h2>
  <div class="kartu baris">
    <div>
      <div class="label">LED Merah (LED1, IO4)</div>
      <span id="ledStatus" class="lencana">...</span>
    </div>
    <button id="ledTombol" onclick="kirim('led')" disabled>Memuat...</button>
  </div>
  <div class="kartu baris">
    <div>
      <div class="label">Relay (IO26)</div>
      <span id="relayStatus" class="lencana">...</span>
    </div>
    <button id="relayTombol" onclick="kirim('relay')" disabled>Memuat...</button>
  </div>
  <p id="pesan" class="peringatan"></p>

  <h2>Info Perangkat</h2>
  <div class="kartu info">
    Sinyal WiFi: <span id="rssi">--</span><br>
    Lama menyala: <span id="lama">--</span><br>
    Diperbarui pukul: <span id="jam">--</span>
  </div>
</div>

<script>
function el(id) { return document.getElementById(id); }

// Mengirim request dan mengembalikan isi JSON-nya (batas tunggu 6 detik)
async function ambilJson(url, opsi) {
  const kendali = new AbortController();
  const batas = setTimeout(function () { kendali.abort(); }, 6000);
  try {
    const r = await fetch(url, Object.assign({ cache: "no-store", signal: kendali.signal }, opsi));
    if (!r.ok) throw new Error("HTTP " + r.status);
    return await r.json();
  } finally {
    clearTimeout(batas);
  }
}

function aturKoneksi(ok) {
  el("koneksi").textContent = ok ? "● Terhubung" : "● Terputus";
  el("koneksi").className = "koneksi " + (ok ? "ok" : "putus");
}

// Tulisan dan warna tombol mengikuti keadaan: "Nyalakan" (biru) atau "Matikan" (abu-abu)
function pulihkanTombol(tombol) {
  const nyala = tombol.dataset.nyala === "1";
  tombol.textContent = nyala ? "Matikan" : "Nyalakan";
  tombol.className = nyala ? "matikan" : "";
  tombol.disabled = false;
}

function aturTombol(nama, nyala, teksNyala) {
  const status = el(nama + "Status");
  const tombol = el(nama + "Tombol");
  status.textContent = nyala ? teksNyala : "Mati";
  status.className = "lencana" + (nyala ? " nyala" : "");
  tombol.dataset.nyala = nyala ? "1" : "0";
  if (tombol.dataset.kirim !== "1") {  // jangan ganggu tombol yang sedang mengirim
    pulihkanTombol(tombol);
  }
}

function tampilkan(d) {
  el("suhu").textContent = d.suhu === null ? "--" : d.suhu.toFixed(1).replace(".", ",");
  el("lembap").textContent = d.kelembapan === null ? "--" : d.kelembapan.toFixed(0);
  el("pot").textContent = d.potensiometer;
  el("potBatang").style.width = d.potensiometer + "%";
  el("dhtPesan").textContent = d.dht_ok ? "" : "DHT11 belum terbaca. Periksa kabel DHT ke IO27.";
  aturTombol("led", d.led, "Menyala");
  aturTombol("relay", d.relay, "Aktif");
  el("rssi").textContent = d.rssi + " dBm";
  const s = d.waktu_nyala;
  el("lama").textContent = Math.floor(s / 3600) + " jam " + Math.floor((s % 3600) / 60) +
                           " menit " + (s % 60) + " detik";
  el("jam").textContent = new Date().toLocaleTimeString("id-ID");
}

// Polling: ambil status, tampilkan, lalu jadwalkan lagi 2 detik kemudian
async function perbarui() {
  try {
    tampilkan(await ambilJson("/api/status"));
    aturKoneksi(true);
  } catch (e) {
    aturKoneksi(false);
  }
  setTimeout(perbarui, 2000);
}

// Dipanggil saat tombol ditekan: kirim perintah kebalikan dari keadaan sekarang
async function kirim(nama) {
  const tombol = el(nama + "Tombol");
  const perintah = {};
  perintah[nama] = tombol.dataset.nyala !== "1";
  tombol.dataset.kirim = "1";
  tombol.disabled = true;
  tombol.textContent = "Mengirim...";
  el("pesan").textContent = "";
  try {
    const d = await ambilJson("/api/kontrol", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(perintah)
    });
    tombol.dataset.kirim = "0";
    tampilkan(d);
    aturKoneksi(true);
  } catch (e) {
    tombol.dataset.kirim = "0";
    pulihkanTombol(tombol);
    el("pesan").textContent = "Perintah gagal dikirim. Periksa koneksi, lalu coba lagi.";
  }
}

perbarui();
</script>
</body>
</html>
)rawliteral";

void catatRequest(int kodeStatus) {
  Serial.printf("[HTTP] %s %s -> %d (dari %s)\n",
                http_method_str(server.method()), server.uri().c_str(),
                kodeStatus, server.client().remoteIP().toString().c_str());
}

void bacaDHT() {
  dht.read(true);  // paksa membaca sensor sekarang (±25 ms), lihat BAB 26
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  bool berhasil = !isnan(h) && !isnan(t);

  if (bacaanPertama || berhasil != dhtOk) {
    if (berhasil) {
      Serial.printf("[DHT11] OK: %.1f °C, %.0f %%RH\n", t, h);
    } else {
      Serial.println("[DHT11] Gagal membaca. Periksa kabel DHT -> IO27.");
    }
  }
  bacaanPertama = false;
  dhtOk = berhasil;
  if (berhasil) {
    suhu = t;
    kelembapan = h;
  }
}

int bacaPotPersen() {
  return analogRead(PIN_POT) * 100 / 4095;
}

void aturLed(bool nyala) {
  ledNyala = nyala;
  digitalWrite(PIN_LED, nyala ? HIGH : LOW);
}

void aturRelay(bool nyala) {
  relayNyala = nyala;
  digitalWrite(PIN_RELAY, nyala ? HIGH : LOW);
}

// Menyusun status lengkap (sensor + aktuator) menjadi JSON
String buatJsonStatus() {
  JsonDocument doc;
  if (dhtOk) {
    doc["suhu"] = suhu;
    doc["kelembapan"] = kelembapan;
  } else {
    doc["suhu"] = nullptr;
    doc["kelembapan"] = nullptr;
  }
  doc["dht_ok"] = dhtOk;
  doc["potensiometer"] = bacaPotPersen();
  doc["led"] = ledNyala;
  doc["relay"] = relayNyala;
  doc["waktu_nyala"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();

  String json;
  serializeJson(doc, json);
  return json;
}

void kirimJson(int kodeStatus, const String &json) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(kodeStatus, "application/json", json);
}

// Mengirim pesan kesalahan dalam format {"error":"..."}
void kirimKesalahan(int kodeStatus, const char *pesan) {
  JsonDocument doc;
  doc["error"] = pesan;
  String json;
  serializeJson(doc, json);
  kirimJson(kodeStatus, json);
  catatRequest(kodeStatus);
  Serial.printf("       %s\n", pesan);
}

// Handler untuk GET /
void halamanDashboard() {
  server.send(200, "text/html", HALAMAN_DASHBOARD);
  catatRequest(200);
}

// Handler untuk GET /api/status (dipanggil dashboard setiap 2 detik,
// sengaja tidak dicatat supaya Serial Monitor tidak penuh)
void kirimStatus() {
  kirimJson(200, buatJsonStatus());
}

// Handler untuk POST /api/kontrol
void terimaKontrol() {
  // Body JSON tersedia di argumen khusus "plain"
  if (!server.hasArg("plain")) {
    kirimKesalahan(400, "Body kosong. Kirim JSON dengan header Content-Type: application/json");
    return;
  }

  JsonDocument perintah;
  DeserializationError err = deserializeJson(perintah, server.arg("plain"));
  if (err) {
    kirimKesalahan(400, "JSON tidak valid");
    return;
  }

  bool adaPerintah = false;
  if (perintah["led"].is<bool>()) {
    aturLed(perintah["led"]);
    adaPerintah = true;
  }
  if (perintah["relay"].is<bool>()) {
    aturRelay(perintah["relay"]);
    adaPerintah = true;
  }
  if (!adaPerintah) {
    kirimKesalahan(400, "Isi \"led\" atau \"relay\" dengan true atau false");
    return;
  }

  kirimJson(200, buatJsonStatus());  // jawab dengan keadaan terbaru
  catatRequest(200);
  Serial.printf("       LED %s, relay %s\n", ledNyala ? "MENYALA" : "MATI",
                relayNyala ? "AKTIF" : "MATI");
}

void halaman404() {
  server.send(404, "text/plain", "404 - alamat tidak ditemukan");
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
  delay(1000);
  Serial.println("\nBAB 46 - Program 3: Dashboard IoT Interaktif");

  pinMode(PIN_LED, OUTPUT);  // pinMode dulu, baru digitalWrite (core 3.x)
  pinMode(PIN_RELAY, OUTPUT);
  aturLed(false);
  aturRelay(false);
  dht.begin();

  sambungkanWiFi();

  server.on("/", HTTP_GET, halamanDashboard);
  server.on("/api/status", HTTP_GET, kirimStatus);
  server.on("/api/kontrol", HTTP_POST, terimaKontrol);
  server.onNotFound(halaman404);
  server.begin();

  Serial.print("Dashboard siap. Buka di browser: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  server.handleClient();

  if (timerDHT.sudahWaktunya(millis())) {
    bacaDHT();
  }
}
