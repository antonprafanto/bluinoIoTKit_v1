/*
 * BAB 22 - Program 2: Perintah Teks untuk LED dan Buzzer
 * Ketik perintah di Serial Monitor (line ending: New Line):
 *   nyala <warna>  → menyalakan LED (warna: merah, hijau, biru, kuning, semua)
 *   mati <warna>   → mematikan LED
 *   bip <jumlah>   → buzzer berbunyi 1-10 kali
 *   status         → menampilkan keadaan semua LED
 *   bantuan        → menampilkan daftar perintah
 *
 * Rangkaian (header kit → pin ESP32):
 *   LED1 (merah) → IO4    LED2 (hijau)  → IO17
 *   LED3 (biru)  → IO2    LED4 (kuning) → IO16
 *   BUZZ         → IO13
 */

#define LED_MERAH  4
#define LED_HIJAU  17
#define LED_BIRU   2
#define LED_KUNING 16
#define BUZZER_PIN 13

const int JUMLAH_LED = 4;
const int PIN_LED[JUMLAH_LED] = {LED_MERAH, LED_HIJAU, LED_BIRU, LED_KUNING};
const char *NAMA_LED[JUMLAH_LED] = {"merah", "hijau", "biru", "kuning"};

void tampilkanBantuan() {
  Serial.println("Daftar perintah:");
  Serial.println("  nyala <warna>  warna: merah, hijau, biru, kuning, semua");
  Serial.println("  mati <warna>");
  Serial.println("  bip <jumlah>   jumlah: 1-10");
  Serial.println("  status");
  Serial.println("  bantuan");
}

// Menyalakan/mematikan LED sesuai nama warnanya.
// Hasilnya false kalau nama warna tidak dikenal.
bool aturLed(String warna, bool nyala) {
  bool dikenal = false;
  for (int i = 0; i < JUMLAH_LED; i++) {
    if (warna == "semua" || warna == NAMA_LED[i]) {
      digitalWrite(PIN_LED[i], nyala ? HIGH : LOW);
      dikenal = true;
    }
  }
  return dikenal;
}

// Buzzer berbunyi pendek sebanyak 'jumlah' kali (paling lama 2 detik)
void bip(int jumlah) {
  for (int i = 0; i < jumlah; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

void prosesPerintah(String baris) {
  baris.trim();          // buang spasi serta \r dan \n di awal dan akhir
  baris.toLowerCase();   // "NYALA Merah" dianggap sama dengan "nyala merah"
  if (baris.length() == 0) return;   // baris kosong: abaikan

  Serial.print("> ");
  Serial.println(baris);

  // Pisahkan kata pertama (perintah) dari sisanya (argumen)
  String perintah = baris;
  String argumen = "";
  int posisiSpasi = baris.indexOf(' ');
  if (posisiSpasi >= 0) {
    perintah = baris.substring(0, posisiSpasi);
    argumen = baris.substring(posisiSpasi + 1);
    argumen.trim();      // "nyala    merah" tetap terbaca
  }

  if (perintah == "nyala" || perintah == "mati") {
    bool nyala = (perintah == "nyala");
    if (aturLed(argumen, nyala)) {
      Serial.printf("OK: LED %s %s\n", argumen.c_str(), nyala ? "NYALA" : "MATI");
    } else {
      Serial.println("Warna tidak dikenal. Pilihan: merah, hijau, biru, kuning, semua");
    }
  } else if (perintah == "bip") {
    int jumlah = argumen.toInt();   // "3" → 3; teks yang bukan angka → 0
    if (jumlah < 1 || jumlah > 10) {
      Serial.println("Jumlah bip harus 1-10, contoh: bip 3");
    } else {
      bip(jumlah);
      Serial.printf("OK: bip %d kali\n", jumlah);
    }
  } else if (perintah == "status") {
    for (int i = 0; i < JUMLAH_LED; i++) {
      Serial.printf("  LED %-6s: %s\n", NAMA_LED[i], digitalRead(PIN_LED[i]) ? "NYALA" : "MATI");
    }
  } else if (perintah == "bantuan") {
    tampilkanBantuan();
  } else {
    Serial.println("Perintah tidak dikenal. Ketik: bantuan");
  }
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(PIN_LED[i], OUTPUT);
    digitalWrite(PIN_LED[i], LOW);
  }
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("BAB 22 - Program 2: Perintah Teks untuk LED dan Buzzer");
  tampilkanBantuan();
}

void loop() {
  if (Serial.available() > 0) {
    String baris = Serial.readStringUntil('\n');  // baca sampai karakter '\n'
    prosesPerintah(baris);
  }
}
