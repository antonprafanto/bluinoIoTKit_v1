/*
 * BAB 55 - Program 4: Gateway Mesh ke MQTT
 * Node ini tersambung ke mesh DAN ke WiFi router sekaligus:
 *  - setiap pesan dari mesh diteruskan ke bluino/<ID_TOPIK>/mesh/node/<NodeID>
 *  - ON / OFF / TOGGLE di bluino/<ID_TOPIK>/mesh/perintah disiarkan ke mesh
 *    sebagai {"jenis":"perintah","led":...} untuk node pekerja (Program 3B)
 *  - ringkasan mesh dikirim ke bluino/<ID_TOPIK>/mesh/info setiap 30 detik
 *  - status online/offline (retained + Last Will) di bluino/<ID_TOPIK>/mesh/status
 * Penting: MESH_CHANNEL di semua node harus sama dengan kanal router.
 *
 * Rangkaian: tidak perlu
 * Library  : Painless Mesh (+ TaskScheduler, ArduinoJson 7, Async TCP),
 *            PubSubClient (Nick O'Leary)
 */

#include <painlessMesh.h>
#include <PubSubClient.h>
#include "esp_mac.h"  // esp_read_mac(): membaca alamat MAC untuk Client ID

// Pengaturan mesh: harus SAMA PERSIS di semua node
#define MESH_PREFIX   "BluinoMesh-andi"  // ← ganti "andi" dengan nama panggilanmu
#define MESH_PASSWORD "rahasia123"       // minimal 8 karakter
#define MESH_PORT     5555
#define MESH_CHANNEL  1                  // ← ganti dengan kanal router (lihat Serial Monitor)

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

// Broker publik gratis tanpa akun (sama dengan BAB 50). Cadangan: "broker.emqx.io"
const char *MQTT_BROKER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;

// Awalan topik unik: nama panggilan + 4 angka acak (huruf kecil, tanpa spasi)
#define ID_TOPIK "andi-4821"  // ← ganti dengan awalan milikmu
const char *TOPIK_STATUS = "bluino/" ID_TOPIK "/mesh/status";
const char *TOPIK_INFO = "bluino/" ID_TOPIK "/mesh/info";
const char *TOPIK_PERINTAH = "bluino/" ID_TOPIK "/mesh/perintah";
const char *AWALAN_TOPIK_NODE = "bluino/" ID_TOPIK "/mesh/node/";

// struct Timer dari BAB 16
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

painlessMesh mesh;
WiFiClient wifiClient;          // jalur TCP yang dipakai MQTT
PubSubClient mqtt(wifiClient);  // client MQTT

Timer timerInfo = {30000, 0};  // ringkasan mesh setiap 30 detik
char idKlien[24];              // Client ID, misalnya "bluino-mesh-A1B2C3"
unsigned long jumlahDiteruskan = 0;

// Variabel sambung ulang MQTT dengan backoff (sama dengan BAB 50)
unsigned long jedaSambung = 0;  // jeda sebelum percobaan berikutnya (0 = boleh langsung)
unsigned long waktuCoba = 0;    // millis() saat percobaan terakhir
bool mqttTadiTersambung = false;
bool wifiTadiTersambung = false;

// Client ID dari 3 byte terakhir alamat MAC, supaya setiap board berbeda
void buatIdKlien() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(idKlien, sizeof(idKlien), "bluino-mesh-%02X%02X%02X", mac[3], mac[4], mac[5]);
}

// Sekali di setup(), SEBELUM mesh.init(): cari kanal router dan bandingkan
void periksaKanalRouter() {
  Serial.printf("[WiFi] Mencari \"%s\" untuk mengetahui kanalnya...\n", NAMA_WIFI);
  WiFi.mode(WIFI_STA);
  int jumlah = WiFi.scanNetworks();  // menunggu 2-4 detik; aman karena mesh belum berjalan
  int kanalRouter = 0;
  for (int i = 0; i < jumlah; i++) {
    if (WiFi.SSID(i) == NAMA_WIFI) {
      kanalRouter = WiFi.channel(i);
      break;
    }
  }
  WiFi.scanDelete();  // bebaskan memori hasil pindai

  if (kanalRouter == 0) {
    Serial.println("[WiFi] Router tidak ditemukan. Periksa nama WiFi dan pastikan 2,4 GHz.");
  } else if (kanalRouter > 11) {
    Serial.printf("[WiFi] Router ada di kanal %d. Ubah kanal router ke 1-11 (misalnya 1, 6, 11).\n",
                  kanalRouter);
  } else if (kanalRouter != MESH_CHANNEL) {
    Serial.printf("[WiFi] Perhatian: router ada di kanal %d, tetapi MESH_CHANNEL = %d.\n",
                  kanalRouter, MESH_CHANNEL);
    Serial.printf("[WiFi] Ubah MESH_CHANNEL menjadi %d di semua sketch, lalu upload ulang.\n",
                  kanalRouter);
  } else {
    Serial.printf("[WiFi] Router ada di kanal %d, sama dengan MESH_CHANNEL. Bagus!\n", kanalRouter);
  }
}

// Satu kali percobaan menyambung ke broker. Hasilnya true kalau berhasil.
bool sambungMqtt() {
  Serial.printf("[MQTT] Menyambung ke %s:%u sebagai %s ... ", MQTT_BROKER, MQTT_PORT, idKlien);
  // Last Will: kalau gateway putus mendadak, broker mengumumkan "offline" (retained)
  if (mqtt.connect(idKlien, TOPIK_STATUS, 1, true, "offline")) {
    Serial.println("berhasil!");
    mqtt.publish(TOPIK_STATUS, "online", true);
    mqtt.subscribe(TOPIK_PERINTAH);
    return true;
  }
  Serial.printf("gagal, state=%d\n", mqtt.state());
  return false;
}

// Dipanggil setiap putaran loop(). Tidak pernah menunggu di dalam perulangan.
void jagaKoneksi() {
  // 1. Laporkan perubahan status WiFi (sambung ulang ke router dikerjakan Painless Mesh)
  bool wifiTersambung = (WiFi.status() == WL_CONNECTED);
  if (wifiTersambung != wifiTadiTersambung) {
    wifiTadiTersambung = wifiTersambung;
    if (wifiTersambung) {
      Serial.printf("[WiFi] Tersambung ke router, IP: %s\n", mesh.getStationIP().toString().c_str());
    } else {
      Serial.println("[WiFi] Terputus dari router, Painless Mesh mencoba menyambung lagi...");
    }
  }

  // 2. Laporkan kalau koneksi MQTT baru saja putus
  bool mqttTersambung = mqtt.connected();
  if (mqttTadiTersambung && !mqttTersambung) {
    Serial.printf("[MQTT] Koneksi terputus, state=%d\n", mqtt.state());
  }
  mqttTadiTersambung = mqttTersambung;
  if (mqttTersambung || !wifiTersambung) {
    return;  // sudah tersambung, atau WiFi belum siap
  }

  // 3. Belum waktunya mencoba lagi? Langsung keluar.
  unsigned long sekarang = millis();
  if (sekarang - waktuCoba < jedaSambung) {
    return;
  }
  waktuCoba = sekarang;

  if (sambungMqtt()) {
    mqttTadiTersambung = true;
    jedaSambung = 0;  // berhasil: kalau nanti putus, langsung coba lagi
  } else {
    // gagal: jeda 2, 4, 8, 16, 32, lalu paling lama 60 detik
    jedaSambung = (jedaSambung == 0) ? 2000 : min(jedaSambung * 2, 60000UL);
    Serial.printf("[MQTT] Coba lagi dalam %lu detik\n", jedaSambung / 1000);
  }
}

// Pesan dari mesh -> MQTT (topik per node)
void pesanDariMesh(uint32_t dari, String &pesan) {
  Serial.printf("[MESH] dari %lu: %s\n", (unsigned long)dari, pesan.c_str());
  if (!mqtt.connected()) {
    Serial.println("[PUB] MQTT belum tersambung, pesan tidak diteruskan");
    return;
  }
  char topik[80];
  snprintf(topik, sizeof(topik), "%s%lu", AWALAN_TOPIK_NODE, (unsigned long)dari);
  if (mqtt.publish(topik, pesan.c_str())) {
    jumlahDiteruskan = jumlahDiteruskan + 1;
    Serial.printf("[PUB] %s\n", topik);
  } else {
    Serial.println("[PUB] Gagal! Koneksi putus atau pesan lebih besar dari buffer.");
  }
}

// Pesan dari MQTT (topik perintah) -> disiarkan ke mesh
void pesanDariMqtt(char *topik, byte *payload, unsigned int panjang) {
  char isi[16];  // payload TIDAK diakhiri '\0', jadi salin dulu
  unsigned int n = min(panjang, (unsigned int)(sizeof(isi) - 1));
  memcpy(isi, payload, n);
  isi[n] = '\0';
  Serial.printf("[MQTT] Perintah masuk di %s: %s\n", topik, isi);

  const char *led;
  if (strcasecmp(isi, "ON") == 0) {
    led = "ON";
  } else if (strcasecmp(isi, "OFF") == 0) {
    led = "OFF";
  } else if (strcasecmp(isi, "TOGGLE") == 0) {
    led = "TOGGLE";
  } else {
    Serial.println("[MQTT] Perintah tidak dikenal. Pakai ON, OFF, atau TOGGLE.");
    return;
  }

  JsonDocument doc;
  doc["jenis"] = "perintah";
  doc["led"] = led;
  String pesan;
  serializeJson(doc, pesan);
  if (mesh.sendBroadcast(pesan)) {
    Serial.printf("[MESH] Disiarkan ke mesh: %s\n", pesan.c_str());
  } else {
    Serial.println("[MESH] Gagal: belum ada node lain di mesh.");
  }
}

void susunanBerubah() {
  Serial.printf("[MESH] Susunan berubah. Jumlah node: %u\n",
                (unsigned)mesh.getNodeList(true).size());
}

// Ringkasan mesh -> topik info
void kirimInfo() {
  if (!mqtt.connected()) {
    return;
  }
  std::list<uint32_t> daftar = mesh.getNodeList();  // node lain (tanpa gateway)

  JsonDocument doc;
  doc["gateway"] = mesh.getNodeId();
  doc["jumlah_node"] = daftar.size() + 1;  // termasuk gateway
  JsonArray nodeLain = doc["node_lain"].to<JsonArray>();
  for (uint32_t id : daftar) {
    nodeLain.add(id);
  }
  doc["diteruskan"] = jumlahDiteruskan;
  doc["waktu_nyala"] = millis() / 1000;  // detik

  String teks;
  serializeJson(doc, teks);
  if (mqtt.publish(TOPIK_INFO, teks.c_str())) {
    Serial.printf("[PUB] %s -> %s\n", TOPIK_INFO, teks.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nBAB 55 - Program 4: Gateway Mesh ke MQTT");

  buatIdKlien();
  periksaKanalRouter();

  mesh.setDebugMsgTypes(ERROR);  // hanya pesan error dari library
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.onReceive(&pesanDariMesh);
  mesh.onChangedConnections(&susunanBerubah);
  mesh.stationManual(NAMA_WIFI, PASSWORD_WIFI);  // station ke router, bukan ke node lain
  mesh.setRoot(true);                            // gateway menjadi root mesh
  mesh.setContainsRoot(true);                    // mesh ini punya root

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(pesanDariMqtt);
  mqtt.setBufferSize(512);                // ringkasan dengan banyak node bisa > 256 byte
  mqtt.setSocketTimeout(3);               // tunggu jawaban broker paling lama 3 detik
  wifiClient.setConnectionTimeout(2000);  // buka koneksi TCP paling lama 2 detik

  Serial.printf("Node ID gateway: %lu\n", (unsigned long)mesh.getNodeId());
  Serial.printf("Client ID MQTT : %s\n", idKlien);
  Serial.printf("Topik data     : %s<NodeID>\n", AWALAN_TOPIK_NODE);
  Serial.printf("Topik perintah : %s\n", TOPIK_PERINTAH);
}

void loop() {
  mesh.update();  // wajib, sesering mungkin
  jagaKoneksi();  // jaga koneksi MQTT (tanpa menunggu)
  mqtt.loop();    // keep-alive dan pesan masuk dari broker

  if (timerInfo.sudahWaktunya(millis())) {
    kirimInfo();
  }
}
