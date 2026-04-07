# BAB 47: WebSocket — Komunikasi Real-Time Dua Arah

> ✅ **Prasyarat:** Selesaikan **BAB 46 (Web Server)**. Kamu harus sudah memahami HTTP, REST API, dan dashboard berbasis `fetch()` polling.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **keterbatasan HTTP polling** dibanding WebSocket.
- Menjelaskan mekanisme **WebSocket Handshake** (HTTP Upgrade → persistent TCP).
- Menggunakan library `arduinoWebSockets` untuk membangun WebSocket server di ESP32.
- Membuat dashboard **Server-Side Push**: data muncul otomatis tanpa browser harus meminta.
- Mengirim **perintah kontrol JSON** dari browser ke ESP32 secara instan.
- Mengimplementasi **JSON protocol** dua arah yang terstruktur.
- Menangani **auto-reconnect** agar koneksi tahan terhadap gangguan jaringan.

---

## 47.1 Masalah: Polling vs Push

```
MASALAH HTTP POLLING (BAB 46):

  Browser                    ESP32
  ───────                    ─────
  │── GET /api/sensors ─────►│  ← Request #1 (detik ke-0)
  │◄─ JSON response ─────────│
  │   [tunggu 5 detik]       │  ← ESP32 & browser sama-sama idle
  │── GET /api/sensors ─────►│  ← Request #2 (detik ke-5)
  │◄─ JSON response ─────────│

  Masalah nyata:
  ✗ Data terupdate hanya tiap 5 detik
  ✗ Overhead TCP + HTTP header ~500 bytes per request
  ✗ 5 browser = 5× request/5 detik = beban tinggi ESP32
  ✗ ESP32 TIDAK BISA kirim data tanpa diminta terlebih dahulu


SOLUSI WEBSOCKET (BAB 47):

  Browser                    ESP32
  ───────                    ─────
  │── HTTP Upgrade ─────────►│  ← Handshake SEKALI saja
  │◄─ 101 Switching ─────────│
  │                          │  ← Koneksi TCP tetap terbuka
  │◄─ {sensors data} ────────│  ← ESP32 PUSH tiap 2 detik
  │◄─ {sensors data} ────────│     tanpa diminta!
  │── {cmd:led, state:1} ───►│  ← Browser kirim perintah kapan saja
  │◄─ {ack:led, state:1} ────│

  Keunggulan:
  ✅ Data muncul INSTAN saat ESP32 siap mengirim
  ✅ Header hanya 2-14 bytes per frame (vs ~500 bytes HTTP)
  ✅ 1 broadcastTXT() → semua browser dapat sekaligus
  ✅ Server bisa menginisiasi pengiriman kapan saja
```

---

## 47.2 Anatomi WebSocket Protocol

```
FASE 1: HANDSHAKE (HTTP → WebSocket Upgrade)

  Client → Server:
  GET / HTTP/1.1
  Host: bluino.local:81
  Upgrade: websocket
  Connection: Upgrade
  Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
  Sec-WebSocket-Version: 13

  Server → Client:
  HTTP/1.1 101 Switching Protocols
  Upgrade: websocket
  Connection: Upgrade
  Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=

  → Setelah ini: bukan lagi HTTP, melainkan binary frame stream!


FASE 2: DATA FRAME

  ┌─────┬────────┬────────────────┬─────────┐
  │ FIN │ Opcode │ Payload Length │ Payload │
  └─────┴────────┴────────────────┴─────────┘

  Opcode umum:
  0x1 = Text frame  ← yang kita pakai (JSON string)
  0x8 = Close frame
  0x9 = Ping frame  ← library otomatis balas dengan Pong
  0xA = Pong frame


JSON PROTOCOL YANG DIGUNAKAN (Program 3):

  ESP32 → Browser (push tiap 2 detik):
  {
    "type": "sensors",
    "temp": 28.5,       "humidity": 65,
    "rssi": -55,        "uptime": 1234,
    "freeHeap": 195000, "sensorOk": true,
    "ledState": true,   "relayState": false,
    "clients": 2
  }

  Browser → ESP32 (perintah kontrol):
  {"cmd":"led",    "state":1}
  {"cmd":"relay",  "state":0}
  {"cmd":"status"}
  {"cmd":"restart"}

  ESP32 → Browser (konfirmasi — broadcast ke SEMUA):
  {"type":"ack", "cmd":"led", "state":true, "ok":true}
```

---

## 47.3 Library & Instalasi

```
LIBRARY TAMBAHAN (install via Arduino IDE Library Manager):

  Nama    : WebSockets
  Author  : Markus Sattler (Links2004)
  Versi   : >= 2.4.0
  Header  : WebSocketsServer.h
  Install : Library Manager → Cari "WebSockets" → pilih by Markus Sattler

LIBRARY DARI BAB SEBELUMNYA (tidak perlu install ulang):
  WiFi.h, ESPmDNS.h, WebServer.h, Preferences.h  → Bawaan ESP32 Core
  DHT.h                                           → Adafruit DHT sensor library


PORT YANG DIGUNAKAN:
  Port 80  → HTTP (halaman HTML dashboard)
  Port 81  → WebSocket (data real-time)
  
  Keduanya berjalan BERSAMAAN pada satu ESP32!


FUNGSI UTAMA WebSocketsServer:
  ws.begin()              → Mulai server
  ws.loop()               → Proses frame (panggil di loop(), non-blocking)
  ws.onEvent(callback)    → Daftarkan event handler
  ws.sendTXT(num, data)   → Kirim teks ke client tertentu (num = ID client)
  ws.broadcastTXT(data)   → Kirim teks ke SEMUA client sekaligus
  ws.remoteIP(num)        → Dapatkan IP client

EVENT CALLBACK:
  WStype_CONNECTED      → Client baru terhubung
  WStype_DISCONNECTED   → Client terputus
  WStype_TEXT           → Frame teks diterima (JSON kita)
  WStype_PING           → Ping diterima (library auto-reply Pong)
  WStype_PONG           → Konfirmasi Pong dari client
```

---

## 47.4 Program 1: WebSocket Echo Server

📂 **Kode:** [`p1_echo/p1_echo.ino`](p1_echo/p1_echo.ino)

**Apa yang dipelajari:**
- Cara menggunakan `WebSocketsServer.h` bersama `WebServer.h`
- Struktur event handler: `WStype_CONNECTED`, `WStype_TEXT`, `WStype_DISCONNECTED`
- `ws.sendTXT(num, data)` vs `ws.broadcastTXT(data)`
- Membangun chat sederhana: browser → ESP32 → echo kembali
- **Non-blocking TCP Handlers**: Mengapa `delay()` haram digunakan di dalam event WebSocket (diganti dengan `millis()` timer).
- **JSON String Payload Safety**: Membedah teks ber-escape (`\"` vs `\\`) secara presisi tanpa merusak format balasan.

**Cara Uji:**
1. Install library **WebSockets by Markus Sattler** via Library Manager
2. Ganti `WIFI_SSID` dan `WIFI_PASS`
3. Upload → buka Serial Monitor (115200 baud)
4. Browser: `http://bluino.local`
5. Ketik pesan → amati echo dari ESP32
6. Buka browser ke-2 → amati counter "Client Aktif" berubah

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 47 Program 1: WebSocket Echo Server ===
[WiFi] ✅ IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] ✅ http://bluino.local | ws://bluino.local:81
[HTTP] Web server aktif di port 80.
[WS]   WebSocket server aktif di port 81.
[WS] Client #0 terhubung dari 192.168.1.10 | Total: 1
[WS] #0 → {"cmd":"echo","msg":"Halo ESP32!"}
[WS] Client #1 terhubung dari 192.168.1.11 | Total: 2
[HB] Clients: 2 | Pesan: 5 | Heap: 228544 B | Uptime: 32 s
```

---

## 47.5 Program 2: Real-Time Sensor Broadcasting

📂 **Kode:** [`p2_broadcast/p2_broadcast.ino`](p2_broadcast/p2_broadcast.ino)

**Apa yang dipelajari:**
- Pola **Server-Side Push**: ESP32 menginisiasi pengiriman data tanpa browser meminta
- `ws.broadcastTXT(json)` → satu panggilan, semua browser update bersamaan
- Menggabungkan DHT11 non-blocking dengan WebSocket broadcast
- Efisiensi Jaringan: 5 browser open = tetap 1 broadcast per 2 detik (vs 5 request/2 detik di BAB 46)
- **Zero-Client CPU Optimization**: Menghentikan perakitan string JSON jika `wsClients == 0` (hemat memori/energi).
- **JS Watchdog Timer**: Menerapkan mekanisme `setTimeout` pada browser untuk mendeteksi jaringan yang terputus (stale data) secara gaib *(silent TCP drop)*.

**Cara Uji:**
1. Ganti `WIFI_SSID` dan `WIFI_PASS`
2. Upload → buka browser: `http://bluino.local`
3. **Amati angka berubah otomatis tiap 2 detik — tanpa refresh halaman!**
4. Buka 2 browser sekaligus → keduanya update bersamaan di detik yang sama
5. Cabut kabel DHT11 → indikator "❌ Error" muncul instan di semua browser
6. Colok kembali → "✅ OK" muncul otomatis

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 47 Program 2: Real-Time Sensor Broadcasting ===
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] ✅ http://bluino.local
[HTTP] Server aktif di port 80.
[WS]   Server aktif di port 81.
[INFO] Dashboard → http://bluino.local
[WS] #0 terhubung dari 192.168.1.10 | Total: 1
[DHT] ⚠️ Gagal baca! Cek wiring IO27.
[DHT] ✅ Sensor pulih!
[HB] 28.5C 65% | Clients: 2 | Heap: 212480 B
```

---

## 47.6 Program 3: Dashboard IoT Real-Time Interaktif

📂 **Kode:** [`p3_dashboard/p3_dashboard.ino`](p3_dashboard/p3_dashboard.ino)

**Apa yang dipelajari:**
- **JSON Protocol dua arah** yang terstruktur
- **Optimistic UI Update**: button UI berubah SEKETIKA saat ditekan, sebelum konfirmasi server
- **Broadcast ACK**: saat LED dinyalakan dari browser A, browser B ikut update otomatis
- **NVS Persistence**: state relay tersimpan, tidak hilang setelah reboot
- **Zero-Heap Serial CLI**: antrian perintah Serial menggunakan *C-String static array* murni (0 alokasi memori dinamis)
- **Zero-Heap JSON Parser**: metode estrak field `getStr` dan `getInt` menggunakan *Pointer Scan* (`strstr`), membebaskan ESP32 dari ancaman `String` *fragmentation* saat diterpa trafik WebSocket yang brutal.
- **Arsitektur Produksi (Capstone):** Menggabungkan JS Watchdog, Zero-Client Efficiency, Zero-Heap C++, dan Pedantic Parity JSON Parser di dalam satu atap yang tak tertembus.

**Cara Uji:**
1. Sambungkan kabel jumper dari relay board ke **IO4** (atau ubah `RELAY_PIN`)
2. Ganti `WIFI_SSID`, `WIFI_PASS`, dan `RELAY_PIN`
3. Upload → buka `http://bluino.local`
4. Tekan tombol **Nyalakan LED** → LED fisik menyala INSTAN
5. Tekan **Aktifkan Relay** → relay berbunyi klik, state tersimpan NVS
6. Restart ESP32 (`relay on` di Serial CLI atau cabut power) → relay langsung ON kembali
7. Buka 2 browser → kontrol LED dari browser A → browser B update otomatis!
8. Coba perintah CLI: `help`, `status`, `led on`, `relay off`, `restart`

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 47 Program 3: Dashboard IoT Real-Time Interaktif ===
[INFO] Ketik 'help' di Serial Monitor.
[INFO] Relay PIN: IO4 — sambungkan kabel jumper!
[NVS] Relay state dimuat: OFF
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] ✅ http://bluino.local
[HTTP] Server aktif di port 80.
[WS]   Server aktif di port 81.

==========================================
 Dashboard : http://bluino.local
 WebSocket : ws://bluino.local:81
 Relay PIN : IO4 (kabel jumper)
==========================================
[WS] #0 terhubung dari 192.168.1.10 | Total: 1
[WS] #0 → {"cmd":"led","state":1}
[WS] LED ON via WebSocket
[WS] #0 → {"cmd":"relay","state":1}
[WS] Relay ON via WebSocket — disimpan NVS
[HB] 28.5C 65% LED:ON Relay:ON WS:1 Heap:191240 B
> status
[CLI] > status
-- Status ----------------------------------------
  WiFi   : Terhubung
  IP     : 192.168.1.105
  RSSI   : -55 dBm
  LED    : ON
  Relay  : ON (IO4)
  Suhu   : 28.5 C
  Hum    : 65 %
  Heap   : 191240 bytes
  Uptime : 120 s
  WS     : 1 client
-------------------------------------------------
```

---

## 47.7 Perbandingan BAB 46 vs BAB 47

| Aspek | BAB 46 (HTTP Polling) | BAB 47 (WebSocket Push) |
|-------|----------------------|------------------------|
| **Protokol** | HTTP request/response | WS persistent TCP |
| **Update data** | Tiap 5 detik (polling) | Push instan saat siap |
| **Header overhead** | ~500 bytes/request | 2–14 bytes/frame |
| **5 browser aktif** | 5× request/5 detik | 1× broadcast/2 detik |
| **Latensi kontrol** | + waktu HTTP roundtrip | < 10ms |
| **Inisiator data** | Client (browser) | Server (ESP32) |
| **Browser disconnect** | Fetch timeout | Event `WStype_DISCONNECTED` |
| **Multi-browser sync** | Tidak sinkron | Otomatis — broadcast ACK |
| **Library** | Bawaan ESP32 Core | Install: WebSockets by Markus Sattler |

---

## 47.8 Konsep Lanjutan

```
WEBSOCKET vs SERVER-SENT EVENTS (SSE) vs MQTT:

  Teknologi    Arah       Protokol   Cocok untuk
  ──────────   ─────      ────────   ────────────
  WebSocket    2-arah     WS/WSS     Dashboard interaktif (BAB ini)
  SSE          1-arah     HTTP       Monitoring pasif (display only)
  MQTT         2-arah     TCP        IoT skala besar, banyak perangkat

KEAMANAN (WSS — WebSocket Secure):
  Produksi nyata → gunakan WSS (WebSocket over TLS)
  Sama seperti HTTPS vs HTTP
  ESP32 mendukung: ESPAsyncWebServer + SSL certificate

BATASAN WebSocketsServer di ESP32:
  Maksimum ~4 client bersamaan (keterbatasan memori / socket)
  Untuk banyak client → pertimbangkan ESPAsyncWebServer + AsyncWebSocket
```

---

## 47.9 Latihan & Tantangan

### Latihan Dasar
1. **Upload Program 1**, kirim pesan dari 2 browser. Amati log Serial — berapa Client ID masing-masing browser?
2. **Upload Program 2**, tiup hangat ke DHT11. Berapa detik sebelum perubahan suhu muncul di browser?
3. Bandingkan dengan BAB 46: di Program 3 BAB 46, berapa detik delay sebelum perubahan terlihat?

### Latihan Menengah
4. **Upload Program 3**, tekan LED dari browser A — berapa milidetik sebelum browser B juga update?
5. **Uji NVS**: ketik `relay on`, power off ESP32, colok lagi — apakah relay langsung aktif?
6. **Modifikasi Program 2**: tambahkan field `"model":"Bluino IoT Kit v3.2"` ke JSON yang di-broadcast.

### Tantangan Lanjutan
7. **Multi-sensor**: tambahkan pembacaan LDR (IO35) ke Program 3. Broadcast nilai `"ldr": 1023` ke dashboard.
8. **Notifikasi Alert**: jika suhu > 35°C, kirim `{"type":"alert","msg":"Suhu kritis!"}` ke semua browser. Tampilkan popup di dashboard.
9. **Timestamp**: tambahkan `"ts": millis()` ke setiap broadcast. Hitung di JS berapa milidetik antara dua frame — ini adalah **latency aktual** WebSocket kita.

### Kata Kunci Penelitian Mandiri
- `WebSocket RFC 6455` — Spesifikasi teknis resmi protokol WebSocket
- `ESPAsyncWebSocket` — WebSocket asinkron untuk ESP32 (cocok untuk banyak client)
- `MQTT` — Protokol IoT skala enterprise (preview BAB 48)
- `WSS (WebSocket Secure)` — WebSocket over TLS/SSL

---

## 47.10 Referensi

| Sumber | Link |
|--------|------|
| arduinoWebSockets by Markus Sattler | https://github.com/Links2004/arduinoWebSockets |
| RFC 6455 — WebSocket Protocol | https://datatracker.ietf.org/doc/html/rfc6455 |
| MDN WebSocket API | https://developer.mozilla.org/en-US/docs/Web/API/WebSocket |
| ESP32 WebSocket Example | https://github.com/Links2004/arduinoWebSockets/tree/master/examples/esp32 |
| WebSocket vs Polling | https://ably.com/topic/long-polling |

---

> Selesai! ESP32 Bluino Kit kamu sekarang mampu berkomunikasi **dua arah secara real-time**: server mem-*push* data sensor tanpa diminta, dan browser mengirim perintah kontrol yang langsung direspons dalam hitungan milidetik — tanpa satu pun baris `fetch()` polling!
>
> **Langkah Selanjutnya:** [BAB 48 — MQTT](../BAB-48/) untuk protokol IoT skala enterprise yang dapat menghubungkan ribuan perangkat sekaligus melalui broker publik/privat.
