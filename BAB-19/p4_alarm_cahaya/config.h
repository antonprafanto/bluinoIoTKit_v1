// config.h — semua pengaturan proyek di satu tempat.
// Mau pindah pin atau mengubah kepekaan alarm? Cukup ubah file ini.
#pragma once

// ---------- Pin (Peta Pin Standar kit Bluino) ----------
const int PIN_LED_MERAH = 4;   // LED1 (merah)
const int PIN_LED_HIJAU = 17;  // LED2 (hijau)
const int PIN_BUZZER = 13;     // BUZZ (buzzer aktif)
const int PIN_TOMBOL = 15;     // SW1 (tombol S1), ditekan = HIGH
const int PIN_LDR = 35;        // LDR: makin gelap, nilai ADC makin besar

// ---------- Pengaturan alarm ----------
const int AMBANG_GELAP = 3000;               // nilai LDR di atas ini = cahaya terhalang
const unsigned long JEDA_BACA_LDR = 100;     // ms, seberapa sering LDR dibaca
const unsigned long JEDA_KEDIP_ALARM = 150;  // ms, kecepatan kedip LED dan bunyi alarm
const unsigned long WAKTU_DEBOUNCE = 50;     // ms, debounce tombol
