/*
 * BAB 47 - Program 3: Dashboard Dua Arah
 * - GET /           -> halaman dashboard (HTML + JavaScript), lewat port 80
 * - ws://<IP>:81/   -> WebSocket dua arah:
 *     ESP32 -> semua browser : {"tipe":"status", ...sensor, LED, relay...}
 *                              setiap 2 detik dan segera setelah ada perubahan
 *     browser -> ESP32       : {"led":true} atau {"relay":false}
 *     ESP32 -> satu browser  : {"tipe":"error","pesan":"..."} kalau perintah ditolak
 * Tombol S1 di kit membalik LED merah; semua browser ikut diperbarui.
 * Rangkaian: DHT -> IO27, VR10K -> IO34, LED1 (merah) -> IO4, RELAY -> IO26, SW1 -> IO15
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu
const char *NAMA_MDNS = "bluino";                // nama .local, tanpa ".local" (BAB 45)

#define PIN_DHT 27
#define TIPE_DHT DHT11
#define PIN_POT 34    // potensiometer VR10K (ADC1)
#define PIN_LED 4     // LED1 (merah), aktif HIGH
#define PIN_RELAY 26  // RELAY, aktif HIGH
#define PIN_S1 15     // tombol S1 (SW1), ditekan = HIGH (pull-down 10 kΩ di PCB)

const int AMBANG_POT = 2;                  // %: perubahan lebih kecil dianggap derau
const unsigned long JEDA_KIRIM_MIN = 100;  // ms: paling sering 10 pesan per detik
const unsigned long WAKTU_DEBOUNCE = 50;   // ms: tombol harus stabil selama ini (BAB 9)

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

WebServer server(80);     // port 80: mengirim halaman web
WebSocketsServer ws(81);  // port 81: pesan WebSocket dua arah
DHT dht(PIN_DHT, TIPE_DHT);

Timer timerDHT = {2000, 0};  // baca DHT11 + siaran rutin setiap 2 detik
Timer timerPot = {20, 0};    // periksa potensiometer setiap 20 ms

float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;
int potTerkirim = 0;  // nilai potensiometer (%) terbaru yang dikirim ke browser
bool ledNyala = false;
bool relayNyala = false;

bool adaPerubahan = false;  // ada keadaan baru yang belum disiarkan?
unsigned long waktuKirimTerakhir = 0;

// Data debounce tombol S1 (BAB 9)
int bacaanS1Sebelumnya = LOW;
int statusS1Stabil = LOW;
unsigned long waktuS1Berubah = 0;

// Halaman dashboard. Tidak memakai CDN, jadi tetap berjalan tanpa internet.
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
  button:disabled { background: #90a4ae; cursor: not-allowed; }
  .peringatan { color: #c62828; font-size: 0.9em; min-height: 1.2em; margin: 8px 0 0; }
  .info { font-size: 0.85em; color: #666; line-height: 1.6; }
</style>
</head>
<body>
<div class="wadah">
  <header>
    <h1>Dashboard Bluino</h1>
    <span id="koneksi" class="koneksi">Menyambung...</span>
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
    Browser tersambung: <span id="klien">--</span><br>
    Sinyal WiFi: <span id="rssi">--</span><br>
    Lama menyala: <span id="lama">--</span><br>
    Diperbarui pukul: <span id="jam">--</span>
  </div>
</div>

<script>
function el(id) { return document.getElementById(id); }
let ws;
let pengawas;                             // timer pendeteksi koneksi macet
const keadaan = { led: false, relay: false };  // keadaan terakhir menurut ESP32

function aturKoneksi(ok) {
  el("koneksi").textContent = ok ? "● Terhubung" : "● Terputus";
  el("koneksi").className = "koneksi " + (ok ? "ok" : "putus");
  if (!ok) {  // selama terputus, tombol tidak bisa ditekan
    el("ledTombol").disabled = true;
    el("relayTombol").disabled = true;
  }
}

// Status = keadaan SEKARANG; tulisan tombol = AKSI yang terjadi kalau ditekan
function aturTombol(nama, nyala, teksNyala) {
  keadaan[nama] = nyala;
  el(nama + "Status").textContent = nyala ? teksNyala : "Mati";
  el(nama + "Status").className = "lencana" + (nyala ? " nyala" : "");
  const tombol = el(nama + "Tombol");
  tombol.textContent = nyala ? "Matikan" : "Nyalakan";
  tombol.className = nyala ? "matikan" : "";
  tombol.disabled = false;
}

function tampilkan(d) {
  el("suhu").textContent = d.suhu === null ? "--" : d.suhu.toFixed(1).replace(".", ",");
  el("lembap").textContent = d.kelembapan === null ? "--" : d.kelembapan.toFixed(0);
  el("pot").textContent = d.potensiometer;
  el("potBatang").style.width = d.potensiometer + "%";
  el("dhtPesan").textContent = d.dht_ok ? "" : "DHT11 belum terbaca. Periksa kabel DHT ke IO27.";
  aturTombol("led", d.led, "Menyala");
  aturTombol("relay", d.relay, "Aktif");
  el("klien").textContent = d.klien;
  el("rssi").textContent = d.rssi + " dBm";
  const s = d.waktu_nyala;
  el("lama").textContent = Math.floor(s / 3600) + " jam " + Math.floor((s % 3600) / 60) +
                           " menit " + (s % 60) + " detik";
  el("jam").textContent = new Date().toLocaleTimeString("id-ID");
}

// Tombol ditekan: kirim keadaan TUJUAN (kebalikan keadaan sekarang),
// lalu tunggu siaran status dari ESP32 untuk mengubah tampilan
function kirim(nama) {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  const perintah = {};
  perintah[nama] = !keadaan[nama];
  ws.send(JSON.stringify(perintah));
  el("pesan").textContent = "";
}

// Pengawas: ESP32 menyiarkan status minimal setiap 2 detik.
// Kalau 6 detik tidak ada pesan sama sekali, koneksi dianggap macet.
function aturPengawas() {
  clearTimeout(pengawas);
  pengawas = setTimeout(putus, 6000);
}

// Dipanggil saat koneksi tertutup ATAU macet:
// buang koneksi lama, lalu coba sambung lagi 3 detik kemudian
function putus() {
  clearTimeout(pengawas);
  aturKoneksi(false);
  ws.onclose = null;    // koneksi lama tidak dipedulikan lagi
  ws.onmessage = null;
  ws.close();
  setTimeout(sambungkan, 3000);
}

function sambungkan() {
  ws = new WebSocket("ws://" + location.hostname + ":81/");
  aturPengawas();       // kalau 6 detik belum juga tersambung, coba lagi
  ws.onopen = function () {
    aturKoneksi(true);
    aturPengawas();
  };
  ws.onmessage = function (e) {
    aturPengawas();     // ada pesan = koneksi masih hidup
    let d;
    try {
      d = JSON.parse(e.data);
    } catch (err) {
      console.warn("Pesan bukan JSON:", e.data);
      return;
    }
    if (d.tipe === "status") {
      tampilkan(d);
    } else if (d.tipe === "error") {
      el("pesan").textContent = "ESP32 menolak perintah: " + d.pesan;
    }
  };
  ws.onclose = putus;
}

sambungkan();
</script>
</body>
</html>
)rawliteral";

void aturLed(bool nyala) {
  ledNyala = nyala;
  digitalWrite(PIN_LED, nyala ? HIGH : LOW);
  adaPerubahan = true;  // semua browser perlu tahu
}

void aturRelay(bool nyala) {
  relayNyala = nyala;
  digitalWrite(PIN_RELAY, nyala ? HIGH : LOW);
  adaPerubahan = true;
}

void bacaDHT() {
  dht.read(true);  // paksa membaca sensor sekarang (±25 ms), lihat BAB 26
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  bool berhasil = !isnan(h) && !isnan(t);

  if (bacaanPertama || berhasil != dhtOk) {  // cetak hanya saat status berubah
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
  long total = 0;
  for (int i = 0; i < 8; i++) {
    total += analogRead(PIN_POT);  // rata-rata 8 bacaan untuk meredam derau ADC
  }
  return (total / 8) * 100 / 4095;
}

// Mengembalikan true SATU KALI setiap S1 baru ditekan (debounce + edge detection, BAB 9)
bool s1BaruDitekan() {
  int bacaan = digitalRead(PIN_S1);
  if (bacaan != bacaanS1Sebelumnya) {  // bacaan mentah berubah: ulangi "stopwatch"
    waktuS1Berubah = millis();
    bacaanS1Sebelumnya = bacaan;
  }
  if ((millis() - waktuS1Berubah) > WAKTU_DEBOUNCE && bacaan != statusS1Stabil) {
    statusS1Stabil = bacaan;
    return statusS1Stabil == HIGH;  // hanya saat berubah dari lepas ke ditekan
  }
  return false;
}

// Menyusun status lengkap (sensor + aktuator) menjadi teks JSON
String buatJsonStatus() {
  JsonDocument doc;
  doc["tipe"] = "status";
  if (dhtOk) {
    doc["suhu"] = suhu;
    doc["kelembapan"] = kelembapan;
  } else {
    doc["suhu"] = nullptr;
    doc["kelembapan"] = nullptr;
  }
  doc["dht_ok"] = dhtOk;
  doc["potensiometer"] = potTerkirim;
  doc["led"] = ledNyala;
  doc["relay"] = relayNyala;
  doc["klien"] = ws.connectedClients();
  doc["rssi"] = WiFi.RSSI();
  doc["waktu_nyala"] = millis() / 1000;

  String json;
  serializeJson(doc, json);
  return json;
}

// Menyiarkan status terbaru ke SEMUA browser yang tersambung
void siarkanStatus() {
  if (ws.connectedClients() == 0) return;  // tidak ada yang mendengarkan: lewati
  String json = buatJsonStatus();
  ws.broadcastTXT(json);
}

// Mengirim pesan kesalahan HANYA ke klien yang mengirim perintah salah
void kirimKesalahan(uint8_t nomor, const char *pesan) {
  JsonDocument doc;
  doc["tipe"] = "error";
  doc["pesan"] = pesan;
  String json;
  serializeJson(doc, json);
  ws.sendTXT(nomor, json);
  Serial.printf("[WS] Perintah dari klien #%u ditolak: %s\n", nomor, pesan);
}

// Membaca perintah JSON dari browser, misalnya {"led":true} atau {"relay":false}
void terimaPerintah(uint8_t nomor, uint8_t *data, size_t panjang) {
  JsonDocument perintah;
  DeserializationError err = deserializeJson(perintah, data, panjang);
  if (err) {
    kirimKesalahan(nomor, "JSON tidak valid");
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
    kirimKesalahan(nomor, "Isi \"led\" atau \"relay\" dengan true atau false");
    return;
  }

  Serial.printf("[WS] Perintah dari klien #%u: LED %s, relay %s\n", nomor,
                ledNyala ? "MENYALA" : "MATI", relayNyala ? "AKTIF" : "MATI");
}

// Dipanggil library (dari dalam ws.loop()) setiap ada kejadian WebSocket
void tanganiWebSocket(uint8_t nomor, WStype_t jenis, uint8_t *data, size_t panjang) {
  switch (jenis) {
    case WStype_CONNECTED:
      Serial.printf("[WS] Klien #%u terhubung dari %s (total %d klien)\n", nomor,
                    ws.remoteIP(nomor).toString().c_str(), ws.connectedClients());
      adaPerubahan = true;  // klien baru segera mendapat status terbaru
      break;

    case WStype_DISCONNECTED:
      Serial.printf("[WS] Klien #%u terputus (sisa %d klien)\n", nomor, ws.connectedClients());
      adaPerubahan = true;  // jumlah klien berubah
      break;

    case WStype_TEXT:
      if (panjang > 0) {
        terimaPerintah(nomor, data, panjang);
      }
      break;

    default:
      break;
  }
}

void halamanUtama() {
  server.send(200, "text/html", HALAMAN_DASHBOARD);
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
  Serial.println("\nBAB 47 - Program 3: Dashboard Dua Arah");

  pinMode(PIN_LED, OUTPUT);  // pinMode dulu, baru digitalWrite (core 3.x)
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_S1, INPUT);    // pull-down eksternal di PCB: ditekan = HIGH
  aturLed(false);
  aturRelay(false);
  dht.begin();
  potTerkirim = bacaPotPersen();

  sambungkanWiFi();
  bool mdnsAktif = MDNS.begin(NAMA_MDNS);
  if (mdnsAktif) {
    MDNS.addService("http", "tcp", 80);
  }

  server.on("/", HTTP_GET, halamanUtama);
  server.onNotFound(halaman404);
  server.begin();

  ws.begin();
  ws.onEvent(tanganiWebSocket);
  ws.enableHeartbeat(15000, 3000, 2);  // ping tiap 15 detik, putus setelah 2x tidak dijawab

  String ip = WiFi.localIP().toString();
  Serial.printf("Halaman web : http://%s/\n", ip.c_str());
  if (mdnsAktif) {
    Serial.printf("              atau http://%s.local/\n", NAMA_MDNS);
  }
  Serial.printf("WebSocket   : ws://%s:81/\n", ip.c_str());
}

void loop() {
  server.handleClient();
  ws.loop();

  unsigned long sekarang = millis();

  // 1. Tombol fisik S1 membalik LED merah
  if (s1BaruDitekan()) {
    aturLed(!ledNyala);
    Serial.printf("[TOMBOL] S1 ditekan: LED %s\n", ledNyala ? "MENYALA" : "MATI");
  }

  // 2. Siaran rutin setiap 2 detik: baca DHT11 dan samakan nilai potensiometer
  if (timerDHT.sudahWaktunya(sekarang)) {
    bacaDHT();
    potTerkirim = bacaPotPersen();
    adaPerubahan = true;
  }

  // 3. Potensiometer: tandai kalau berubah cukup jauh (bukan sekadar derau)
  if (timerPot.sudahWaktunya(sekarang)) {
    int pot = bacaPotPersen();
    if (abs(pot - potTerkirim) >= AMBANG_POT) {
      potTerkirim = pot;
      adaPerubahan = true;
    }
  }

  // 4. Siarkan kalau ada perubahan, tetapi paling sering setiap JEDA_KIRIM_MIN
  if (adaPerubahan && sekarang - waktuKirimTerakhir >= JEDA_KIRIM_MIN) {
    siarkanStatus();
    adaPerubahan = false;
    waktuKirimTerakhir = sekarang;
  }
}
