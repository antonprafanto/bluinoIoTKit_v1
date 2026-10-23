/*
 * BAB 18 - Program 1: Mengenal Function Pointer
 * Satu variabel bisa menyimpan fungsi yang berbeda-beda,
 * dan sebuah fungsi bisa menerima fungsi lain sebagai parameter (callback).
 *
 * Rangkaian: tidak perlu (hanya Serial Monitor)
 */

// Nama tipe untuk "fungsi yang menerima satu int dan tidak mengembalikan nilai"
typedef void (*AksiAngka)(int);

void cetakAngka(int x) {
  Serial.print("cetakAngka: ");
  Serial.println(x);
}

void cetakDobel(int x) {
  Serial.print("cetakDobel: ");
  Serial.println(x * 2);
}

void cetakKuadrat(int x) {
  Serial.print("cetakKuadrat: ");
  Serial.println(x * x);
}

// Fungsi ini MENERIMA fungsi lain sebagai parameter.
// "aksi" adalah callback: dipanggil untuk setiap angka 1 sampai n.
void ulangiUntukSetiapAngka(int n, AksiAngka aksi) {
  for (int i = 1; i <= n; i++) {
    aksi(i);  // panggil balik (call back) fungsi yang diserahkan
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);  // beri waktu sebentar supaya Serial Monitor siap
  Serial.println("BAB 18 - Program 1: Mengenal Function Pointer");

  // 1) Satu variabel, isinya bisa diganti-ganti
  AksiAngka aksi = cetakAngka;  // tanpa tanda kurung: menyimpan fungsinya
  aksi(5);                      // dengan tanda kurung: memanggilnya
  aksi = cetakDobel;            // ganti isinya ke fungsi lain
  aksi(5);

  // 2) Fungsi sebagai parameter — inilah callback
  Serial.println("-- ulangiUntukSetiapAngka(3, cetakKuadrat) --");
  ulangiUntukSetiapAngka(3, cetakKuadrat);

  // 3) Array berisi function pointer: daftar aksi
  AksiAngka daftarAksi[] = {cetakAngka, cetakDobel, cetakKuadrat};
  Serial.println("-- semua aksi untuk angka 4 --");
  for (int i = 0; i < 3; i++) {
    daftarAksi[i](4);
  }

  // 4) Pointer kosong (nullptr) harus diperiksa sebelum dipanggil
  AksiAngka kosong = nullptr;
  if (kosong != nullptr) {
    kosong(1);
  } else {
    Serial.println("Pointer kosong: tidak dipanggil (aman)");
  }
}

void loop() {
  // Tidak ada yang diulang. Tekan tombol EN untuk menjalankan ulang program.
}
