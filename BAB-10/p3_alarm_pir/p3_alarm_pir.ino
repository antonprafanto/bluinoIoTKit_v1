/*
 * BAB 10 - Program 3: Alarm Gerakan dengan PIR dan Dua Tombol
 * S1 = aktifkan alarm (dibaca dengan polling)
 * S2 = matikan alarm (interupsi), PIR = deteksi gerakan (interupsi)
 * Saat alarm aktif, gerakan di depan sensor PIR membunyikan buzzer
 * terus-menerus sampai alarm dimatikan dengan S2.
 *
 * Rangkaian:
 *   SW1 (tombol S1, header kuning) → IO15
 *   SW2 (tombol S2, header kuning) → IO25
 *   PIR (header hijau)             → IO33
 *   LED1 (merah, header LED)       → IO4   (indikator alarm aktif)
 *   BUZZ                           → IO13
 */

#define TOMBOL_S1  15
#define TOMBOL_S2  25
#define PIR_PIN    33
#define LED_MERAH  4
#define BUZZER_PIN 13

// Variabel bersama ISR <-> loop(): wajib volatile
volatile bool alarmAktif = false;  // diisi true oleh loop() (S1), false oleh ISR S2
volatile bool adaGerakan = false;  // bendera dari ISR sensor PIR

// Kunci bagian kritis untuk bendera adaGerakan
portMUX_TYPE kunciGerakan = portMUX_INITIALIZER_UNLOCKED;

bool alarmAktifSebelumnya = false;  // hanya dipakai loop()

// ISR tombol S2: matikan alarm.
// Pantulan tidak masalah: mengisi false berkali-kali hasilnya tetap false.
void IRAM_ATTR isrTombolS2() {
  alarmAktif = false;
}

// ISR sensor PIR: output PIR berubah LOW → HIGH = gerakan mulai terdeteksi
void IRAM_ATTR isrPir() {
  portENTER_CRITICAL_ISR(&kunciGerakan);
  adaGerakan = true;
  portEXIT_CRITICAL_ISR(&kunciGerakan);
}

// Bunyikan buzzer dan kedipkan LED terus-menerus sampai alarm dimatikan (S2)
void bunyikanAlarm() {
  while (alarmAktif) {  // alarmAktif diubah oleh ISR S2 dari "luar" perulangan ini
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_MERAH, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_MERAH, HIGH);
    delay(150);
  }
  digitalWrite(BUZZER_PIN, LOW);  // pastikan buzzer diam
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);  // pull-down 10 kΩ sudah ada di kit
  pinMode(TOMBOL_S2, INPUT);
  pinMode(PIR_PIN, INPUT);    // output PIR sudah HIGH/LOW yang jelas
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // S2 memakai interupsi supaya bisa menghentikan alarm kapan saja.
  // Interupsi PIR baru dipasang saat alarm aktif (lihat loop()).
  attachInterrupt(digitalPinToInterrupt(TOMBOL_S2), isrTombolS2, RISING);

  Serial.println("BAB 10 - Program 3: Alarm Gerakan dengan PIR dan Dua Tombol");
  Serial.println("S1 = aktifkan alarm, S2 = matikan alarm.");
  Serial.println("Beri PIR waktu sekitar 1 menit setelah dinyalakan supaya stabil.");
}

void loop() {
  // 1. S1 cukup dibaca dengan polling: saat menunggu, loop() berputar sangat cepat
  if (digitalRead(TOMBOL_S1) == HIGH) {
    alarmAktif = true;
  }

  // 2. Status alarm berubah (karena S1 atau S2)? Pasang atau lepas interupsi PIR.
  bool aktifSekarang = alarmAktif;  // salin sekali, lalu pakai salinannya
  if (aktifSekarang != alarmAktifSebelumnya) {
    alarmAktifSebelumnya = aktifSekarang;

    if (aktifSekarang) {
      attachInterrupt(digitalPinToInterrupt(PIR_PIN), isrPir, RISING);
      digitalWrite(LED_MERAH, HIGH);
      Serial.println("Alarm AKTIF: PIR diawasi (LED merah menyala)");
    } else {
      detachInterrupt(digitalPinToInterrupt(PIR_PIN));
      digitalWrite(LED_MERAH, LOW);
      Serial.println("Alarm MATI: PIR tidak diawasi");
    }
  }

  // 3. Ambil bendera gerakan: baca lalu turunkan di dalam satu bagian kritis
  portENTER_CRITICAL(&kunciGerakan);
  bool gerakan = adaGerakan;
  adaGerakan = false;
  portEXIT_CRITICAL(&kunciGerakan);

  // 4. Ada gerakan saat alarm aktif? Bunyikan sampai S2 ditekan.
  if (gerakan && alarmAktif) {
    Serial.println("GERAKAN TERDETEKSI! Alarm berbunyi sampai S2 ditekan...");
    bunyikanAlarm();
    Serial.println("Alarm dihentikan dengan tombol S2");
  }
}
