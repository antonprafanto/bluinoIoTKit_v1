/*
 * BAB 47 - Program 2: Siaran Data Sensor Real-Time
 * - GET /           -> halaman monitor (HTML + JavaScript), lewat port 80
 * - ws://<IP>:81/   -> ESP32 MENGIRIM data sensor ke semua browser tanpa diminta:
 *     * suhu & kelembapan DHT11 setiap 2 detik
 *     * potensiometer begitu nilainya berubah (paling sering 10 pesan per detik)
 * Rangkaian: DHT -> IO27, VR10K -> IO34
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
#define PIN_POT 34  // potensiometer VR10K (ADC1, aman dipakai bersama WiFi)

const int AMBANG_POT = 2;                  // %: perubahan lebih kecil dianggap derau
const unsigned long JEDA_KIRIM_MIN = 100;  // ms: paling sering 10 pesan per detik

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
WebSocketsServer ws(81);  // port 81: pesan WebSocket
DHT dht(PIN_DHT, TIPE_DHT);

Timer timerDHT = {2000, 0};       // baca DHT11 + kiriman rutin setiap 2 detik
Timer timerPot = {20, 0};         // periksa potensiometer setiap 20 ms
Timer timerLaporan = {10000, 0};  // laporan di Serial Monitor setiap 10 detik

float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;
int potTerkirim = 0;  // nilai potensiometer (%) terbaru yang dikirim ke browser

bool adaPerubahan = false;  // ada data baru yang belum dikirim?
unsigned long waktuKirimTerakhir = 0;
unsigned long jumlahSiaran = 0;  // pesan yang disiarkan sejak laporan terakhir

// Halaman monitor. Tidak memakai CDN, jadi tetap berjalan tanpa internet.
const char HALAMAN_MONITOR[] = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sensor Real-Time</title>
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
  .peringatan { color: #c62828; font-size: 0.9em; min-height: 1.2em; margin: 8px 0 0; }
  .info { font-size: 0.85em; color: #666; line-height: 1.6; }
</style>
</head>
<body>
<div class="wadah">
  <header>
    <h1>Sensor Real-Time</h1>
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

  <h2>Info Koneksi</h2>
  <div class="kartu info">
    Browser tersambung: <span id="klien">--</span><br>
    Pesan diterima halaman ini: <span id="jumlah">0</span><br>
    Pesan terakhir pukul: <span id="jam">--</span><br>
    Lama ESP32 menyala: <span id="lama">--</span>
  </div>
</div>

<script>
function el(id) { return document.getElementById(id); }
let ws;
let pengawas;         // timer pendeteksi koneksi macet
let jumlahPesan = 0;

function aturKoneksi(ok) {
  el("koneksi").textContent = ok ? "● Terhubung" : "● Terputus";
  el("koneksi").className = "koneksi " + (ok ? "ok" : "putus");
}

function tampilkan(d) {
  el("suhu").textContent = d.suhu === null ? "--" : d.suhu.toFixed(1).replace(".", ",");
  el("lembap").textContent = d.kelembapan === null ? "--" : d.kelembapan.toFixed(0);
  el("pot").textContent = d.potensiometer;
  el("potBatang").style.width = d.potensiometer + "%";
  el("dhtPesan").textContent = d.dht_ok ? "" : "DHT11 belum terbaca. Periksa kabel DHT ke IO27.";
  el("klien").textContent = d.klien;
  jumlahPesan = jumlahPesan + 1;
  el("jumlah").textContent = jumlahPesan;
  el("jam").textContent = new Date().toLocaleTimeString("id-ID");
  const s = d.waktu_nyala;
  el("lama").textContent = Math.floor(s / 3600) + " jam " + Math.floor((s % 3600) / 60) +
                           " menit " + (s % 60) + " detik";
}

// Pengawas: ESP32 mengirim data minimal setiap 2 detik. Kalau 6 detik
// tidak ada pesan sama sekali, koneksi dianggap macet.
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
    if (d.tipe === "sensor") tampilkan(d);
  };
  ws.onclose = putus;
}

sambungkan();
</script>
</body>
</html>
)rawliteral";

// Dipanggil library (dari dalam ws.loop()) setiap ada kejadian WebSocket
void tanganiWebSocket(uint8_t nomor, WStype_t jenis, uint8_t *data, size_t panjang) {
  switch (jenis) {
    case WStype_CONNECTED:
      Serial.printf("[WS] Klien #%u terhubung dari %s (total %d klien)\n", nomor,
                    ws.remoteIP(nomor).toString().c_str(), ws.connectedClients());
      adaPerubahan = true;  // klien baru segera dapat data; yang lain melihat jumlah klien baru
      break;

    case WStype_DISCONNECTED:
      Serial.printf("[WS] Klien #%u terputus (sisa %d klien)\n", nomor, ws.connectedClients());
      adaPerubahan = true;
      break;

    case WStype_TEXT:
      // Program ini hanya MENGIRIM data; pesan dari browser cukup dicatat
      if (panjang > 0) {
        Serial.printf("[WS] Pesan dari klien #%u diabaikan: %s\n", nomor, (const char *)data);
      }
      break;

    default:
      break;
  }
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

// Menyusun data sensor menjadi teks JSON
String buatJsonSensor() {
  JsonDocument doc;
  doc["tipe"] = "sensor";
  if (dhtOk) {
    doc["suhu"] = suhu;
    doc["kelembapan"] = kelembapan;
  } else {
    doc["suhu"] = nullptr;  // null = tidak ada nilai
    doc["kelembapan"] = nullptr;
  }
  doc["dht_ok"] = dhtOk;
  doc["potensiometer"] = potTerkirim;
  doc["klien"] = ws.connectedClients();
  doc["waktu_nyala"] = millis() / 1000;

  String json;
  serializeJson(doc, json);
  return json;
}

// Mengirim data terbaru ke SEMUA browser yang tersambung
void siarkanSensor() {
  if (ws.connectedClients() == 0) return;  // tidak ada yang mendengarkan: lewati
  String json = buatJsonSensor();
  ws.broadcastTXT(json);
  jumlahSiaran = jumlahSiaran + 1;
}

void halamanUtama() {
  server.send(200, "text/html", HALAMAN_MONITOR);
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
  Serial.println("\nBAB 47 - Program 2: Siaran Data Sensor Real-Time");

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

  // 1. Kiriman rutin setiap 2 detik: baca DHT11 dan samakan nilai potensiometer
  if (timerDHT.sudahWaktunya(sekarang)) {
    bacaDHT();
    potTerkirim = bacaPotPersen();
    adaPerubahan = true;
  }

  // 2. Potensiometer: tandai kalau berubah cukup jauh (bukan sekadar derau)
  if (timerPot.sudahWaktunya(sekarang)) {
    int pot = bacaPotPersen();
    if (abs(pot - potTerkirim) >= AMBANG_POT) {
      potTerkirim = pot;
      adaPerubahan = true;
    }
  }

  // 3. Kirim kalau ada perubahan, tetapi paling sering setiap JEDA_KIRIM_MIN
  if (adaPerubahan && sekarang - waktuKirimTerakhir >= JEDA_KIRIM_MIN) {
    siarkanSensor();
    adaPerubahan = false;
    waktuKirimTerakhir = sekarang;
  }

  // 4. Laporan berkala di Serial Monitor
  if (timerLaporan.sudahWaktunya(sekarang)) {
    int klien = ws.connectedClients();
    if (klien > 0) {
      Serial.printf("[INFO] 10 detik terakhir: %lu pesan disiarkan ke %d klien\n",
                    jumlahSiaran, klien);
    }
    jumlahSiaran = 0;
  }
}
