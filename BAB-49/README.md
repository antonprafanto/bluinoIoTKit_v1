# 🔐 BAB 49 — HTTPS & Keamanan: `WiFiClientSecure` & TLS/SSL

## Konsep yang Dipelajari

- Perbedaan fundamental HTTP vs HTTPS
- Cara kerja TLS/SSL Handshake
- Penggunaan `WiFiClientSecure` di ESP32
- Metode `setInsecure()` dan `setCACert()` — kapan menggunakan masing-masing
- Konsep Man-in-the-Middle (MITM) Attack
- Root CA Certificate: apa itu, cara mendapatkan, dan cara memperbaruinya
- Integrasi nyata dengan **Antares.id** — Platform IoT Indonesia

---

## 🧪 Perangkat & Library yang Dibutuhkan

### Hardware
| Komponen | Pin |
|---|---|
| ESP32 DEVKIT V1 | — |
| DHT11 | IO27 (hardwired) |
| OLED 128×64 I2C | SDA=IO21, SCL=IO22 (hardwired) |

### Library (Install via Arduino Library Manager)
| Library | Versi | Keterangan |
|---------|-------|-----------|
| ArduinoJson | >= 6.x | Parse & build JSON |
| Adafruit SSD1306 | latest | Driver OLED |
| Adafruit GFX Library | latest | Dependensi OLED |
| DHT sensor library | latest | Baca DHT11 |

> **Library Bawaan ESP32 Core (tidak perlu install):**
> `WiFi.h`, `WiFiClientSecure.h`, `HTTPClient.h`

---

## 📡 HTTP vs HTTPS — Perbedaan Fundamental

```
HTTP  (Port 80)
  ESP32 ──── Teks Polos (Plaintext) ────► Server
             "temp=29&key=rahasia"
             ⚠️ SIAPA SAJA BISA MEMBACA INI DI JARINGAN!

HTTPS (Port 443 / 8443)
  ESP32 ──── Data Terenkripsi TLS ──────► Server  
             "X@#$k*&!@#$%^&*()"
             ✅ Hanya pengirim dan penerima yang bisa membaca!
```

### Apa itu TLS Handshake?
Sebelum data dikirim, ESP32 dan server melakukan "jabat tangan" *(handshake)* untuk:
1. **Server mengirim sertifikat digital** (semacam KTP digital)
2. **ESP32 memverifikasi sertifikat** menggunakan Root CA yang dipercaya
3. **Keduanya sepakat kunci enkripsi** yang akan digunakan
4. **Koneksi aman dimulai** — semua data terenkripsi

---

## ⚠️ Bahaya: Man-in-the-Middle Attack (MITM)

```
TANPA Verifikasi Sertifikat (setInsecure):
  ESP32 ──► [Hacker berpura-pura jadi server!] ──► Server Asli
             ✅ Data terenkripsi? YA
             ❌ Hacker yg menerima? YA — BERBAHAYA!

DENGAN Verifikasi Sertifikat (setCACert):
  ESP32 ──► Cek KTP server ──► Server Asli
             ✅ Data terenkripsi? YA
             ✅ Identitas server terverifikasi? YA — AMAN!
```

---

## 🔑 Root CA Certificate

Root CA (*Certificate Authority*) adalah organisasi terpercaya yang menandatangani sertifikat server. Contoh: Let's Encrypt, DigiCert, GlobalSign.

### Cara Mendapatkan Root CA dari Browser
1. Buka `https://platform.antares.id` di browser Chrome
2. Klik ikon **Gembok** di address bar → **Connection is secure**
3. Klik **Certificate is valid** → Tab **Details**
4. Di bagian **Certificate Hierarchy**, klik sertifikat paling atas (**Root CA**)
5. Klik **Export** → simpan sebagai `.pem`

### Cara Mendapatkan Root CA via OpenSSL (Terminal)
```bash
openssl s_client -connect platform.antares.id:8443 -showcerts 2>/dev/null \
  | awk '/BEGIN CERTIFICATE/{c++} c==3{print}'
```

> ⚠️ **Root CA punya masa berlaku (expiry date)!**  
> Jika sertifikat kadaluarsa, ESP32 tidak bisa terhubung. Selalu rencanakan mekanisme **OTA Update** untuk memperbarui Root CA saat dibutuhkan.

---

## 📊 Perbandingan Tiga Metode Koneksi HTTPS

| Metode | Enkripsi | Verifikasi Server | Cocok Untuk |
|--------|----------|-------------------|-------------|
| HTTP biasa | ❌ | ❌ | Prototipe lokal |
| `setInsecure()` | ✅ | ❌ | Prototipe HTTPS cepat |
| `setCACert()` | ✅ | ✅ | **Produksi — Standar Industri** |

---

## 🌐 Program 1: HTTPS Cepat dengan `setInsecure()`

### Konsep
`WiFiClientSecure` dengan `.setInsecure()` tetap mengenkripsi data, namun **tidak memverifikasi identitas server**. ESP32 akan menerima sertifikat dari siapa saja — termasuk hacker!

### Tujuan Pembelajaran
- Memahami cara kerja `WiFiClientSecure`
- Terbiasa menggunakan HTTPS sebagai pengganti HTTP biasa
- Memahami **mengapa** ini tidak aman untuk produksi

### Hardware
- OLED: tampilkan status koneksi dan hasil response

### Kode Program
> Lihat file: [`p1_https_insecure/p1_https_insecure.ino`](p1_https_insecure/p1_https_insecure.ino)

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 49 Program 1: HTTPS — setInsecure() ===
[WARN] Mode INSECURE: hanya untuk prototipe/debugging!
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[HTTPS] ⚠️ Mode: setInsecure() — verifikasi sertifikat DIMATIKAN
[INFO] Fetch tiap 60 detik
[HTTPS] GET https://httpbin.org/get
[HTTPS] ✅ 200 OK | IP Publik: 114.10.20.30 | Heap: 198400 B
[HB] Uptime: 60 s | Heap: 197200 B
```

---

## 🔒 Program 2: HTTPS Produksi dengan `setCACert()`

### Konsep
`WiFiClientSecure` dengan `.setCACert(rootCA)` memverifikasi identitas server menggunakan Root CA yang kita percayai. Jika sertifikat server tidak cocok dengan Root CA kita, koneksi **DITOLAK**.

### Tujuan Pembelajaran
- Memahami cara menyisipkan Root CA ke dalam kode
- Memahami rantai sertifikat *(Certificate Chain)*
- Mengerti perbedaan keamanan antara `setInsecure()` dan `setCACert()`

### Hardware
- OLED: tampilkan status verifikasi CA dan origin koneksi

### Kode Program
> Lihat file: [`p2_https_cacert/p2_https_cacert.ino`](p2_https_cacert/p2_https_cacert.ino)

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 49 Program 2: HTTPS — setCACert() ===
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[TLS] Mode: setCACert() — verifikasi sertifikat AKTIF ✅
[INFO] Fetch tiap 60 detik
[HTTPS] GET https://httpbin.org/get
[TLS] Memverifikasi sertifikat server dengan Root CA...
[HTTPS] ✅ 200 OK — Server terverifikasi! | IP Publik: 114.10.20.30 | Heap: 195200 B
[HB] Uptime: 60 s | Heap: 194800 B
```

---

## ☁️ Program 3: HTTPS POST DHT11 ke Antares.id

### Tentang Antares.id
**Antares** adalah platform IoT *cloud* karya **TELKOM Indonesia**, dibangun di atas standar internasional **oneM2M**. Platform ini menyediakan:
- Dashboard monitoring sensor real-time
- Riwayat data tersimpan di cloud
- REST API standar internasional
- Gratis untuk penggunaan edukasi!

### Cara Daftar Antares.id (Gratis)
1. Buka [https://antares.id](https://antares.id) → **Daftar**
2. Verifikasi email → Login
3. Di dashboard: buat **Application** (misal: `bluino-kit`)
4. Di dalam Application: buat **Device** (misal: `sensor-ruangan`)
5. Salin **Access Key** dari menu profil (kanan atas)
6. Salin nama Application dan Device Anda
7. Masukkan ketiganya ke dalam kode Program 3

### Format Request Antares (oneM2M)
```
POST https://platform.antares.id:8443/~/antares-cse/antares-id/{APP}/{DEVICE}

Header:
  X-M2M-Origin: {ACCESS_KEY}
  Content-Type: application/json;ty=4
  Accept: application/json

Body:
{
  "m2m:cin": {
    "con": "{\"temp\":29.1, \"humidity\":65}"
  }
}
```

> **Mengapa ada format `m2m:cin`?**
> Antares menggunakan standar **oneM2M** — protokol IoT internasional yang sama digunakan oleh Telkom Korea, NTT Jepang, dan Deutsche Telekom. Dengan mempelajari Antares, Anda sudah belajar standar IoT korporat global!

### Hardware
- DHT11 (IO27): baca suhu & kelembaban
- OLED: tampilkan status upload, suhu, dan total upload

### Kode Program
> Lihat file: [`p3_https_antares/p3_https_antares.ino`](p3_https_antares/p3_https_antares.ino)

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 49 Program 3: HTTPS POST — Antares.id ===
[INFO] Endpoint: platform.antares.id:8443
[INFO] App: bluino-kit | Device: sensor-ruangan
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[DHT] ✅ Baca OK: 29.1°C | 65%
[DHT] 29.1°C | 65%
[HTTPS] POST → platform.antares.id:8443
[HTTPS] ✅ 201 Created — Upload #1 berhasil!
[HB] 29.1°C 65% | Upload: 1 | Heap: 193400 B
```

> **Kenapa response code 201, bukan 200?**
> Server merespons `201 Created` karena kita membuat *Content Instance* baru di platform Antares, bukan sekadar mengambil data. Ini adalah perilaku standar HTTP/oneM2M.

---

## 🔑 Rangkuman Konsep Keamanan IoT

| Prinsip | Penjelasan |
|---------|-----------|
| **Enkripsi** | Data tidak bisa dibaca pihak ketiga saat transit |
| **Autentikasi server** | Verifikasi bahwa server yang kita hubungi adalah server yang benar |
| **Autentikasi client** | Server memverifikasi bahwa kita adalah client yang sah (via API Key) |
| **Certificate lifecycle** | Root CA punya masa berlaku — selalu siapkan mekanisme OTA update |

---

## 🔗 Referensi

- [Antares Documentation](https://antares.id/id/docs.html)
- [WiFiClientSecure — ESP32 Arduino Core](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure)
- [ArduinoJson Documentation](https://arduinojson.org)
