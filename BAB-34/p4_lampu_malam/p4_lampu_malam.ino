/*
 * BAB 34 - Program 4: Lampu Malam Otomatis
 * Alur: baca LDR → EMA → kalibrasi dua titik (persen cahaya)
 *       → histeresis → LED merah.
 * Sebagai pembanding, program menghitung berapa kali lampu akan berganti
 * keadaan kalau memakai satu ambang pada data mentah ("tanpa olah").
 *
 * Rangkaian: LDR → IO35, LED1 (merah) → IO4
 */

#define PIN_LDR   35  // LDR → IO35 (makin gelap, nilai ADC makin besar)
#define LED_MERAH 4   // LED1 (merah) → IO4

// ── Hasil kalibrasimu ──
const int TITIK_GELAP  = 3900;  // ADC saat LDR ditutup rapat    ← ganti dengan angkamu
const int TITIK_TERANG = 300;   // ADC saat LDR disinari senter  ← ganti dengan angkamu
static_assert(TITIK_GELAP > TITIK_TERANG, "TITIK_GELAP harus lebih besar dari TITIK_TERANG");

// ── Ambang (dalam persen cahaya) ──
const float NYALA_DI_BAWAH = 30.0;  // lebih gelap dari ini → lampu nyala
const float MATI_DI_ATAS   = 40.0;  // lebih terang dari ini → lampu mati
const float AMBANG_TUNGGAL = 35.0;  // hanya untuk pembanding "tanpa olah"

const float ALPHA = 0.1;  // EMA; dengan sampel setiap 20 ms, konstanta waktunya ±0,18 detik

// Timer serbaguna dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerSampel  = {20, 0};   // baca LDR 50 kali per detik
Timer timerLaporan = {500, 0};  // cetak laporan 2 kali per detik

int adcMentah = 0;
float adcHalus = 0;             // hasil EMA
bool lampuNyala = false;        // keputusan sungguhan: data halus + histeresis
bool lampuSatuAmbang = false;   // keputusan pembanding: data mentah + satu ambang
unsigned long gantiTanpaOlah = 0;
unsigned long gantiDenganOlah = 0;

// Kalibrasi dua titik: TITIK_GELAP → 0 %, TITIK_TERANG → 100 %
float kePersenCahaya(float adc) {
  float persen = (TITIK_GELAP - adc) * 100.0f / (TITIK_GELAP - TITIK_TERANG);
  return constrain(persen, 0.0f, 100.0f);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);

  adcMentah = analogRead(PIN_LDR);
  adcHalus = adcMentah;  // nilai awal EMA = bacaan pertama
  lampuSatuAmbang = kePersenCahaya(adcMentah) < AMBANG_TUNGGAL;
  lampuNyala = kePersenCahaya(adcHalus) < NYALA_DI_BAWAH;  // kalau sudah gelap sejak awal, langsung nyala
  digitalWrite(LED_MERAH, lampuNyala ? HIGH : LOW);

  Serial.println("BAB 34 - Program 4: Lampu Malam Otomatis");
  Serial.printf("Titik gelap %d, titik terang %d | Nyala di bawah %.0f %%, mati di atas %.0f %%\n",
                TITIK_GELAP, TITIK_TERANG, NYALA_DI_BAWAH, MATI_DI_ATAS);
}

void loop() {
  unsigned long sekarang = millis();

  // ── Tugas 1: baca dan olah LDR setiap 20 ms ──
  if (timerSampel.sudahWaktunya(sekarang)) {
    adcMentah = analogRead(PIN_LDR);
    adcHalus = adcHalus + ALPHA * (adcMentah - adcHalus);  // EMA

    // Pembanding: satu ambang pada data mentah
    bool keputusanMentah = kePersenCahaya(adcMentah) < AMBANG_TUNGGAL;
    if (keputusanMentah != lampuSatuAmbang) {
      lampuSatuAmbang = keputusanMentah;
      gantiTanpaOlah = gantiTanpaOlah + 1;
    }

    // Keputusan sungguhan: data halus + histeresis
    float persen = kePersenCahaya(adcHalus);
    bool lampuBaru = lampuNyala;
    if (!lampuNyala && persen < NYALA_DI_BAWAH) {
      lampuBaru = true;
    } else if (lampuNyala && persen > MATI_DI_ATAS) {
      lampuBaru = false;
    }

    if (lampuBaru != lampuNyala) {
      lampuNyala = lampuBaru;
      gantiDenganOlah = gantiDenganOlah + 1;
      digitalWrite(LED_MERAH, lampuNyala ? HIGH : LOW);
      Serial.printf(">> Lampu %s (cahaya %.1f %%)\n", lampuNyala ? "NYALA" : "MATI", persen);
    }
  }

  // ── Tugas 2: laporan setiap 500 ms ──
  if (timerLaporan.sudahWaktunya(sekarang)) {
    Serial.printf("ADC mentah %4d | ADC halus %6.1f | Cahaya %5.1f %% | Lampu %-5s | Ganti: tanpa olah %lu, dengan olah %lu\n",
                  adcMentah, adcHalus, kePersenCahaya(adcHalus), lampuNyala ? "NYALA" : "MATI",
                  gantiTanpaOlah, gantiDenganOlah);
  }
}
