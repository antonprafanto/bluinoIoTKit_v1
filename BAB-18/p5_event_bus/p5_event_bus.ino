/*
 * BAB 18 - Program 5: Event Bus dengan Antrean Event
 * Sumber event (tombol S1 dan LDR) hanya MENGIRIM event ke antrean.
 * Event bus membagikan setiap event ke semua pendengar yang
 * mendaftar untuk jenis event tersebut.
 *
 * Rangkaian:
 *   Tombol S1 (SW1, pin kuning ke-3) → IO15
 *   LDR (pin kuning ke-1)            → IO35
 *   LED1 (merah)                     → IO4
 *   BUZZ (buzzer aktif)              → IO13
 */

#include <functional>

#define TOMBOL_S1  15
#define LDR_PIN    35
#define LED_MERAH  4
#define BUZZER_PIN 13

// LDR di kit: makin gelap, nilai ADC makin besar. Sesuaikan dengan ruanganmu.
const int AMBANG_GELAP = 3000;   // di atas nilai ini dianggap gelap
const int AMBANG_TERANG = 2500;  // di bawah nilai ini dianggap terang lagi

// ============================================================
// 1. TIPE DATA: jenis event, data event, dan bentuk pendengar
//    (semua tipe ditulis SEBELUM fungsi pertama — lihat catatan)
// ============================================================
enum JenisEvent {
  EVENT_TOMBOL_DITEKAN,
  EVENT_JADI_GELAP,
  EVENT_JADI_TERANG
};

struct Event {
  JenisEvent jenis;
  int nilai;  // data tambahan: jumlah tekan, atau nilai ADC LDR
};

// Pendengar = fungsi apa pun yang menerima sebuah Event
using Pendengar = std::function<void(const Event &)>;

struct Langganan {
  JenisEvent jenis;  // jenis event yang didengarkan
  Pendengar fungsi;  // fungsi yang dipanggil saat event itu terjadi
};

const char *namaEvent(JenisEvent jenis) {
  switch (jenis) {
    case EVENT_TOMBOL_DITEKAN: return "TOMBOL_DITEKAN";
    case EVENT_JADI_GELAP:     return "JADI_GELAP";
    case EVENT_JADI_TERANG:    return "JADI_TERANG";
  }
  return "?";
}

// ============================================================
// 2. ANTREAN EVENT (FIFO, ring buffer)
// ============================================================
const int UKURAN_ANTREAN = 8;
Event antrean[UKURAN_ANTREAN];
int posisiAmbil = 0;  // indeks event yang akan diproses berikutnya
int posisiTaruh = 0;  // indeks tempat event baru akan ditaruh
int jumlahAntre = 0;  // banyaknya event yang sedang menunggu

// Dipanggil oleh SUMBER event: hanya menaruh event di antrean, jadi cepat
void kirimEvent(JenisEvent jenis, int nilai) {
  if (jumlahAntre == UKURAN_ANTREAN) {
    Serial.println("[BUS] Antrean penuh, event dibuang!");
    return;
  }
  antrean[posisiTaruh] = {jenis, nilai};
  posisiTaruh = (posisiTaruh + 1) % UKURAN_ANTREAN;  // setelah indeks 7 kembali ke 0
  jumlahAntre++;
}

// ============================================================
// 3. EVENT BUS: daftar langganan (jenis event → fungsi pendengar)
// ============================================================
const int MAKS_LANGGANAN = 10;
Langganan daftarLangganan[MAKS_LANGGANAN];
int jumlahLangganan = 0;

// Mendaftarkan sebuah pendengar untuk satu jenis event
void dengarkan(JenisEvent jenis, Pendengar fungsi) {
  if (jumlahLangganan == MAKS_LANGGANAN) {
    Serial.println("[BUS] Daftar langganan penuh!");
    return;
  }
  daftarLangganan[jumlahLangganan] = {jenis, fungsi};
  jumlahLangganan++;
}

// Dipanggil di loop(): ambil SATU event, bagikan ke semua pendengarnya
void prosesEvent() {
  if (jumlahAntre == 0) return;
  Event e = antrean[posisiAmbil];
  posisiAmbil = (posisiAmbil + 1) % UKURAN_ANTREAN;
  jumlahAntre--;

  for (int i = 0; i < jumlahLangganan; i++) {
    if (daftarLangganan[i].jenis == e.jenis && daftarLangganan[i].fungsi) {
      daftarLangganan[i].fungsi(e);
    }
  }
}

// ============================================================
// 4. SUMBER EVENT — tidak tahu siapa yang mendengarkan
// ============================================================
// 4a. Tombol S1 (debounce seperti BAB 9)
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;
int jumlahTekan = 0;

void periksaTombol() {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = millis();
    bacaanSebelumnya = bacaan;
  }
  if (millis() - waktuBerubah > 50 && bacaan != statusStabil) {
    statusStabil = bacaan;
    if (statusStabil == HIGH) {
      jumlahTekan++;
      kirimEvent(EVENT_TOMBOL_DITEKAN, jumlahTekan);
    }
  }
}

// 4b. LDR: kirim event hanya saat keadaan berganti (terang <-> gelap)
bool sedangGelap = false;
unsigned long waktuBacaLdr = 0;

void periksaCahaya() {
  if (millis() - waktuBacaLdr < 200) return;  // baca setiap 200 ms
  waktuBacaLdr = millis();

  int nilai = analogRead(LDR_PIN);
  if (!sedangGelap && nilai > AMBANG_GELAP) {
    sedangGelap = true;
    kirimEvent(EVENT_JADI_GELAP, nilai);
  } else if (sedangGelap && nilai < AMBANG_TERANG) {
    sedangGelap = false;
    kirimEvent(EVENT_JADI_TERANG, nilai);
  }
}

// ============================================================
// 5. ALAT BANTU UNTUK PENDENGAR: LED dan buzzer non-blocking
// ============================================================
bool ledNyala = false;

void aturLed(bool nyala) {
  ledNyala = nyala;
  digitalWrite(LED_MERAH, ledNyala ? HIGH : LOW);
}

const unsigned long LAMA_BUNYI = 80;  // ms
int sisaBip = 0;
bool buzzerBunyi = false;
unsigned long waktuBuzzer = 0;

void mulaiBip(int kali) {
  sisaBip = kali;
  buzzerBunyi = true;
  digitalWrite(BUZZER_PIN, HIGH);
  waktuBuzzer = millis();
}

void urusBuzzer() {
  if (sisaBip == 0) return;
  if (millis() - waktuBuzzer < LAMA_BUNYI) return;
  waktuBuzzer = millis();
  if (buzzerBunyi) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerBunyi = false;
    sisaBip--;
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerBunyi = true;
  }
}

// Pendengar yang ditulis sebagai fungsi biasa
void pencatat(const Event &e) {
  Serial.print("[LOG] ");
  Serial.print(namaEvent(e.jenis));
  Serial.print(" (nilai: ");
  Serial.print(e.nilai);
  Serial.println(")");
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  aturLed(false);
  digitalWrite(BUZZER_PIN, LOW);

  // ---- Daftarkan pendengar (urutan daftar = urutan dipanggil) ----
  // Pencatat mendengarkan SEMUA jenis event
  dengarkan(EVENT_TOMBOL_DITEKAN, pencatat);
  dengarkan(EVENT_JADI_GELAP, pencatat);
  dengarkan(EVENT_JADI_TERANG, pencatat);

  // LED merah: toggle saat tombol ditekan, nyala saat gelap, mati saat terang
  dengarkan(EVENT_TOMBOL_DITEKAN, [](const Event &e) {
    aturLed(!ledNyala);
    Serial.println(ledNyala ? "[LED] toggle -> NYALA" : "[LED] toggle -> MATI");
  });
  dengarkan(EVENT_JADI_GELAP, [](const Event &e) {
    aturLed(true);
    Serial.println("[LED] gelap -> NYALA");
  });
  dengarkan(EVENT_JADI_TERANG, [](const Event &e) {
    aturLed(false);
    Serial.println("[LED] terang -> MATI");
  });

  // Buzzer: 1 bip saat tombol ditekan, 2 bip saat menjadi gelap
  dengarkan(EVENT_TOMBOL_DITEKAN, [](const Event &e) {
    mulaiBip(1);
    Serial.println("[BUZZER] bip 1 kali");
  });
  dengarkan(EVENT_JADI_GELAP, [](const Event &e) {
    mulaiBip(2);
    Serial.println("[BUZZER] bip 2 kali");
  });

  Serial.println("BAB 18 - Program 5: Event Bus dengan Antrean Event");
  Serial.println("Tekan S1, atau tutup LDR dengan telapak tangan.");

  // Kirim keadaan cahaya awal supaya LED langsung sesuai kondisi ruangan
  int nilaiAwal = analogRead(LDR_PIN);
  sedangGelap = nilaiAwal > AMBANG_GELAP;
  kirimEvent(sedangGelap ? EVENT_JADI_GELAP : EVENT_JADI_TERANG, nilaiAwal);
}

void loop() {
  periksaTombol();  // sumber event 1
  periksaCahaya();  // sumber event 2
  prosesEvent();    // bagikan satu event (kalau ada) ke pendengarnya
  urusBuzzer();     // jadwal bunyi buzzer tanpa delay()
}
