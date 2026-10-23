/*
 * BAB 22 - Program 1: Mengintip Byte dari Serial Monitor
 * Setiap byte yang tiba di ESP32 ditampilkan beserta kodenya, sehingga
 * terlihat karakter apa saja yang dikirim Serial Monitor, termasuk
 * karakter akhir baris (line ending) yang biasanya tak terlihat.
 *
 * Rangkaian: tidak ada, cukup kabel USB.
 * Alat uji : Serial Monitor, baud 115200. Coba keempat pilihan line ending.
 */

const unsigned long JEDA_AKHIR_PESAN = 200;  // ms tanpa byte baru = pesan selesai

int jumlahByte = 0;                    // banyak byte dalam pesan yang sedang diterima
unsigned long waktuByteTerakhir = 0;   // kapan byte terakhir tiba

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 22 - Program 1: Mengintip Byte dari Serial Monitor");
  Serial.println("Ketik teks pendek, misalnya ok, lalu tekan Enter.");
}

void loop() {
  // Ambil hanya byte yang SUDAH tiba, tanpa menunggu (non-blocking)
  while (Serial.available() > 0) {
    int b = Serial.read();             // satu byte, nilainya 0-255
    jumlahByte++;
    waktuByteTerakhir = millis();

    // Siapkan nama yang mudah dibaca untuk byte ini
    char nama[12];
    if (b == '\r') {
      strcpy(nama, "CR \\r");          // tercetak: CR \r
    } else if (b == '\n') {
      strcpy(nama, "LF \\n");          // tercetak: LF \n
    } else if (b >= 32 && b <= 126) {  // huruf, angka, tanda baca, spasi
      snprintf(nama, sizeof(nama), "'%c'", b);
    } else if (b >= 128) {             // bagian dari huruf di luar ASCII, misalnya é
      strcpy(nama, "non-ASCII");
    } else {
      strcpy(nama, "kontrol");         // karakter kendali lain, misalnya Tab
    }

    Serial.printf("Byte ke-%d: %-9s kode %3d (0x%02X)\n", jumlahByte, nama, b, b);
  }

  // Tidak ada byte baru selama 200 ms → anggap satu pesan sudah selesai
  if (jumlahByte > 0 && millis() - waktuByteTerakhir >= JEDA_AKHIR_PESAN) {
    Serial.printf("--- total %d byte ---\n", jumlahByte);
    Serial.println();
    jumlahByte = 0;
  }
}
