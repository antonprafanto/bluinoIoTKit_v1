/*
 * BAB 19 - Program 4: Proyek Terstruktur — Alarm Cahaya
 * Tekan S1 untuk menyiagakan alarm (LED hijau menyala). Saat siaga,
 * kalau cahaya ke LDR terhalang (misalnya ditutup telapak tangan),
 * alarm berbunyi sampai S1 ditekan lagi.
 *
 * File: p4_alarm_cahaya.ino (utama), config.h, Led.h, Led.cpp, Tombol.h, Tombol.cpp
 * Rangkaian (nomor pin ada di config.h):
 *   LED1 (merah)          → IO4
 *   LED2 (hijau)          → IO17
 *   BUZZ (buzzer aktif)   → IO13
 *   Tombol S1 (SW1)       → IO15
 *   LDR                   → IO35
 */

#include "config.h"
#include "Led.h"
#include "Tombol.h"

// Keadaan alarm (state machine seperti di BAB 17)
enum KeadaanAlarm {
  ALARM_MATI,
  ALARM_SIAGA,
  ALARM_BERBUNYI
};

KeadaanAlarm keadaan = ALARM_MATI;

Led ledMerah(PIN_LED_MERAH);
Led ledHijau(PIN_LED_HIJAU);
Led buzzer(PIN_BUZZER);  // buzzer aktif dikendalikan persis seperti LED (BAB 8)
Tombol tombolS1(PIN_TOMBOL, WAKTU_DEBOUNCE);

unsigned long waktuBacaLdr = 0;

// Pindah ke keadaan baru: matikan semua output, lalu atur sesuai keadaannya
void gantiKeadaan(KeadaanAlarm baru) {
  keadaan = baru;
  ledMerah.mati();
  ledHijau.mati();
  buzzer.mati();

  switch (keadaan) {
    case ALARM_MATI:
      Serial.println("Alarm MATI");
      break;
    case ALARM_SIAGA:
      ledHijau.nyala();
      Serial.println("Alarm SIAGA - tutup LDR dengan telapak tangan untuk menguji");
      break;
    case ALARM_BERBUNYI:
      Serial.println("ALARM BERBUNYI! Tekan S1 untuk mematikan");
      break;
  }
}

// Callback tombol S1: dari MATI menjadi SIAGA, dari keadaan lain menjadi MATI
void saatS1Ditekan() {
  if (keadaan == ALARM_MATI) {
    gantiKeadaan(ALARM_SIAGA);
  } else {
    gantiKeadaan(ALARM_MATI);
  }
}

void setup() {
  Serial.begin(115200);
  ledMerah.begin();
  ledHijau.begin();
  buzzer.begin();
  tombolS1.begin();
  tombolS1.saatDitekan(saatS1Ditekan);  // daftarkan callback (BAB 18)

  Serial.println("BAB 19 - Program 4: Proyek Terstruktur - Alarm Cahaya");
  Serial.println("Tekan S1 untuk menyiagakan alarm");
  gantiKeadaan(ALARM_MATI);
}

void loop() {
  tombolS1.update();

  // Saat SIAGA: baca LDR secara berkala
  if (keadaan == ALARM_SIAGA && millis() - waktuBacaLdr >= JEDA_BACA_LDR) {
    waktuBacaLdr = millis();
    int nilaiLdr = analogRead(PIN_LDR);
    if (nilaiLdr > AMBANG_GELAP) {
      Serial.print("Cahaya terhalang! Nilai LDR = ");
      Serial.println(nilaiLdr);
      gantiKeadaan(ALARM_BERBUNYI);
    }
  }

  // Saat BERBUNYI: LED merah dan buzzer berkedip bersamaan
  if (keadaan == ALARM_BERBUNYI) {
    ledMerah.kedip(JEDA_KEDIP_ALARM);
    buzzer.kedip(JEDA_KEDIP_ALARM);
  }
}
