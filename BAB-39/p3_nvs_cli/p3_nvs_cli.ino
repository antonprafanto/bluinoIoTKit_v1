/*
 * BAB 39 - Program 3: Pengaturan Lewat Serial Monitor
 * Melihat, mengubah, dan menghapus pengaturan di NVS dengan perintah teks.
 *
 * Ketik perintah di Serial Monitor (line ending: New Line):
 *   lihat                  → tampilkan semua pengaturan
 *   atur nama <teks>       → nama alat, 1-20 karakter
 *   atur ambang <angka>    → ambang suhu alarm 0-60 (°C), desimal pakai titik
 *   atur interval <detik>  → interval laporan 1-3600 detik
 *   atur buzzer on|off     → buzzer alarm aktif atau tidak
 *   hapus <pengaturan>     → hapus satu pengaturan (kembali ke nilai bawaan)
 *   reset                  → hapus semua pengaturan program ini
 *   info                   → tipe data setiap key dan sisa entri NVS
 *   bantuan                → daftar perintah
 *
 * Rangkaian: tidak perlu, cukup ESP32 dan kabel USB.
 */

#include <Preferences.h>

#define NAMESPACE "pengaturan"   // nama namespace, maksimal 15 karakter

// Nilai bawaan, dipakai kalau pengaturan belum pernah disimpan
const char    *NAMA_BAWAAN     = "Bluino-01";
const float    AMBANG_BAWAAN   = 30.0;
const uint16_t INTERVAL_BAWAAN = 10;
const bool     BUZZER_BAWAAN   = true;

const unsigned int PANJANG_NAMA_MAKS = 20;

Preferences prefs;

// Salinan pengaturan di RAM. Program bekerja dengan variabel-variabel ini.
String   nama;
float    ambang;
uint16_t interval;
bool     buzzer;

// ---------- Pembaca baris tanpa menunggu (sama seperti BAB 22) ----------
const int PANJANG_MAKS = 64;   // termasuk 1 tempat untuk '\0'
char baris[PANJANG_MAKS];
int panjang = 0;

bool bacaBaris() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {      // karakter akhir baris
      if (panjang == 0) continue;      // baris kosong: lewati
      baris[panjang] = '\0';
      panjang = 0;
      return true;
    }
    if (panjang < PANJANG_MAKS - 1) {  // kalau sudah penuh, karakter dibuang
      baris[panjang] = c;
      panjang++;
    }
  }
  return false;
}

// true kalau SELURUH teks adalah angka, misalnya "31.5" atau "-2"
bool apakahAngka(const String &teks) {
  char *sisa;
  strtof(teks.c_str(), &sisa);
  return sisa != teks.c_str() && *sisa == '\0';
}

// true kalau SELURUH teks adalah bilangan bulat, misalnya "60"
bool apakahBilanganBulat(const String &teks) {
  char *sisa;
  strtol(teks.c_str(), &sisa, 10);
  return sisa != teks.c_str() && *sisa == '\0';
}

// ---------- Fungsi-fungsi NVS ----------

void muatPengaturan() {
  prefs.begin(NAMESPACE, false);    // baca-tulis: namespace dibuat kalau belum ada
  nama     = prefs.getString("nama", NAMA_BAWAAN);
  ambang   = prefs.getFloat("ambang", AMBANG_BAWAAN);
  interval = prefs.getUShort("interval", INTERVAL_BAWAAN);
  buzzer   = prefs.getBool("buzzer", BUZZER_BAWAAN);
  prefs.end();
}

// Keterangan "(bawaan)" untuk pengaturan yang belum ada di NVS.
// Dipanggil saat namespace sedang dibuka.
const char *asal(const char *kunci) {
  return prefs.isKey(kunci) ? "" : "  (bawaan)";
}

void tampilkanPengaturan() {
  prefs.begin(NAMESPACE, true);     // hanya-baca: cukup untuk memeriksa isKey()
  Serial.println("Pengaturan saat ini:");
  Serial.printf("  nama     = %s%s\n", nama.c_str(), asal("nama"));
  Serial.printf("  ambang   = %.1f °C%s\n", ambang, asal("ambang"));
  Serial.printf("  interval = %u detik%s\n", (unsigned int)interval, asal("interval"));
  Serial.printf("  buzzer   = %s%s\n", buzzer ? "on" : "off", asal("buzzer"));
  prefs.end();
}

// Mencetak hasil penyimpanan. 'tertulis' adalah nilai kembali fungsi put...()
void laporSimpan(size_t tertulis, const char *kunci, const String &teksNilai) {
  if (tertulis > 0) {
    Serial.printf("OK: %s = %s (tersimpan di NVS)\n", kunci, teksNilai.c_str());
  } else {
    Serial.println("Gagal menyimpan ke NVS!");
  }
}

void aturPengaturan(String kunci, String nilai) {
  prefs.begin(NAMESPACE, false);

  if (kunci == "nama") {
    if (nilai.length() < 1 || nilai.length() > PANJANG_NAMA_MAKS) {
      Serial.printf("Nama harus 1-%u karakter.\n", PANJANG_NAMA_MAKS);
    } else if (nilai == nama) {
      Serial.println("Nilainya sama dengan nilai sekarang, tidak perlu ditulis.");
    } else {
      size_t n = prefs.putString("nama", nilai);
      if (n > 0) nama = nilai;
      laporSimpan(n, "nama", nama);
    }

  } else if (kunci == "ambang") {
    float baru = nilai.toFloat();
    if (!apakahAngka(nilai) || baru < 0 || baru > 60) {
      Serial.println("Ambang harus angka 0-60, contoh: atur ambang 31.5");
    } else if (baru == ambang) {
      Serial.println("Nilainya sama dengan nilai sekarang, tidak perlu ditulis.");
    } else {
      size_t n = prefs.putFloat("ambang", baru);
      if (n > 0) ambang = baru;
      laporSimpan(n, "ambang", String(ambang, 1));
    }

  } else if (kunci == "interval") {
    long baru = nilai.toInt();
    if (!apakahBilanganBulat(nilai) || baru < 1 || baru > 3600) {
      Serial.println("Interval harus bilangan bulat 1-3600, contoh: atur interval 60");
    } else if (baru == interval) {
      Serial.println("Nilainya sama dengan nilai sekarang, tidak perlu ditulis.");
    } else {
      size_t n = prefs.putUShort("interval", (uint16_t)baru);
      if (n > 0) interval = (uint16_t)baru;
      laporSimpan(n, "interval", String((unsigned int)interval));
    }

  } else if (kunci == "buzzer") {
    nilai.toLowerCase();
    if (nilai != "on" && nilai != "off") {
      Serial.println("Pilihan buzzer: on atau off, contoh: atur buzzer off");
    } else {
      bool baru = (nilai == "on");
      if (baru == buzzer) {
        Serial.println("Nilainya sama dengan nilai sekarang, tidak perlu ditulis.");
      } else {
        size_t n = prefs.putBool("buzzer", baru);
        if (n > 0) buzzer = baru;
        laporSimpan(n, "buzzer", buzzer ? "on" : "off");
      }
    }

  } else {
    Serial.println("Pengaturan tidak dikenal. Pilihan: nama, ambang, interval, buzzer");
  }

  prefs.end();
}

bool kunciDikenal(const String &kunci) {
  return kunci == "nama" || kunci == "ambang" || kunci == "interval" || kunci == "buzzer";
}

void hapusPengaturan(String kunci) {
  kunci.toLowerCase();
  if (!kunciDikenal(kunci)) {
    Serial.println("Pengaturan tidak dikenal. Pilihan: nama, ambang, interval, buzzer");
    return;
  }
  prefs.begin(NAMESPACE, false);
  if (!prefs.isKey(kunci.c_str())) {
    Serial.printf("%s belum pernah disimpan, sudah memakai nilai bawaan.\n", kunci.c_str());
  } else if (prefs.remove(kunci.c_str())) {
    Serial.printf("OK: %s dihapus dari NVS, kembali ke nilai bawaan.\n", kunci.c_str());
  } else {
    Serial.println("Gagal menghapus!");
  }
  prefs.end();
  muatPengaturan();   // muat ulang supaya nilai di RAM ikut kembali ke bawaan
}

void resetPengaturan() {
  prefs.begin(NAMESPACE, false);
  bool berhasil = prefs.clear();    // hapus semua key di namespace "pengaturan"
  prefs.end();
  muatPengaturan();
  if (berhasil) {
    Serial.println("OK: semua pengaturan dihapus, kembali ke nilai bawaan.");
  } else {
    Serial.println("Gagal menghapus!");
  }
}

// Mengubah hasil getType() menjadi teks yang mudah dibaca
const char *namaTipe(PreferenceType tipe) {
  switch (tipe) {
    case PT_U8:      return "U8   (putUChar / putBool)";
    case PT_U16:     return "U16  (putUShort)";
    case PT_U32:     return "U32  (putUInt)";
    case PT_STR:     return "STR  (putString)";
    case PT_BLOB:    return "BLOB (putFloat / putBytes)";
    case PT_INVALID: return "belum ada";
    default:         return "tipe lain";
  }
}

void tampilkanInfo() {
  const char *daftarKunci[] = {"nama", "ambang", "interval", "buzzer"};
  prefs.begin(NAMESPACE, true);
  Serial.printf("Isi namespace \"%s\":\n", NAMESPACE);
  for (int i = 0; i < 4; i++) {
    Serial.printf("  %-8s : %s\n", daftarKunci[i], namaTipe(prefs.getType(daftarKunci[i])));
  }
  if (prefs.isKey("nama")) {
    Serial.printf("  Panjang teks nama: %u byte (termasuk penutup)\n",
                  (unsigned int)prefs.getStringLength("nama"));
  }
  Serial.printf("Entri kosong di seluruh partisi NVS: %u\n", (unsigned int)prefs.freeEntries());
  prefs.end();
}

void tampilkanBantuan() {
  Serial.println("Daftar perintah:");
  Serial.println("  lihat                  tampilkan semua pengaturan");
  Serial.println("  atur nama <teks>       1-20 karakter, contoh: atur nama Lab IoT 1");
  Serial.println("  atur ambang <angka>    0-60, contoh: atur ambang 31.5");
  Serial.println("  atur interval <detik>  1-3600, contoh: atur interval 60");
  Serial.println("  atur buzzer on|off     contoh: atur buzzer off");
  Serial.println("  hapus <pengaturan>     contoh: hapus ambang");
  Serial.println("  reset                  hapus semua pengaturan");
  Serial.println("  info                   tipe data dan sisa ruang NVS");
  Serial.println("  bantuan                tampilkan daftar ini");
}

void prosesPerintah(const char *teks) {
  String masukan = teks;
  masukan.trim();                      // buang spasi di awal dan akhir
  if (masukan.length() == 0) return;

  Serial.print("> ");
  Serial.println(masukan);

  // Pisahkan kata pertama (perintah) dari sisanya (argumen), seperti BAB 22
  String perintah = masukan;
  String argumen = "";
  int posisiSpasi = masukan.indexOf(' ');
  if (posisiSpasi >= 0) {
    perintah = masukan.substring(0, posisiSpasi);
    argumen = masukan.substring(posisiSpasi + 1);
    argumen.trim();
  }
  perintah.toLowerCase();              // "LIHAT" dianggap sama dengan "lihat"

  if (perintah == "lihat") {
    tampilkanPengaturan();
  } else if (perintah == "atur") {
    // Argumen dipecah lagi: nama pengaturan, lalu nilainya
    int spasiKedua = argumen.indexOf(' ');
    if (spasiKedua < 0) {
      Serial.println("Format: atur <pengaturan> <nilai>, contoh: atur ambang 31.5");
      return;
    }
    String kunci = argumen.substring(0, spasiKedua);
    String nilai = argumen.substring(spasiKedua + 1);
    kunci.toLowerCase();
    nilai.trim();
    aturPengaturan(kunci, nilai);
  } else if (perintah == "hapus") {
    hapusPengaturan(argumen);
  } else if (perintah == "reset") {
    resetPengaturan();
  } else if (perintah == "info") {
    tampilkanInfo();
  } else if (perintah == "bantuan") {
    tampilkanBantuan();
  } else {
    Serial.println("Perintah tidak dikenal. Ketik: bantuan");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("BAB 39 - Program 3: Pengaturan Lewat Serial Monitor");
  muatPengaturan();
  tampilkanPengaturan();
  Serial.println("Ketik bantuan untuk melihat daftar perintah.");
}

void loop() {
  if (bacaBaris()) {          // tidak pernah menunggu
    prosesPerintah(baris);
  }
  // Di sini program bebas mengerjakan tugas lain, misalnya membaca sensor
}
