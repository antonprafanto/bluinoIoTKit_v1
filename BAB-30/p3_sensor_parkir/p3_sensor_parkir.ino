/*
 * BAB 30 - Program 3: Sensor Parkir Tiga Zona
 * Jarak diukur 10 kali per detik. LED dan buzzer menunjukkan zonanya:
 *   AMAN    (lebih dari 50 cm, atau tidak ada benda) : LED hijau, buzzer diam
 *   WASPADA (20 sampai 50 cm)                        : LED kuning, bip pelan
 *   BAHAYA  (kurang dari 20 cm)                      : LED merah, bip cepat
 * Tidak ada delay() di loop(), jadi irama bip tetap rapi.
 *
 * Rangkaian:
 *   Tr   (header hijau) → IO25   (lepas dulu kabel SW2 → 25)
 *   Ec   (header hijau) → pembagi tegangan 1 kΩ + 2 kΩ → IO36 (VP)
 *   BUZZ                → IO13
 *   LED1 (merah)        → IO4
 *   LED4 (kuning)       → IO16
 *   LED2 (hijau)        → IO17
 */

#define PIN_TRIG   25
#define PIN_ECHO   36
#define BUZZER_PIN 13
#define LED_MERAH  4
#define LED_KUNING 16
#define LED_HIJAU  17

const unsigned long BATAS_TUNGGU_US = 15000;  // 15 ms: cukup untuk benda sampai ±2,4 m
const float BATAS_BAHAYA_CM  = 20.0;
const float BATAS_WASPADA_CM = 50.0;
const unsigned long LAMA_BIP_MS = 100;        // lama buzzer berbunyi pada setiap bip

enum Zona { AMAN, WASPADA, BAHAYA };

// Timer serbaguna dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerUkur    = {100, 0};  // ukur jarak setiap 100 ms (minimal 60 ms)
Timer timerLaporan = {500, 0};  // laporan ke Serial Monitor setiap 500 ms

float jarakCm = -1;             // -1 artinya tidak ada benda dalam jangkauan
Zona zona = AMAN;
bool buzzerNyala = false;
unsigned long waktuBuzzerLalu = 0;

unsigned long ukurEchoUs() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  return pulseIn(PIN_ECHO, HIGH, BATAS_TUNGGU_US);
}

Zona tentukanZona(float jarak) {
  if (jarak < 0) return AMAN;                   // tidak ada pantulan
  if (jarak < BATAS_BAHAYA_CM) return BAHAYA;
  if (jarak < BATAS_WASPADA_CM) return WASPADA;
  return AMAN;
}

const char *namaZona(Zona z) {
  if (z == BAHAYA) return "BAHAYA";
  if (z == WASPADA) return "WASPADA";
  return "AMAN";
}

// Hanya LED milik zona saat ini yang menyala
void tampilkanLed(Zona z) {
  digitalWrite(LED_HIJAU,  z == AMAN    ? HIGH : LOW);
  digitalWrite(LED_KUNING, z == WASPADA ? HIGH : LOW);
  digitalWrite(LED_MERAH,  z == BAHAYA  ? HIGH : LOW);
}

// Bip tanpa delay(): bunyi 100 ms, lalu diam 400 ms (WASPADA) atau 100 ms (BAHAYA)
void aturBuzzer(Zona z, unsigned long sekarang) {
  if (z == AMAN) {
    buzzerNyala = false;
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }
  unsigned long lamaDiam = (z == BAHAYA) ? 100 : 400;
  unsigned long lamaKeadaanIni = buzzerNyala ? LAMA_BIP_MS : lamaDiam;
  if (sekarang - waktuBuzzerLalu >= lamaKeadaanIni) {
    waktuBuzzerLalu = sekarang;
    buzzerNyala = !buzzerNyala;
    digitalWrite(BUZZER_PIN, buzzerNyala ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  tampilkanLed(AMAN);
  Serial.println("BAB 30 - Program 3: Sensor Parkir Tiga Zona");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: ukur jarak 10 kali per detik, lalu tentukan zonanya
  if (timerUkur.sudahWaktunya(sekarang)) {
    unsigned long durasi = ukurEchoUs();
    jarakCm = (durasi == 0) ? -1.0 : durasi * 0.0343 / 2;
    zona = tentukanZona(jarakCm);
    tampilkanLed(zona);
  }

  // Tugas 2: bunyikan buzzer sesuai zona
  aturBuzzer(zona, sekarang);

  // Tugas 3: laporan ke Serial Monitor
  if (timerLaporan.sudahWaktunya(sekarang)) {
    if (jarakCm < 0) {
      Serial.println("Zona AMAN    | tidak ada benda dalam jangkauan");
    } else {
      Serial.printf("Zona %-7s | jarak %5.1f cm\n", namaZona(zona), jarakCm);
    }
  }
}
