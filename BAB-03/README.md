# BAB 3: Dasar Elektronika untuk Mikrokontroler

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami Hukum Ohm dan menghitung nilai resistor
- Memahami konsep pembagi tegangan (voltage divider)
- Menjelaskan perbedaan tegangan 3.3V dan 5V serta risikonya
- Memahami fungsi komponen dasar (resistor, LED, kapasitor, dioda, transistor)
- Melakukan pengukuran dasar menggunakan multimeter

---

## 3.1 Konsep Dasar Listrik

### Tegangan, Arus, dan Hambatan

Bayangkan aliran air dalam pipa:

| Konsep Listrik | Analogi Air | Simbol | Satuan |
|---------------|-------------|--------|--------|
| **Tegangan (Voltage)** | Tekanan air | V | Volt (V) |
| **Arus (Current)** | Debit air | I | Ampere (A) |
| **Hambatan (Resistance)** | Diameter pipa | R | Ohm (Ω) |

### Hukum Ohm

```
V = I × R

Di mana:
V = Tegangan (Volt)
I = Arus (Ampere)  
R = Hambatan (Ohm)
```

### Contoh Perhitungan: Resistor untuk LED

Pada kit Bluino, setiap LED 5mm sudah dilengkapi resistor pembatas arus **330Ω**. Mari kita hitung arusnya:

```
Diketahui:
- Tegangan sumber (V)     = 3.3V (dari GPIO ESP32)
- Forward voltage LED (Vf) = ~2.0V (LED merah)
- Resistor (R)             = 330Ω

Hitung arus:
V_resistor = V - Vf = 3.3V - 2.0V = 1.3V
I = V_resistor / R = 1.3V / 330Ω = 3.9 mA ✅

Ini aman karena:
- LED butuh 2-20 mA
- GPIO ESP32 max 12 mA (recommended)
```

> **Pada kit Bluino:** Resistor 330Ω sudah terintegrasi di PCB. Kamu tidak perlu menambahkannya lagi.

---

## 3.2 Pembagi Tegangan (Voltage Divider)

Pembagi tegangan adalah rangkaian yang menggunakan 2 resistor untuk menurunkan tegangan.

### Rumus

```
        Vin
         │
        ┌┤ R1
        │┤
Vout ───┤
        │┤
        └┤ R2
         │
        GND

Vout = Vin × R2 / (R1 + R2)
```

### Dimana Pembagi Tegangan Dipakai di Kit Bluino?

**1. Sensor LDR (Light Dependent Resistor)**

LDR dirangkai seri dengan resistor tetap 10KΩ membentuk voltage divider (10KΩ di atas, LDR di bawah):

```
    3.3V
     │
    ┌┤ 10KΩ (resistor tetap)
    │┤
    ├──── Vout → ke pin ADC ESP32
    │┤
    └┤ LDR (resistansi berubah sesuai cahaya)
     │
    GND
```

- Terang → LDR resistansi kecil → Vout ditarik ke GND → ADC rendah
- Gelap → LDR resistansi besar → Vout ditarik ke 3.3V → ADC tinggi

**2. HC-SR04 Echo Pin (jika digunakan)**

HC-SR04 output echo = 5V, tapi ESP32 hanya toleran 3.3V! Perlu voltage divider:

```
    Echo (5V)
      │
     ┌┤ 1KΩ
     │┤
     ├──── ke GPIO ESP32 (3.3V aman)
     │┤
     └┤ 2KΩ
      │
     GND

Vout = 5V × 2K / (1K + 2K) = 3.33V ✅
```

> ⚠️ **PENTING:** Menghubungkan sinyal 5V langsung ke GPIO ESP32 tanpa voltage divider **BISA MERUSAK** chip ESP32!

---

## 3.3 Tegangan 3.3V vs 5V — Mengapa Ini Krusial?

### ESP32 Beroperasi pada 3.3V Logic

| Parameter | Nilai |
|-----------|-------|
| Tegangan operasi | 3.3V |
| GPIO output HIGH | ~3.3V |
| GPIO input threshold HIGH | >2.475V (75% dari 3.3V) |
| GPIO input threshold LOW | <0.825V (25% dari 3.3V) |
| ⚠️ **Tegangan input maksimum** | **3.6V** |

### Komponen 5V pada Kit Bluino

Beberapa komponen beroperasi pada 5V:

| Komponen | Tegangan Operasi | Aman untuk ESP32? |
|----------|-----------------|-------------------|
| LED 5mm | 3.3V (via resistor) | ✅ Aman — didrive langsung dari GPIO |
| WS2812 | 3.3V (power & data) | ✅ Aman — pada kit ini terhubung ke 3.3V |
| Relay 5V | 5V (koil) | ✅ Aman — menggunakan transistor driver |
| Servo | 5V (power), 3.3V (sinyal) | ✅ Aman — sinyal 3.3V cukup |
| HC-SR04 | 5V (power + echo) | ⚠️ **Echo = 5V** — PERLU voltage divider! |
| OLED | 3.3V | ✅ Aman |
| DHT11 | 3.3V–5V | ✅ Aman pada 3.3V |

---

## 3.4 Komponen Elektronika Dasar

### LED (Light Emitting Diode)

LED adalah dioda yang memancarkan cahaya. LED memiliki **polaritas** — hanya mengalirkan arus dalam satu arah.

```
         Anoda (+)    Katoda (-)
           │             │
           ├──►|─────────┤
           │    LED       │
```

- **Anoda (+):** Kaki lebih panjang — hubungkan ke sumber tegangan (GPIO)
- **Katoda (-):** Kaki lebih pendek — hubungkan ke GND

> **Kit Bluino:** LED sudah terpasang di PCB dengan resistor 330Ω dan orientasi yang benar. Beri sinyal **HIGH** untuk menyalakan (active-high).

### Resistor

Membatasi arus listrik. Nilai dibaca dari gelang warna:

| Pada Kit Bluino | Nilai | Dipakai Untuk |
|----------------|-------|---------------|
| Resistor LED | 330Ω | Pembatas arus 4 LED |
| Resistor pull-up DHT11 | 4.7KΩ | Pull-up data line DHT11 & DS18B20 |
| Resistor pull-down button | 10KΩ | Pull-down 2 push button |
| Resistor LDR | 10KΩ | Voltage divider sensor cahaya |
| Resistor relay LED | 1KΩ | LED indikator relay |

### Kapasitor

Menyimpan dan melepaskan muatan listrik. Digunakan untuk:
- **Filtering:** Menghaluskan tegangan (menghilangkan ripple)
- **Decoupling:** Menghilangkan noise pada power supply

> **Kit Bluino:** Kapasitor 100µF/6V (C1) dan kapasitor keramik (C2) digunakan pada power regulator untuk menstabilkan tegangan 5V.

### Dioda

Mengalirkan arus hanya dalam satu arah.

| Dioda di Kit | Fungsi |
|-------------|--------|
| **1N4001** | Proteksi polaritas terbalik pada input daya 12V |
| **1N4148** | Dioda flyback — melindungi transistor dari spike tegangan saat relay dimatikan |

### Transistor sebagai Saklar

Transistor bisa berfungsi sebagai **saklar elektronik** yang dikendalikan oleh sinyal kecil.

```
     5V
      │
   ┌──┤ Relay Coil
   │  │
   │  └──┤ 1N4148 (flyback diode)
   │     │
   │     ├── Collector
   │     │
   │   BC547 (NPN Transistor)
   │     │
   │     ├── Base ── Resistor ── GPIO ESP32
   │     │
   │     ├── Emitter
   │     │
    ─── GND
```

**Cara kerja pada kit Bluino:**
1. GPIO ESP32 mengirim sinyal **HIGH** (3.3V)
2. Arus kecil mengalir ke **Base** transistor BC547
3. Transistor **ON** — arus besar mengalir dari Collector ke Emitter
4. Relay coil **terenergize** — kontak relay menutup
5. Dioda flyback 1N4148 melindungi transistor dari spike tegangan saat relay OFF

> **Mengapa tidak langsung dari GPIO?** Relay membutuhkan arus ~70mA, sedangkan GPIO ESP32 hanya mampu ~12mA. Transistor berfungsi sebagai penguat arus.

---

## 3.5 Regulator Tegangan

### Pada Kit Bluino: IC 78M05

```
   +12V ──── 1N4001 ───── IN │78M05│ OUT ──┬──────┬── +5V
                                    │      │      │
                                 C1 ═     C2 ═    │
                          100µF/6V  │  keramik    │
                                    │      │      │
                                   GND    GND    GND
```

- **Input:** 7V–35V DC (biasanya adaptor 12V)
- **Output:** 5V DC stabil
- **1N4001:** Mencegah kerusakan jika kabel daya (polaritas) terbalik
- **C1 (100µF/6V) & C2 (keramik):** Filter pada **output** untuk menstabilkan tegangan 5V dan menghilangkan noise frekuensi tinggi.

> **Penting:** Untuk penggunaan normal via USB, regulator ini tidak digunakan. Regulator ini dipakai saat kit dipasang pada sistem yang membutuhkan supply 12V eksternal (misalnya pemasangan permanen di rumah).

---

## 3.6 Pengukuran dengan Multimeter

### Alat yang Dibutuhkan

Multimeter digital (DMM) — alat ukur serbaguna untuk:

| Mode | Fungsi | Contoh pada Kit |
|------|--------|-----------------|
| **V DC** | Ukur tegangan DC | Cek 3.3V di pin ESP32 |
| **V AC** | Ukur tegangan AC | Cek tegangan PLN (hati-hati!) |
| **Ω** | Ukur resistansi | Cek nilai resistor |
| **Continuity** | Cek koneksi | Cek kabel jumper, cek solder |
| **mA** | Ukur arus | Ukur konsumsi daya ESP32 |

### Pengukuran Penting untuk Kit Bluino

1. **Cek tegangan 3.3V:** Ukur antara pin 3V3 dan GND — harus ~3.3V
2. **Cek tegangan 5V:** Ukur antara pin VIN dan GND (saat via USB) — harus ~5V
3. **Cek resistor:** Set ke mode Ω, ukur resistor 330Ω — harus ~330Ω
4. **Cek koneksi jumper:** Set ke mode Continuity, sentuh kedua ujung — harus berbunyi

---

## 3.7 Rangkuman Konsep

```
┌─────────────────────────────────────────────────────────┐
│                 YANG HARUS DIINGAT                      │
├─────────────────────────────────────────────────────────┤
│ ✅ V = I × R (Hukum Ohm)                               │
│ ✅ ESP32 logic = 3.3V, max input = 3.6V                 │
│ ✅ JANGAN hubungkan 5V langsung ke GPIO ESP32            │
│ ✅ Gunakan voltage divider untuk sinyal 5V               │
│ ✅ LED butuh resistor pembatas arus                      │
│ ✅ Relay butuh transistor driver (arus besar)            │
│ ✅ Dioda flyback melindungi transistor dari spike relay   │
│ ✅ Kapasitor menstabilkan power supply                   │
└─────────────────────────────────────────────────────────┘
```

---

## 📝 Latihan

1. **Hitung resistor LED:** Kamu ingin menyalakan LED hijau (Vf = 2.2V) dari GPIO 3.3V ESP32 dengan arus 5mA. Berapa nilai resistor yang diperlukan?

2. **Voltage divider:** Kamu punya sinyal 5V dari sensor dan ingin menurunkan ke 3.3V menggunakan voltage divider. Jika R2 = 10KΩ, berapa R1 yang dibutuhkan? (Petunjuk: gunakan rumus Vout = Vin × R2 / (R1 + R2))

3. **Relay driver:** Mengapa kita tidak bisa menghubungkan relay langsung ke GPIO ESP32 tanpa transistor? Apa yang terjadi jika kita memaksa?

4. **Identifikasi komponen:** Lihat schematic kit Bluino (file `Schematic.jpg`). Identifikasi:
   - Berapa buah dioda yang digunakan? Apa fungsi masing-masing?
   - Dimana kapasitor ditempatkan dan apa fungsinya?
   - Transistor apa yang digunakan dan untuk apa?

5. **Keamanan 3.3V:** Apa yang terjadi pada ESP32 jika kamu menghubungkan pin echo HC-SR04 (5V) langsung ke GPIO ESP32 tanpa voltage divider?

---

## 📚 Referensi

- [Ohm's Law Calculator](https://ohmslawcalculator.com/)
- [LED Resistor Calculator](https://ledcalculator.net/)
- [ESP32 GPIO Maximum Ratings (Datasheet)](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [Voltage Divider Calculator](https://ohmslawcalculator.com/voltage-divider-calculator)

---

[⬅️ BAB 2: Pengenalan ESP32](../BAB-02/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 4 — Membaca Schematic ➡️](../BAB-04/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

