/*
 * BAB 7 - Program 1: Hello World
 * Menampilkan pesan pembuka sekali (di setup), lalu pesan
 * berhitung setiap 2 detik (di loop) ke Serial Monitor.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino (tidak perlu merangkai)
 * Alat uji : Serial Monitor, baud 115200
 */

int hitungan = 0;  // variabel untuk menyimpan angka hitungan, nilai awalnya 0

void setup() {
  Serial.begin(115200);  // buka komunikasi Serial dengan kecepatan 115200 baud
  delay(1000);           // jeda 1 detik supaya Serial Monitor sempat siap

  Serial.println("=================================");
  Serial.println("  Hello World!");
  Serial.println("  Halo dari ESP32 di kit Bluino");
  Serial.println("=================================");
  Serial.println();      // cetak baris kosong
}

void loop() {
  hitungan = hitungan + 1;  // tambah 1 setiap kali loop() berjalan

  Serial.print("ESP32 sedang berjalan... putaran ke-");
  Serial.println(hitungan);  // cetak angkanya, lalu pindah baris

  delay(2000);  // tunggu 2 detik, lalu loop() diulang dari atas
}
