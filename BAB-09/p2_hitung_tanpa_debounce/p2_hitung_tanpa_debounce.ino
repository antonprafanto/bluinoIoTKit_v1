/*
 * BAB 9 - Program 2: Penghitung Tanpa Debounce
 * Menghitung berapa kali tombol S1 ditekan, sengaja tanpa debounce,
 * supaya efek bouncing terlihat (hitungan kadang melompat).
 *
 * Rangkaian: Tombol S1 (pin kuning ke-3) → IO15
 */

#define TOMBOL_S1 15

int statusSebelumnya = LOW;  // hasil digitalRead() pada putaran loop() sebelumnya
int jumlahTekan = 0;         // berapa kali tombol terhitung ditekan

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);

  Serial.println("BAB 9 - Program 2: Penghitung Tanpa Debounce");
  Serial.println("Tekan S1 satu kali, lalu lihat angkanya...");
}

void loop() {
  int status = digitalRead(TOMBOL_S1);

  // Tepi naik: sebelumnya LOW, sekarang HIGH → dihitung satu kali tekan
  if (status == HIGH && statusSebelumnya == LOW) {
    jumlahTekan++;
    Serial.print("Tombol ditekan! Jumlah = ");
    Serial.println(jumlahTekan);
  }

  statusSebelumnya = status;  // simpan untuk putaran berikutnya
}
