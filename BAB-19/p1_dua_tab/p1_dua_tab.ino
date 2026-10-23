/*
 * BAB 19 - Program 1: Dua Tab .ino
 * File utama hanya berisi pengaturan, setup(), dan loop().
 * Fungsi-fungsi pola LED ada di tab kedua: pola_led.ino.
 *
 * File: p1_dua_tab.ino (utama), pola_led.ino
 * Rangkaian:
 *   LED1 (merah) → IO4
 *   LED2 (hijau) → IO17
 */

// Pin dan variabel global ditulis di file utama, paling atas,
// supaya dikenal juga oleh tab .ino yang lain.
#define LED_MERAH 4
#define LED_HIJAU 17

int putaran = 0;  // berapa kali loop() sudah berjalan

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  Serial.println("BAB 19 - Program 1: Dua Tab .ino");
}

void loop() {
  putaran++;
  kedipBergantian(3);  // fungsi ini ditulis di tab pola_led.ino
  nyalaBersama(1000);  // fungsi ini juga ada di tab pola_led.ino
}
