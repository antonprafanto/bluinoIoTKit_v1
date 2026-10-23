// BAB 6 — Program 1: Uji Peta Pin (LED dan Tombol)
// Memeriksa kabel jumper LED dan tombol sesuai Peta Pin Standar kit Bluino.
// Rangkaian:
//   LED1 (merah) -> IO4    LED4 (kuning) -> IO16
//   LED2 (hijau) -> IO17   LED3 (biru)   -> IO2
//   SW1 (pin kuning ke-3) -> IO15, SW2 (pin kuning ke-4) -> IO25

#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17
#define LED_BIRU   2    // IO2 juga menyalakan LED biru bawaan board
#define TOMBOL_S1  15   // ditekan = HIGH (pull-down 10 kΩ sudah ada di kit)
#define TOMBOL_S2  25   // ditekan = HIGH

bool s1Sebelumnya = false;  // keadaan tombol pada putaran loop() sebelumnya
bool s2Sebelumnya = false;

// Menyalakan satu LED selama 0,7 detik sambil melaporkannya ke Serial Monitor
void ujiLed(int pin, const char *warna) {
  Serial.print("LED ");
  Serial.print(warna);
  Serial.print(" (IO");
  Serial.print(pin);
  Serial.println(") menyala");
  digitalWrite(pin, HIGH);
  delay(700);
  digitalWrite(pin, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung

  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_BIRU, OUTPUT);
  pinMode(TOMBOL_S1, INPUT);  // INPUT biasa, karena pull-down sudah ada di PCB
  pinMode(TOMBOL_S2, INPUT);

  Serial.println("=== Uji Peta Pin Kit Bluino ===");
  ujiLed(LED_MERAH, "merah");
  ujiLed(LED_KUNING, "kuning");
  ujiLed(LED_HIJAU, "hijau");
  ujiLed(LED_BIRU, "biru");
  Serial.println("Uji LED selesai. Tekan tombol S1 atau S2.");
}

void loop() {
  bool s1 = (digitalRead(TOMBOL_S1) == HIGH);
  bool s2 = (digitalRead(TOMBOL_S2) == HIGH);

  // S1 menyalakan LED merah dan kuning, S2 menyalakan LED hijau dan biru
  digitalWrite(LED_MERAH, s1 ? HIGH : LOW);
  digitalWrite(LED_KUNING, s1 ? HIGH : LOW);
  digitalWrite(LED_HIJAU, s2 ? HIGH : LOW);
  digitalWrite(LED_BIRU, s2 ? HIGH : LOW);

  // Laporkan hanya saat keadaan tombol berubah
  if (s1 != s1Sebelumnya) {
    Serial.println(s1 ? "S1 (IO15) ditekan" : "S1 (IO15) dilepas");
    s1Sebelumnya = s1;
  }
  if (s2 != s2Sebelumnya) {
    Serial.println(s2 ? "S2 (IO25) ditekan" : "S2 (IO25) dilepas");
    s2Sebelumnya = s2;
  }

  delay(20);  // jeda singkat supaya getaran kontak tombol tidak terbaca berkali-kali
}
