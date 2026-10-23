/*
 * BAB 40 - Program 4: File Konfigurasi
 * Pengaturan disimpan di /config.txt dengan format kunci=nilai:
 *   nama=Bluino-01
 *   kedip_ms=500
 *   led=1
 * - Saat menyala, /config.txt dibaca. Kalau belum ada, dibuat dari nilai bawaan.
 * - LED biru (IO2) berkedip sesuai kedip_ms, kalau led=1.
 * Perintah Serial Monitor (line ending: New Line):
 *   lihat                -> pengaturan yang sedang dipakai
 *   isi                  -> isi mentah /config.txt
 *   set <kunci> <nilai>  -> ubah pengaturan (belum disimpan)
 *   simpan               -> simpan ke /config.txt
 *   muat                 -> baca ulang /config.txt
 *   bawaan               -> kembali ke nilai bawaan (belum disimpan)
 * Rangkaian: LED3 (biru) -> IO2 (opsional; LED biru di board juga di IO2)
 */

#include <LittleFS.h>

#define LED_BIRU 2

const char *FILE_KONFIG = "/config.txt";
const char *FILE_SEMENTARA = "/config.tmp";
const unsigned int PANJANG_MAKS = 60;  // batas panjang satu perintah

// Nilai bawaan
const char *NAMA_BAWAAN = "Bluino-01";
const unsigned long KEDIP_BAWAAN = 500;

// Pengaturan yang sedang dipakai (di RAM)
String nama = NAMA_BAWAAN;
unsigned long kedipMs = KEDIP_BAWAAN;
bool ledAktif = true;

bool fsSiap = false;
String penampung = "";
unsigned long waktuKedipLalu = 0;
bool ledNyala = false;

void aturBawaan() {
  nama = NAMA_BAWAAN;
  kedipMs = KEDIP_BAWAAN;
  ledAktif = true;
}

// Memeriksa lalu memakai satu pasangan kunci-nilai. Hasil: true kalau diterima.
bool terapkan(String kunci, String nilai) {
  kunci.trim();
  kunci.toLowerCase();
  nilai.trim();
  if (kunci == "nama") {
    if (nilai.length() == 0 || nilai.length() > 20) {
      Serial.println("  nama harus 1-20 karakter");
      return false;
    }
    nama = nilai;
  } else if (kunci == "kedip_ms") {
    long angka = nilai.toInt();  // teks bukan angka -> 0
    if (angka < 50 || angka > 5000) {
      Serial.println("  kedip_ms harus angka 50-5000");
      return false;
    }
    kedipMs = angka;
  } else if (kunci == "led") {
    if (nilai == "1") {
      ledAktif = true;
    } else if (nilai == "0") {
      ledAktif = false;
    } else {
      Serial.println("  led harus 1 (berkedip) atau 0 (mati)");
      return false;
    }
  } else {
    Serial.printf("  kunci '%s' tidak dikenal\n", kunci.c_str());
    return false;
  }
  return true;
}

// Membaca /config.txt baris demi baris. Hasil: false kalau file tidak ada.
bool muatKonfigurasi() {
  File f = LittleFS.open(FILE_KONFIG, "r");
  if (!f) {
    return false;
  }
  aturBawaan();  // kunci yang tidak tertulis di file memakai nilai bawaan
  int nomorBaris = 0;
  while (f.available()) {
    String baris = f.readStringUntil('\n');
    nomorBaris = nomorBaris + 1;
    baris.trim();  // buang spasi dan '\r'
    if (baris.length() == 0 || baris.startsWith("#")) {
      continue;  // lewati baris kosong dan komentar
    }
    int samaDengan = baris.indexOf('=');
    if (samaDengan < 0) {
      Serial.printf("  baris %d dilewati: tidak ada tanda '='\n", nomorBaris);
      continue;
    }
    if (!terapkan(baris.substring(0, samaDengan), baris.substring(samaDengan + 1))) {
      Serial.printf("  baris %d dilewati\n", nomorBaris);
    }
  }
  f.close();
  return true;
}

size_t ukuranFile(const char *path) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    return 0;
  }
  size_t ukuran = f.size();
  f.close();
  return ukuran;
}

// Menyimpan dengan aman: tulis ke file sementara, periksa, baru ganti nama
bool simpanKonfigurasi() {
  String isi = "";
  isi += "# Pengaturan Program 4 BAB 40\r\n";
  isi += "# Format: kunci=nilai, satu per baris. Baris diawali # diabaikan.\r\n";
  isi += "nama=" + nama + "\r\n";
  isi += "kedip_ms=" + String(kedipMs) + "\r\n";
  isi += "led=" + String(ledAktif ? 1 : 0) + "\r\n";

  File f = LittleFS.open(FILE_SEMENTARA, "w");
  if (!f) {
    Serial.println("Gagal membuat /config.tmp");
    return false;
  }
  f.print(isi);
  f.close();

  // Periksa: apakah seluruh isi benar-benar tersimpan?
  if (ukuranFile(FILE_SEMENTARA) != isi.length()) {
    Serial.println("Gagal menyimpan: isi /config.tmp tidak lengkap (LittleFS penuh?)");
    LittleFS.remove(FILE_SEMENTARA);
    return false;
  }
  // Ganti /config.txt dengan file baru dalam satu langkah
  if (!LittleFS.rename(FILE_SEMENTARA, FILE_KONFIG)) {
    Serial.println("Gagal mengganti nama /config.tmp menjadi /config.txt");
    return false;
  }
  return true;
}

void tampilkanPengaturan() {
  Serial.println("Pengaturan aktif:");
  Serial.printf("  nama     = %s\n", nama.c_str());
  Serial.printf("  kedip_ms = %lu\n", kedipMs);
  Serial.printf("  led      = %d (%s)\n", ledAktif ? 1 : 0, ledAktif ? "berkedip" : "mati");
}

void tampilkanIsiFile() {
  File f = LittleFS.open(FILE_KONFIG, "r");
  if (!f) {
    Serial.println("/config.txt belum ada");
    return;
  }
  Serial.printf("--- Isi /config.txt (%u byte) ---\n", (unsigned)f.size());
  while (f.available()) {
    Serial.write(f.read());
  }
  f.close();
  Serial.println("--- selesai ---");
}

void prosesPerintah(String baris) {
  baris.trim();
  if (baris.length() == 0) {
    return;
  }
  Serial.print("> ");
  Serial.println(baris);

  String perintah = baris;
  String argumen = "";
  int spasi = baris.indexOf(' ');
  if (spasi >= 0) {
    perintah = baris.substring(0, spasi);
    argumen = baris.substring(spasi + 1);
    argumen.trim();
  }
  perintah.toLowerCase();

  bool butuhFile = (perintah == "isi" || perintah == "simpan" || perintah == "muat");
  if (butuhFile && !fsSiap) {
    Serial.println("LittleFS tidak siap, perintah ini tidak bisa dijalankan.");
  } else if (perintah == "lihat") {
    tampilkanPengaturan();
  } else if (perintah == "isi") {
    tampilkanIsiFile();
  } else if (perintah == "set") {
    int pemisah = argumen.indexOf(' ');
    if (pemisah < 0) {
      Serial.println("Format: set <kunci> <nilai>, misalnya: set kedip_ms 200");
    } else if (terapkan(argumen.substring(0, pemisah), argumen.substring(pemisah + 1))) {
      Serial.println("OK (belum disimpan; ketik 'simpan' supaya tetap ada setelah reset)");
    } else {
      Serial.println("Pengaturan tidak diubah");
    }
  } else if (perintah == "simpan") {
    if (simpanKonfigurasi()) {
      Serial.println("OK: tersimpan di /config.txt");
    }
  } else if (perintah == "muat") {
    if (muatKonfigurasi()) {
      Serial.println("OK: dibaca ulang dari /config.txt");
      tampilkanPengaturan();
    } else {
      Serial.println("/config.txt belum ada");
    }
  } else if (perintah == "bawaan") {
    aturBawaan();
    Serial.println("OK: kembali ke nilai bawaan (belum disimpan)");
    tampilkanPengaturan();
  } else {
    Serial.println("Perintah tidak dikenal. Pilihan: lihat, isi, set, simpan, muat, bawaan");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  penampung.reserve(PANJANG_MAKS);
  Serial.println();
  Serial.println("BAB 40 - Program 4: File Konfigurasi");

  fsSiap = LittleFS.begin(true);
  if (!fsSiap) {
    Serial.println("LittleFS gagal di-mount! Memakai nilai bawaan.");
  } else if (muatKonfigurasi()) {
    Serial.println("Pengaturan dibaca dari /config.txt");
  } else {
    Serial.println("/config.txt belum ada, dibuat dari nilai bawaan");
    simpanKonfigurasi();
  }
  tampilkanPengaturan();
  Serial.printf("Halo, saya %s!\n", nama.c_str());
  Serial.println("Perintah: lihat, isi, set <kunci> <nilai>, simpan, muat, bawaan");
}

void loop() {
  unsigned long sekarang = millis();

  // LED biru berkedip tanpa delay(), mengikuti pengaturan
  if (ledAktif) {
    if (sekarang - waktuKedipLalu >= kedipMs) {
      waktuKedipLalu = sekarang;
      ledNyala = !ledNyala;
      digitalWrite(LED_BIRU, ledNyala ? HIGH : LOW);
    }
  } else if (ledNyala) {
    ledNyala = false;
    digitalWrite(LED_BIRU, LOW);
  }

  // Perintah dari Serial Monitor, tanpa menunggu (pola BAB 16)
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      prosesPerintah(penampung);
      penampung = "";
    } else if (penampung.length() < PANJANG_MAKS) {
      penampung += c;
    }
  }
}
