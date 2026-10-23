/*
 * BAB 7 - Program 3: Sinyal SOS dengan Fungsi Buatan Sendiri
 * LED biru (IO2) mengedipkan kode Morse SOS ( ... --- ... ) berulang-ulang,
 * sementara Serial Monitor menampilkan titik dan garis yang dikirim.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino
 * Pin      : IO2 = LED biru bawaan board
 * Alat uji : Serial Monitor, baud 115200
 */

#define LED_BIRU 2

const int SATUAN = 200;  // 1 satuan waktu Morse dalam milidetik (makin kecil, makin cepat)

// Fungsi buatan sendiri: mengirim satu "titik" (LED menyala 1 satuan)
void titik() {
  Serial.print(".");
  digitalWrite(LED_BIRU, HIGH);
  delay(SATUAN);
  digitalWrite(LED_BIRU, LOW);
  delay(SATUAN);  // jeda antarsinyal dalam satu huruf = 1 satuan
}

// Fungsi buatan sendiri: mengirim satu "garis" (LED menyala 3 satuan)
void garis() {
  Serial.print("-");
  digitalWrite(LED_BIRU, HIGH);
  delay(3 * SATUAN);
  digitalWrite(LED_BIRU, LOW);
  delay(SATUAN);  // jeda antarsinyal dalam satu huruf = 1 satuan
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);
  Serial.print("Program SOS dimulai! 1 satuan = ");
  Serial.print(SATUAN);
  Serial.println(" ms");
}

void loop() {
  // Huruf S = titik titik titik
  titik(); titik(); titik();
  Serial.print(" ");
  delay(2 * SATUAN);  // jeda antarhuruf: 1 satuan (dari sinyal terakhir) + 2 satuan = 3 satuan

  // Huruf O = garis garis garis
  garis(); garis(); garis();
  Serial.print(" ");
  delay(2 * SATUAN);  // jeda antarhuruf = 3 satuan

  // Huruf S lagi
  titik(); titik(); titik();
  Serial.println();   // satu kata SOS selesai, pindah baris
  delay(6 * SATUAN);  // jeda antarkata: 1 + 6 = 7 satuan
}
