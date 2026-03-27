# BAB 26: Sensor Suhu & Kelembaban Udara (DHT11)

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menjelaskan perbedaan fundamental antara sensor suhu presisi DS18B20 dengan sensor cuaca lingkungan seperti DHT11/DHT22.
- Mengirim dan menerima data menggunakan protokol komunikasi *Singe-Bus* khusus milik keluarga DHT.
- Membaca tidak hanya nilai fisik Suhu (°C), tetapi juga Kelembaban Relatif Udara (% RH).
- Menghitung **Heat Index** (Indeks Panas Terasa) berbasis suhu dan kelembaban.
- Menangani *error handling* bawaan mikrokontroler jika pembacaan `float` mengembalikan `NaN` (*Not a Number*).
- Menghindari kesalahan *sampling frequency limit* (aturan wajib delay 2 detik) yang paling sering menjebak pemula pada DHT11.

---

## 26.1 Mengenal Keluarga Sensor DHT (Digital Humidity and Temperature)

Jika DS18B20 difokuskan murni pada presisi suhu absolut (-55°C hingga +125°C), **DHT11** (dan kakaknya, DHT22) bertugas memonitor cuaca alias suhu ruangan dan **Kelembaban Relatif (Relative Humidity - % RH)**. Sesuai dengan singkatannya penciptanya, ini adalah modul *Digital Humidity and Temperature* multifungsi.

Sensor kelembaban (humidity) sangat krusial dalam IoT agrikultur (Greenhouse), inkubator penetas telur, penyimpan obat/vaksin, dan kontrol AC (Air Conditioner) gedung cerdas.

### Perbandingan Keluarga DHT

```
┌─────────────────────────────────────────────────────────────┐
│                    DHT11 vs DHT22                           │
├─────────────┬────────────────────────┬──────────────────────┤
│ Fitur       │ DHT11 (Biru)           │ DHT22 / AM2302 (Putih)
├─────────────┼────────────────────────┼──────────────────────┤
│ Kelembaban  │ 20% – 90% (±5%)        │ 0% – 100% (±2%)      │
│ Suhu        │ 0°C – 50°C (±2°C)      │ -40°C – 80°C (±0.5°C)│
│ Resolusi    │ 1 bit (0.1)            │ 16 bit (0.1)         │
│ Sampling Mks│ 1 Hz (Tiap 1-2 detik)  │ 0.5 Hz (Tiap 2 detik)│
│ Harga       │ Sangat Murah           │ Lebih Mahal          │
└─────────────┴────────────────────────┴──────────────────────┘
```

> ✅ **Kit Bluino menggunakan DHT11 (Cangkang Biru).** Sensor ini sangat tahan banting untuk eksperimen kelas, namun memiliki batas *sampling* — ia **tidak boleh dibaca lebih cepat dari 1 kali per detik (idealnya 2 detik sekali)**.

---

## 26.2 Protokol Komunikasi DHT (Bukan 1-Wire Standar!)

DHT11 menggunakan protokol *Single-Bus* miliknya sendiri yang dikembangkan oleh Aosong, **Bukan protokol 1-Wire buatan Dallas** seperti di BAB 25. 

Artinya: **Kamu tidak bisa menggabungkan DS18B20 dan DHT11 pada kabel yang sama!** Mereka tidak berbagi *ROM Code* maupun standar pewaktuan yang sama. Setiap sensor DHT membutuhkan 1 pin digitalnya sendiri secara eksklusif.

### Konfigurasi Hardware di Kit Bluino
```
           Resistor Pull-up
ESP32 IO27 ──[4K7Ω]── VCC 3.3V
              │
              └─────────────── Pin DATA Sensor DHT11
```

> ✅ **Kabar Baik:** Jalur Data dari sensor DHT11 di Kit Bluino telah disambungkan secara permanen ke **Pin IO27**, lengkap dengan *resistor pull-up* sebesar 4K7Ω di atas PCB. Kamu tidak perlu memasang kabel *jumper* tambahan!

---

## 26.3 Instalasi Library Wajib

Sensor DHT merespons permintaan mikrokontroler dengan mengirim rentetan persis **40 bit data** (~5 byte) yang terdiri dari data kelembaban integral, pecahan, suhu integral, pecahan, dan *checksum* paritas. 

Proses pembacaan 40 bit ini memiliki toleransi waktu mikrosekon. Oleh karena itu, kita **wajib** menggunakan *library* yang telah ditulis dalam *Assembly* atau optimasi tingkat rendah.

**Langkah Instalasi:**
```
Library Manager (Ctrl+Shift+I) → Ketik "DHT sensor library"

1. Instal: "DHT sensor library by Adafruit" 
             (Pilih "Install All" jika ditanya dependensi!)
2. Pastikan: "Adafruit Unified Sensor" juga ikut terinstal.
```

> ⚠️ Tanpa *Adafruit Unified Sensor*, library utama DHT akan gagal saat dikompilasi!

---

## 26.4 Program 1: Pembacaan Cuaca Dasar (Handling Error NaN)

Program ini adalah pondasi utama pembacaan cuaca. Perhatikan baik-baik teknik *error handling* menggunakan logika `isnan()` (*Is Not a Number*).

```cpp
/*
 * BAB 26 - Program 1: Pembacaan Suhu & Kelembaban Dasar
 * 
 * Target: Membaca parameter iklim ruangan dengan limitasi 2 detik
 * Wiring (Topologi Kit Bluino):
 *   Pin Data DHT11 → IO27 (Pull-up internal PCB siap jalan)
 */

#include "DHT.h" // Library resmi Adafruit

// ── Konfigurasi Pin dan Tipe ────────────────────────────────────
#define DHTPIN  27       // Pin Data terpasang ke IO27
#define DHTTYPE DHT11    // Ubah ke DHT22 jika memakai sensor putih!

// ── Objek Konstruktor ───────────────────────────────────────────
DHT dht(DHTPIN, DHTTYPE); 

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("   BAB 26: DHT11 — Suhu & Kelembaban");
  Serial.println("══════════════════════════════════════════");

  // Inisialisasi antarmuka sensor 
  dht.begin();
  
  Serial.println("✅ Sensor DHT11 siap!");
  Serial.println("Dibutuhkan 2 detik untuk warm-up pertama...");
  Serial.println("──────────────────────────────────────────");
}

void loop() {
  // ⏳ ATURAN EMAS DHT11: 
  // Jeda WAJIB antar pembacaan adalah 2 detik. Membaca lebih 
  // cepat dari ini akan membuat sensor "hang" atau membalas Error.
  delay(2000); 

  // Ambil Kelembaban Relatif (Relative Humidity - %)
  float h = dht.readHumidity();
  // Ambil Suhu dalam derajat Celcius
  float t = dht.readTemperature();
  // (Opsional) Ambil Suhu dalam derajat Fahrenheit: dht.readTemperature(true)

  // ── Penanganan Bencana (Error Handling) ───────────────────────
  // Jika pin terputus/rusak, Arduino akan mengembalikan tipe Float 
  // yang bukan angka riil, melainkan "NaN" (Not a Number).
  // Kita WAJIB memblokir nilai NaN ini agar tidak merusak logika IoT!
  if (isnan(h) || isnan(t)) {
    Serial.println("❌ ERROR: Gagal membaca data dari DHT11!");
    Serial.println("          Cek jalur IO27 dan power 3.3V.");
    return; // Lempar eksekusi kembali ke awal loop, abaikan kode di bawah
  }

  // ── Modul Sukses, Tampilkan Data: ─────────────────────────────
  Serial.printf("💧 Kelembaban: %4.1f %%  |  🌡️ Suhu: %4.1f °C\n", h, t);
}
```

**Contoh output yang diharapkan:**
```text
══════════════════════════════════════════
   BAB 26: DHT11 — Suhu & Kelembaban
══════════════════════════════════════════
✅ Sensor DHT11 siap!
Dibutuhkan 2 detik untuk warm-up pertama...
──────────────────────────────────────────
💧 Kelembaban: 65.0 %  |  🌡️ Suhu: 31.0 °C
💧 Kelembaban: 65.0 %  |  🌡️ Suhu: 31.0 °C
💧 Kelembaban: 66.0 %  |  🌡️ Suhu: 31.1 °C
```

**Penjelasan Alur Program:**
1. Konstruktor `DHT dht(DHTPIN, DHTTYPE)` wajib diisi tipe karena toleransi *timing* internal library DHT11 sangat berbeda dengan DHT22.
2. Fungsi `delay(2000)` di baris awal loop bukan tanpa alasan! Di hardware internal DHT11, konverter Analog/Digital butuh waktu hampir 1,5 detik untuk bereaksi terhadap perubahan gelombang ion air absolut. 
3. Perintah `isnan()` menyelamatkan server-mu! Ingatlah saat mengirim data ke *Database Cloud*, menyuntikkan nilai teks *NaN* ke dalam *Field Server Bertipe Number* akan seketika menghasilkan fatal server error!

---

## 26.5 Program 2: Mengkalkulasi *Heat Index* (Suhu Terasa)

Tahukah kamu bahwa tubuh manusia tidak sekadar merasakan besaran suhu konvensional (°C)? Di iklim tropis seperti Indonesia, kelembaban yang sangat tinggi membuat keringat manusia sulit menguap ke udara. 

Akibatnya? Ruangan bersuhu $31°C$ tetapi dengan kelembaban ekstrim $90\%$, akan dijatuhkan ke otakmu sebagai peringatan bahaya: **Suhu Terasa (Heat Index) seolah mencapai $41°C$!** Ini bisa menyebabkan *Heat Stroke* fatal!

**Mengapa Uap Air Membuat Udara Terasa Lebih Panas?**
Sistem pendingin alami tubuh manusia bertumpu pada penguapan keringat di pori-pori kulit. Namun, saat kelembaban udara (% RH) di ruangan sangat tinggi, atmosfer di sekitarmu sudah terlalu "jenuh/kenyang" oleh uap air. Akibatnya keringat di tubuhmu **ditolak meresap ke udara (gagal menguap)**. Panas tubuh pun akhirnya terperangkap dan memantul ke dalam, menipu otakmu hingga merasa bahwa suhu jauh di atas termometer ruangan!

Library *Adafruit DHT* telah menyertakan algoritma kompleks kalkulus *Rothfusz* peninggalan riset BMKG Amerika (*National Weather Service*) untuk menghitung ini secara presisi!

```cpp
/*
 * BAB 26 - Program 2: Stasiun Peringatan Cuaca (Heat Index)
 * 
 * Membaca suhu dan mengkombinasikannya dengan RH% untuk 
 * menghasilkan "Nilai Suhu Terasa Oleh Kulit Manusia"
 */

#include "DHT.h"

#define DHTPIN 27 
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("══════════════════════════════════════════════");
  Serial.println(" BAB 26: Kalkulator Heat Index Cuaca Tropis");
  Serial.println("══════════════════════════════════════════════");
  
  dht.begin();
}

void loop() {
  delay(2000); // Batas aman rekapitulator DHT11

  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Celcius default

  if (isnan(h) || isnan(t)) {
    Serial.println("❌ Modul DHT11 gagal merespons!");
    return;
  }

  // ── Kalkulasi Algoritma Heat Index (Rothfusz) ─────────────────
  // Parameter parameter ke-3: isFahrenheit = false (Artikan Celcius!)
  float heatIndexCuaca = dht.computeHeatIndex(t, h, false);

  Serial.printf("Suhu Ruangan  : %5.1f °C\n", t);
  Serial.printf("Kelembaban    : %5.1f %% RH\n", h);
  Serial.printf("Terasa Seperti: %5.1f °C   ", heatIndexCuaca);

  // Analisa Bahaya berdasarkan Klasifikasi Medis Global
  if (heatIndexCuaca < 27.0) {
    Serial.println("(Zon Nyaman / Sejuk) 🌿");
  } else if (heatIndexCuaca < 32.0) {
    Serial.println("(Waspada: Mulai Berkeringat) 😓");
  } else if (heatIndexCuaca < 41.0) {
    Serial.println("(SANGAT WASPADA: Kram Panas & Kelelahan Ekstrem!) ⚠️");
  } else {
    Serial.println("(BAHAYA: Potensi Heat Stroke Medis!) 🚑");
  }
  
  Serial.println("──────────────────────────────────────────────");
}
```

**Contoh Output di Siang Hari Musim Hujan:**
```
Suhu Ruangan  :  31.0 °C
Kelembaban    :  85.0 % RH
Terasa Seperti:  40.4 °C   (SANGAT WASPADA: Kram Panas & Kelelahan Ekstrem!) ⚠️
──────────────────────────────────────────────
```
*Gila bukan? Mesin mikrokontroler murah milikmu kini mampu memberikan penasihat medis instan seakurat satelit cuaca klimatologi!*

---

## 26.6 Program 3: Arsitektur Sensor *Non-Blocking* Industri

Meskipun sensor DHT butuh waktu 2000 milidetik *Cooldown* (jeda pendinginan), menggunakan `delay(2000)` benar-benar **terlarang** dalam arsitektur sistem tertanam (*Embedded OS*) profesional. Mesin ESP32 berlari sangat kencang, menidurkan ESP32 secara buta mematikan semua *Web Server* HTTP, pembaca UI, dan aliran LCD!

Mari translasikan ke pola *Non-Blocking Polling* yang diorkestrasi oleh `millis()`.

> 💡 **Apa itu `struct` (Structure) pada kode C++ di bawah?**
> Bayangkan `struct` sebagai sebuah *kardus boks pengiriman* yang didalamnya berisi beberapa laci variabel berbeda (`suhu`, `hum`, `sukses`). Pola ini (*Data Register*) memudahkan mikrokontroler membawa seluruh kumpulan laporan iklim hanya dalam satu genggaman variabel tunggal bernama `cuacaTerakhir` untuk dikirim ke OLED atau Web Server di fungsi lain.

```cpp
/*
 * BAB 26 - Program 3: DHT11 Pola Arsitektur Non-Blocking
 * Mengukur iklim cuaca di *background* tanpa menghalangi loop utama!
 */

#include "DHT.h"

#define DHTPIN 27 
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ── Timer State Variables ───────────────────────────────────────
unsigned long tBacaTerakhir = 0;
const unsigned long INTERVAL_BACA_MS = 2000; // Limit mutlak DHT11!

// ── Data Register ───────────────────────────────────────────────
// (Tersedia untuk diambil oleh fungsi web atau display lain kapan saja!)
struct DataCuaca {
  float suhu = NAN;
  float hum  = NAN;
  float heat = NAN;
  bool  sukses = false;
};

DataCuaca cuacaTerakhir;

// Fungsi helper bersih untuk menarik data di luar loop utama
void perbaruiDataDHT() {
  unsigned long sekarang = millis();

  // Kunci batas kecepatan: Jangan mengeksekusi jika belum lewat 2 detik!
  // 💡 Trik Matematika WARM-UP: Karena tBacaTerakhir diawali nilai 0 di baris atas,
  // mikrokontroler akan secara alami diam (menolak masuk ke if ini) selama 2000 milidetik 
  // pertama sejak booting. Ini secara otomatis memberikan *boot-delay* yang diwajibkan 
  // oleh hardware fisik DHT11 tanpa kita perlu menulis perintah delay() pemblokir arus!
  if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
    tBacaTerakhir = sekarang;

    float hum = dht.readHumidity();
    float suhu = dht.readTemperature();

    if (isnan(hum) || isnan(suhu)) {
      cuacaTerakhir.sukses = false;
      // 💡 Sifat Toleransi Kesalahan (Fault Tolerance):
      // Perhatikan bahwa kita TIDAK mengubah nilai `cuacaTerakhir.suhu` menjadi NAN di sini!
      // Nilai terakhir yang valid akan tetap dipertahankan (Latch-on-Fail) agar layar OLED
      // atau antarmuka server klien tidak tiba-tiba kosong hanya karena sebutir paket data
      // mengalami gangguan minor sesaat!
      Serial.printf("[%6lu ms] ❌ Kegagalan Paritas Checksum Sensor!\n", sekarang);
    } else {
      cuacaTerakhir.suhu = suhu;
      cuacaTerakhir.hum  = hum;
      cuacaTerakhir.heat = dht.computeHeatIndex(suhu, hum, false);
      cuacaTerakhir.sukses = true;
      
      // Catat di log hanya ketika data benar-benar memiliki pembaruan riil
      Serial.printf("[%6lu ms] 🌧️ Data Masuk: %.1f°C | RH: %.1f%%\n",
                    sekarang, suhu, hum);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("══════════════════════════════════════════════");
  Serial.println(" BAB 26: Asynchronous Weather Threading");
  Serial.println("══════════════════════════════════════════════");
  
  dht.begin();
}

void loop() {
  // 1. Jalankan pemburu cuaca asinkron di latar belakang
  perbaruiDataDHT();

  // 2. Mesin ESP32 bebas berlari puluhan ribu kali sedetik di sini!
  //
  // if (tombolDitekan) displayDataOLED(cuacaTerakhir.suhu);
  // if (clientWebMasuk) webServer.kirimJson(cuacaTerakhir);
  //
}
```

**Contoh output yang diharapkan:**
```text
══════════════════════════════════════════════
 BAB 26: Asynchronous Weather Threading
══════════════════════════════════════════════
[  2001 ms] 🌧️ Data Masuk: 30.5°C | RH: 70.0%
[  4001 ms] 🌧️ Data Masuk: 30.5°C | RH: 70.0%
[  6001 ms] 🌧️ Data Masuk: 30.6°C | RH: 71.0%
```

Program di atas menjamin bahwa sensor akan "didatangi dan ditanyai tegangannya" secara teliti setiap tepat 2000 ms, tak selisih sesendok waktu pun! Mesin IoT mu sekarang memiliki detak jantung abadi.

---

## 26.7 Tips & Panduan Troubleshooting Bebas Pusing

### 1. `Serial.println` Mengeluarkan Teks `nan` Secara Acak
```text
Kelembaban: nan %   |   Suhu: nan °C
```
Ini berarti mikrokontroler ESP32 kamu **gagal menerima balasan sinyal digital 40-bit** dari sensor. Pada modul DHT11, pembacaannya bergantung pada komponen *NTC Thermistor* (untuk suhu) dan substrat polimer resistif (untuk kelembaban) yang kemudian diubah menjadi paket digital oleh chip IC 8-bit internal di dalam cangkang biru tersebut.
- **Penyebab A:** Pengkabelan pin IO27 bermasalah (kendur/terlepas), sehingga sinyal digital terputus di tengah jalan.
- **Penyebab B:** Sinyal tercampur *noise* listrik statis, sehingga menembagakan *Checksum* Paritas gagal di verifikasi library.
- **Penyebab C:** IC internal DHT11 macet (*hardware hang*). Kadang mencabut dan memasang kembali pin VCC sensor (me-reset daya) akan menormalkan kembali logika 8-bit di dalamnya.
- **Penyebab D:** Kamu melanggar hukum termodinamika instrumen dengan memaksa frekuensi di bawah `2000` ms!

### 2. Apakah saya harus membuang nilai `NaN`?
**HARUS!** Di bahasa standar C++, nilai apa pun jika dijumlahkan, dikalikan, apalagi dikirim ke database JSON yang bersumber dari elemen `NAN` akan menularkan virus kelumpuhan `null / nan` ke seluruh operasi matematika algoritma server. Selalu letakkan barikade gerbang awal `if (isnan(h)) return;`.

### 3. Modul DHT11 Menunjukkan Nilai Selisih Terus-menerus?
Resistor substrat milik DHT11 tidak dikalibrasi ganda oleh mesin laser (*non-laser trimmed*). Mereka hanya mampu membaca kelipatan desimal bulat kasaran `(.0)`. Selisih toleransi ±2°C di DHT11 adalah kodrat alam semesta yang lazim untuk perangkat sensor lingkungan kelas pelajar.

---

## 26.8 Ringkasan

```
┌─────────────────────────────────────────────────────────────┐
│             RINGKASAN BAB 26 — ESP32 + DHT11                │
├─────────────────────────────────────────────────────────────┤
│ Jenis Bus      = Single-Bus Khusus Milik Produsen Aosong    │
│ Pin Kit Bluino = IO27                                       │
│ Sampling Wait  = WAJIB >= 2 Detik Sekali! (Maks 1 Hz)       │
│                                                             │
│ Prosedur Wajib Setup:                                       │
│   DHT dht(27, DHT11);                                       │
│   dht.begin();                                              │
│                                                             │
│ Prosedur Baca Float Memory:                                 │
│   float t = dht.readTemperature();                          │
│   float h = dht.readHumidity();                             │
│                                                             │
│ Pelindung Kegagalan Sistem (SANGAT KRITIKAL):               │
│   if(isnan(h) || isnan(t)) {                                │
│       // Sensor Hang! Buang data, jangan teruskan ke Cloud! │
│   }                                                         │
│                                                             │
│ Fungsi Medis Tambahan Dari Library:                         │
│   float heat = dht.computeHeatIndex(t, h, false);           │
└─────────────────────────────────────────────────────────────┘
```

---

## 26.9 Latihan

1. **Kotak Suhu Penyimpan Vaksin:** Tulislah pola gabungan BAB 26 dan sistem LED di mana LED Hijau akan menyala saat suhu DHT11 berada antara $16°C \sim 25°C$, namun saat melebihi nilai tersebut, *Buzzer* dan LED Merah mendadak melengking memperingatkan ancaman keracunan medis.
2. **OLED Weather Station:** Kombinasikan struktur `DataCuaca` non-blocking di Program 3! Lukislah teks rapi di layar OLED (BAB 23) dengan memisahkan wilayah atas kiri untuk "SUHU" dan atas kanan untuk "KELEMBABAN". Pastikan tampilan antarmuka (UI) akan tergantikan oleh kata *"OFFLINE!"* jika `data.sukses == false`.
3. **Data Logger Iklim:** Masukkan dua variabel `h` dan `t` tersebut ke dalam baris penulisan string `.CSV` menuju modul SPI TF Micro SD (BAB 24)! Inilah proyek industri riil IoT pencatatan pergudangan sembako dan palet logistik beras Bulog di stasiun gudang besar.

---

*Selanjutnya → [BAB 27: Sinkronisasi Waktu Lokal via NTP Server Jaringan](../BAB-27/README.md)*
