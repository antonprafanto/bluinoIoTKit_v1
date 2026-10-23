/*
 * BAB 21 - Program 3: Crash yang Disengaja
 * Ketik angka 1-5 di Serial Monitor untuk memicu satu jenis crash,
 * lalu pelajari pesan yang muncul sebelum ESP32 restart.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino (tidak perlu merangkai)
 * Alat uji : Serial Monitor, baud 115200, line ending "New Line"
 */

int *volatile alamatKosong = NULL;    // pointer yang menunjuk alamat 0 (NULL)
volatile int pembagi = 0;             // volatile: compiler tidak boleh menebak nilainya
volatile int batasRekursi = 1000000;  // jauh melebihi kemampuan stack

void tampilkanMenu() {
  Serial.println();
  Serial.println("Ketik angka lalu tekan Enter:");
  Serial.println("  1 = membaca alamat NULL     (LoadProhibited)");
  Serial.println("  2 = menulis ke alamat NULL  (StoreProhibited)");
  Serial.println("  3 = membagi dengan nol      (IntegerDivideByZero)");
  Serial.println("  4 = rekursi tanpa akhir     (stack overflow)");
  Serial.println("  5 = memanggil abort()");
}

void bacaNull() {
  int nilai = *alamatKosong;  // membaca isi alamat 0 -> crash
  Serial.println(nilai);      // tidak pernah sampai di sini
}

void tulisNull() {
  *alamatKosong = 42;  // menulis ke alamat 0 -> crash
}

void bagiNol() {
  int hasil = 100 / pembagi;  // pembagian bilangan bulat dengan 0 -> crash
  Serial.println(hasil);
}

int rekursi(int level) {
  volatile uint8_t catatan[200];  // setiap level memakai sekitar 200 byte stack
  catatan[0] = level;
  if (level >= batasRekursi) {
    return 0;
  }
  return rekursi(level + 1) + catatan[0];  // terus memanggil diri sendiri
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BAB 21 - Program 3: Crash yang Disengaja");
  tampilkanMenu();
}

void loop() {
  if (Serial.available() == 0) {
    return;  // belum ada ketikan
  }

  char pilihan = Serial.read();
  if (pilihan == '\n' || pilihan == '\r') {
    return;  // abaikan karakter Enter
  }

  Serial.print("Pilihan ");
  Serial.print(pilihan);
  Serial.println(": ESP32 akan crash...");
  Serial.flush();  // tunggu teks terkirim sebelum crash

  switch (pilihan) {
    case '1': bacaNull(); break;
    case '2': tulisNull(); break;
    case '3': bagiNol(); break;
    case '4': Serial.println(rekursi(0)); break;
    case '5': abort(); break;
    default:
      Serial.println("Pilihan tidak dikenal.");
      tampilkanMenu();
      break;
  }
}
