/*
 * BAB 7 - Program 4: Informasi Chip ESP32
 * Menampilkan data chip, memori, dan versi software sekali saat program mulai.
 *
 * Hardware : ESP32 DEVKIT V1 di kit Bluino (tidak perlu merangkai)
 * Alat uji : Serial Monitor, baud 115200
 * Tips     : tekan tombol EN di board untuk menampilkan ulang informasinya
 */

void setup() {
  Serial.begin(115200);
  delay(1000);  // jeda 1 detik supaya Serial Monitor sempat siap

  Serial.println("=================================");
  Serial.println("      Informasi Chip ESP32");
  Serial.println("=================================");

  Serial.print("Model chip       : ");
  Serial.println(ESP.getChipModel());

  Serial.print("Revisi chip      : ");
  Serial.println(ESP.getChipRevision());  // contoh: 301 artinya revisi v3.1

  Serial.print("Jumlah core      : ");
  Serial.println(ESP.getChipCores());

  Serial.print("Frekuensi CPU    : ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");

  Serial.print("Ukuran flash     : ");
  Serial.print(ESP.getFlashChipSize() / (1024 * 1024));  // byte → MB
  Serial.println(" MB");

  Serial.print("Ukuran program   : ");
  Serial.print(ESP.getSketchSize() / 1024);  // byte → KB
  Serial.println(" KB");

  Serial.print("RAM (heap) bebas : ");
  Serial.print(ESP.getFreeHeap() / 1024);  // byte → KB
  Serial.println(" KB");

  Serial.print("Versi core esp32 : ");
  Serial.println(ESP.getCoreVersion());

  Serial.print("Versi ESP-IDF    : ");
  Serial.println(ESP.getSdkVersion());

  Serial.println("=================================");
}

void loop() {
  // Kosong: semua informasi sudah dicetak sekali di setup().
}
