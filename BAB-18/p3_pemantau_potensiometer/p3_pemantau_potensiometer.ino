/*
 * BAB 18 - Program 3: Callback dengan Data — Pemantau Potensiometer
 * PemantauSensor membaca ADC setiap 100 ms dan memanggil callback
 * hanya saat nilainya berubah minimal sebesar "ambang".
 *
 * Rangkaian: Potensiometer (VR10K, pin kuning ke-5) → IO34
 */

#define POT_PIN 34

// Tipe callback yang menerima dua data: nilai lama dan nilai baru
typedef void (*CallbackSensor)(int nilaiLama, int nilaiBaru);

struct PemantauSensor {
  int pin = -1;
  int ambang = 50;               // perubahan minimum agar dianggap "berubah"
  unsigned long interval = 100;  // jeda antarpembacaan (ms)
  CallbackSensor saatBerubah = nullptr;

  int nilaiTerakhir = 0;         // nilai saat callback terakhir dipanggil
  unsigned long waktuBaca = 0;

  void begin(int nomorPin, int ambangPerubahan, unsigned long intervalMs) {
    pin = nomorPin;
    ambang = ambangPerubahan;
    interval = intervalMs;
    nilaiTerakhir = analogRead(pin);  // nilai awal sebagai pembanding
    waktuBaca = millis();
  }

  // Panggil sesering mungkin di loop()
  void update() {
    if (millis() - waktuBaca < interval) return;  // belum waktunya membaca
    waktuBaca = millis();

    int nilai = analogRead(pin);  // 0–4095
    if (abs(nilai - nilaiTerakhir) >= ambang) {
      int lama = nilaiTerakhir;
      nilaiTerakhir = nilai;      // simpan dulu, baru beri tahu
      if (saatBerubah != nullptr) saatBerubah(lama, nilai);  // kirim data ke callback
    }
  }
};

// Callback: menerima data dari PemantauSensor lewat parameternya
void tampilkanPerubahan(int lama, int baru) {
  int persen = map(baru, 0, 4095, 0, 100);
  Serial.print("Potensiometer berubah: ");
  Serial.print(lama);
  Serial.print(" -> ");
  Serial.print(baru);
  Serial.print(" (");
  Serial.print(persen);
  Serial.println("%)");
}

PemantauSensor pemantauPot;

void setup() {
  Serial.begin(115200);

  pemantauPot.begin(POT_PIN, 50, 100);  // ambang 50, baca setiap 100 ms
  pemantauPot.saatBerubah = tampilkanPerubahan;

  Serial.println("BAB 18 - Program 3: Pemantau Potensiometer dengan Callback");
  Serial.println("Putar potensiometer pelan-pelan...");
}

void loop() {
  pemantauPot.update();
}
