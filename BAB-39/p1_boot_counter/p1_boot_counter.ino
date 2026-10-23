/*
 * BAB 39 - Program 1: Penghitung Boot
 * Setiap kali ESP32 menyala atau di-reset, angka boot bertambah 1.
 * Angkanya disimpan di NVS, jadi tidak kembali ke 0 walaupun
 * listrik dicabut atau program di-upload ulang.
 *
 * Rangkaian: tidak perlu, cukup ESP32 dan kabel USB.
 * Alat uji : Serial Monitor, baud 115200.
 */

#include <Preferences.h>   // library bawaan esp32 core untuk NVS

Preferences prefs;          // objek untuk mengakses NVS

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung lagi setelah reset

  Serial.println();
  Serial.println("BAB 39 - Program 1: Penghitung Boot");

  // Buka namespace "boot" dalam mode baca-tulis (false = bukan hanya-baca).
  // Kalau namespace belum ada, NVS membuatnya otomatis.
  if (!prefs.begin("boot", false)) {
    Serial.println("Gagal membuka NVS!");
    return;
  }

  uint32_t jumlahBoot = prefs.getUInt("jumlah", 0);   // 0 kalau key belum ada
  jumlahBoot = jumlahBoot + 1;
  size_t tertulis = prefs.putUInt("jumlah", jumlahBoot);  // simpan nilai baru
  prefs.end();                                           // tutup namespace

  if (tertulis == 0) {
    Serial.println("Gagal menyimpan ke NVS!");
  }

  Serial.printf("ESP32 ini sudah menyala %lu kali.\n", (unsigned long)jumlahBoot);
  Serial.println("Tekan tombol EN, cabut-pasang kabel USB, atau upload ulang,");
  Serial.println("lalu lihat angkanya bertambah.");
}

void loop() {
  // Tidak ada yang dikerjakan di sini.
}
