// pola_led.ino — tab kedua: kumpulan pola nyala LED.
// Tidak perlu #include apa pun: Arduino IDE menggabungkan tab ini di bawah
// file utama, jadi LED_MERAH, LED_HIJAU, dan putaran sudah dikenal di sini.

// Mencetak nama pola beserta nomor putarannya
void cetakPola(const char *nama) {
  Serial.print("Putaran ");
  Serial.print(putaran);
  Serial.print(": ");
  Serial.println(nama);
}

// LED merah dan hijau menyala bergantian sebanyak "kali"
void kedipBergantian(int kali) {
  cetakPola("kedip bergantian");
  for (int i = 0; i < kali; i++) {
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(LED_HIJAU, LOW);
    delay(250);
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(LED_HIJAU, HIGH);
    delay(250);
  }
  digitalWrite(LED_HIJAU, LOW);
}

// Kedua LED menyala bersama selama "lamaMs" milidetik, lalu mati setengah detik
void nyalaBersama(unsigned long lamaMs) {
  cetakPola("nyala bersama");
  digitalWrite(LED_MERAH, HIGH);
  digitalWrite(LED_HIJAU, HIGH);
  delay(lamaMs);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);
  delay(500);
}
