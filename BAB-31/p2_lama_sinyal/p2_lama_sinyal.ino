/*
 * BAB 31 - Program 2: Mengukur Lama Sinyal Gerakan
 * Mendeteksi tepi naik (gerakan mulai) dan tepi turun (sinyal gerakan
 * berakhir) dari sensor PIR AM312, lalu mencetak lamanya OUT bernilai
 * HIGH dan lamanya OUT bernilai LOW sebelum gerakan berikutnya.
 *
 * Rangkaian: PIR (header hijau, pin ke-5 dari atas) → IO33
 */

#define PIN_PIR 33

int statusSebelumnya = LOW;     // bacaan PIR pada putaran loop() sebelumnya
int jumlahGerakan = 0;          // berapa kali OUT naik ke HIGH
unsigned long waktuNaik = 0;    // kapan OUT terakhir naik ke HIGH (ms)
unsigned long waktuTurun = 0;   // kapan OUT terakhir turun ke LOW (ms)

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);

  Serial.println("BAB 31 - Program 2: Mengukur Lama Sinyal Gerakan");
  Serial.println("Lambaikan tangan di depan kubah lensa PIR, lalu diam.");
}

void loop() {
  int statusSekarang = digitalRead(PIN_PIR);
  unsigned long sekarang = millis();

  // Tepi naik: LOW → HIGH = gerakan mulai terdeteksi
  if (statusSebelumnya == LOW && statusSekarang == HIGH) {
    jumlahGerakan = jumlahGerakan + 1;
    waktuNaik = sekarang;

    Serial.print("Gerakan ke-");
    Serial.print(jumlahGerakan);
    Serial.print(" MULAI pada detik ke-");
    Serial.print(sekarang / 1000.0, 1);
    if (jumlahGerakan > 1) {
      // Lama OUT bernilai LOW sejak sinyal gerakan sebelumnya berakhir
      Serial.print(" (sebelumnya LOW selama ");
      Serial.print((sekarang - waktuTurun) / 1000.0, 1);
      Serial.print(" detik)");
    }
    Serial.println();
  }

  // Tepi turun: HIGH → LOW = sinyal gerakan berakhir
  if (statusSebelumnya == HIGH && statusSekarang == LOW) {
    waktuTurun = sekarang;

    Serial.print("Gerakan ke-");
    Serial.print(jumlahGerakan);
    Serial.print(" SELESAI: OUT HIGH selama ");
    Serial.print((sekarang - waktuNaik) / 1000.0, 1);
    Serial.println(" detik");
  }

  statusSebelumnya = statusSekarang;  // simpan untuk putaran berikutnya
}
