/*
 * BAB 31 - Program 3: Lampu Otomatis
 * LED kuning menyala saat sensor PIR mendeteksi gerakan dan padam
 * setelah tidak ada gerakan selama LAMA_NYALA. Selama MASA_STABIL
 * setelah ESP32 menyala, sensor diabaikan dan LED berkedip pelan.
 *
 * Rangkaian:
 *   PIR (header hijau, pin ke-5 dari atas) → IO33
 *   LED4 (kuning, header LED)              → IO16
 */

#define PIN_PIR 33
#define LAMPU   16  // LED kuning sebagai "lampu"

const unsigned long MASA_STABIL = 60000;  // ms: abaikan PIR selama 1 menit pertama
const unsigned long LAMA_NYALA  = 10000;  // ms: lampu padam setelah sepi selama ini

bool sudahStabil = false;
bool lampuNyala = false;
int statusSebelumnya = LOW;
unsigned long waktuGerakanTerakhir = 0;  // kapan PIR terakhir terbaca HIGH
int sisaDetikTercetak = -1;              // angka hitung mundur terakhir yang dicetak

// Mencetak waktu sejak ESP32 mulai, misalnya "[detik 75.3] "
void cetakWaktu(unsigned long ms) {
  Serial.print("[detik ");
  Serial.print(ms / 1000.0, 1);
  Serial.print("] ");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  pinMode(LAMPU, OUTPUT);
  digitalWrite(LAMPU, LOW);

  Serial.println("BAB 31 - Program 3: Lampu Otomatis");
  Serial.println("Menunggu sensor PIR stabil. Jangan bergerak di depan sensor.");
}

void loop() {
  unsigned long sekarang = millis();

  // ---- Tahap 1: masa stabil (millis() mulai dari 0 saat ESP32 menyala) ----
  if (!sudahStabil) {
    if (sekarang >= MASA_STABIL) {
      sudahStabil = true;
      digitalWrite(LAMPU, LOW);
      Serial.println("Lampu otomatis AKTIF.");
    } else {
      digitalWrite(LAMPU, (sekarang / 500) % 2 == 0 ? HIGH : LOW);  // berkedip pelan
      int sisaDetik = (MASA_STABIL - sekarang + 999) / 1000;      // dibulatkan ke atas
      if (sisaDetik != sisaDetikTercetak && sisaDetik % 10 == 0) {
        sisaDetikTercetak = sisaDetik;
        Serial.print("  Siap dalam ");
        Serial.print(sisaDetik);
        Serial.println(" detik");
      }
    }
    return;  // selama masa stabil, bagian di bawah dilewati
  }

  // ---- Tahap 2: lampu otomatis ----
  int statusPir = digitalRead(PIN_PIR);
  bool gerakanBaru = (statusSebelumnya == LOW && statusPir == HIGH);  // tepi naik
  statusSebelumnya = statusPir;

  if (statusPir == HIGH) {
    waktuGerakanTerakhir = sekarang;  // masih ada gerakan: hitungan sepi diulang

    if (!lampuNyala) {
      lampuNyala = true;
      digitalWrite(LAMPU, HIGH);
      cetakWaktu(sekarang);
      Serial.println("Ada gerakan -> lampu NYALA");
    } else if (gerakanBaru) {
      cetakWaktu(sekarang);
      Serial.println("Ada gerakan lagi -> waktu nyala diperpanjang");
    }
  }

  if (lampuNyala && (sekarang - waktuGerakanTerakhir >= LAMA_NYALA)) {
    lampuNyala = false;
    digitalWrite(LAMPU, LOW);
    cetakWaktu(sekarang);
    Serial.println("Sepi selama 10 detik -> lampu MATI");
  }
}
