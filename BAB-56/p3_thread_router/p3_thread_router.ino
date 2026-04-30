/*
 * BAB 56 - Program 3: Thread Router Basic
 *
 * PLATFORM: ESP32-H2 SAJA!
 * ESP32-H2 memiliki radio IEEE 802.15.4 yang dibutuhkan Thread.
 *
 * Development Environment:
 *   - ESP-IDF v5.1.2+
 *   - Aktifkan komponen OpenThread di sdkconfig
 *   - Build: idf.py set-target esp32h2 && idf.py build && idf.py flash
 *
 * Fungsi:
 *   ► Inisiasi OpenThread stack
 *   ► Bergabung ke Thread network (atau buat network baru jika belum ada)
 *   ► Tampilkan Thread network info: Leader, Router Table, IPv6 addresses
 *   ► Kirim/terima UDP message antar node Thread
 *   ► Heartbeat tampilkan status Thread tiap 30 detik
 *
 * Hardware: ESP32-H2 (tidak ada pin tambahan yang dibutuhkan)
 */

// ── Includes OpenThread (ESP-IDF) ─────────────────────────────────────
#include "esp_openthread.h"
#include "esp_openthread_netif_glue.h"
#include "esp_openthread_types.h"
#include "openthread/cli.h"
#include "openthread/dataset.h"
#include "openthread/dataset_ftd.h"
#include "openthread/instance.h"
#include "openthread/logging.h"
#include "openthread/thread.h"
#include "openthread/thread_ftd.h"
#include "openthread/udp.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "BAB56_P3";

// ── Thread Network Dataset (gunakan nilai unik untuk jaringanmu!) ──────
// Hasilkan dataset baru: idf.py monitor → ketik 'dataset init new' → 'dataset'
#define THREAD_NETWORK_NAME   "BluinoMesh"
#define THREAD_CHANNEL        15           // Channel 802.15.4 (11-26)
#define THREAD_PANID          0x1234       // PAN ID (hex, unik per jaringan)

// ── UDP Port untuk komunikasi antar node ──────────────────────────────
#define THREAD_UDP_PORT       5683

static otUdpSocket  sUdpSocket;
static otInstance  *sInstance = NULL;
uint32_t txCount = 0, rxCount = 0;

// ── Callback: terima pesan UDP dari node lain ──────────────────────────
void udpReceiveCallback(void *aContext, otMessage *aMessage,
                        const otMessageInfo *aMessageInfo) {
  char buf[128] = {0};
  int  len = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
  buf[len] = '\0';
  rxCount++;

  char srcAddr[OT_IP6_ADDRESS_STRING_SIZE];
  otIp6AddressToString(&aMessageInfo->mPeerAddr, srcAddr, sizeof(srcAddr));
  ESP_LOGI(TAG, "[UDP] Terima #%u dari %s: '%s'", rxCount, srcAddr, buf);
}

// ── Fungsi: kirim UDP broadcast ke semua node Thread ──────────────────
void sendThreadBroadcast(const char *message) {
  otMessageInfo msgInfo;
  memset(&msgInfo, 0, sizeof(msgInfo));

  // Alamat multicast Thread semua node: ff03::1
  otIp6AddressFromString("ff03::1", &msgInfo.mPeerAddr);
  msgInfo.mPeerPort    = THREAD_UDP_PORT;
  msgInfo.mSockPort    = THREAD_UDP_PORT;

  otMessage *msg = otUdpNewMessage(sInstance, NULL);
  if (!msg) { ESP_LOGE(TAG, "Gagal alokasi UDP message!"); return; }

  otMessageAppend(msg, message, strlen(message));
  otError err = otUdpSend(sInstance, &sUdpSocket, msg, &msgInfo);

  if (err == OT_ERROR_NONE) {
    txCount++;
    ESP_LOGI(TAG, "[UDP] Broadcast #%u: '%s'", txCount, message);
  } else {
    otMessageFree(msg);
    ESP_LOGE(TAG, "[UDP] Gagal kirim: %d", err);
  }
}

// ── Fungsi: tampilkan info Thread network ─────────────────────────────
void printThreadStatus() {
  if (!sInstance) return;

  otDeviceRole role = otThreadGetDeviceRole(sInstance);
  const char *roleStr;
  switch (role) {
    case OT_DEVICE_ROLE_LEADER:     roleStr = "Leader";     break;
    case OT_DEVICE_ROLE_ROUTER:     roleStr = "Router";     break;
    case OT_DEVICE_ROLE_CHILD:      roleStr = "Child (End Device)"; break;
    case OT_DEVICE_ROLE_DETACHED:   roleStr = "Detached";   break;
    default:                         roleStr = "Disabled";   break;
  }

  uint16_t rloc16 = otThreadGetRloc16(sInstance);
  uint8_t  channel = otLinkGetChannel(sInstance);

  ESP_LOGI(TAG, "[THREAD] Role:%s | RLOC16:0x%04X | Channel:%u | Tx:%u Rx:%u",
    roleStr, rloc16, channel, txCount, rxCount);

  // Tampilkan alamat IPv6 node ini
  const otNetifAddress *addr = otIp6GetUnicastAddresses(sInstance);
  int addrIdx = 0;
  while (addr && addrIdx < 3) {
    char addrStr[OT_IP6_ADDRESS_STRING_SIZE];
    otIp6AddressToString(&addr->mAddress, addrStr, sizeof(addrStr));
    ESP_LOGI(TAG, "[THREAD] IPv6[%d]: %s", addrIdx++, addrStr);
    addr = addr->mNext;
  }
}

// ── Init Thread dataset & start stack ─────────────────────────────────
void initThreadDataset() {
  otOperationalDataset dataset;
  memset(&dataset, 0, sizeof(dataset));

  // Cek apakah dataset sudah ada di NVS
  if (otDatasetIsCommissioned(sInstance)) {
    ESP_LOGI(TAG, "[THREAD] Dataset sudah ada, bergabung ke jaringan...");
  } else {
    // Buat dataset baru
    otDatasetCreateNewNetwork(sInstance, &dataset);
    // Set nama jaringan
    strncpy(dataset.mNetworkName.m8, THREAD_NETWORK_NAME,
      sizeof(dataset.mNetworkName.m8));
    dataset.mComponents.mIsNetworkNamePresent = true;
    // Set channel
    dataset.mChannel                          = THREAD_CHANNEL;
    dataset.mComponents.mIsChannelPresent     = true;
    // Set PAN ID
    dataset.mPanId                            = THREAD_PANID;
    dataset.mComponents.mIsPanIdPresent       = true;

    otDatasetSetActive(sInstance, &dataset);
    ESP_LOGI(TAG, "[THREAD] Dataset baru dibuat: '%s' ch%u PAN:0x%04X",
      THREAD_NETWORK_NAME, THREAD_CHANNEL, THREAD_PANID);
  }

  // Start Thread interface & jaringan
  otIp6SetEnabled(sInstance, true);
  otThreadSetEnabled(sInstance, true);
  ESP_LOGI(TAG, "[THREAD] ✅ Stack Thread aktif!");
}

extern "C" void app_main() {
  ESP_LOGI(TAG, "\n=== BAB 56 Program 3: Thread Router Basic ===");

  // ── Init OpenThread platform ───────────────────────────────────────
  esp_openthread_platform_config_t config = {
    .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
    .host_config  = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
    .port_config  = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
  };
  esp_openthread_init(&config);

  sInstance = esp_openthread_get_instance();

  // ── Buka UDP socket ────────────────────────────────────────────────
  otSockAddr sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));
  sockAddr.mPort = THREAD_UDP_PORT;
  otUdpOpen(sInstance, &sUdpSocket, udpReceiveCallback, NULL);
  otUdpBind(sInstance, &sUdpSocket, &sockAddr, OT_NETIF_THREAD);
  ESP_LOGI(TAG, "[UDP] Socket terbuka di port %u", THREAD_UDP_PORT);

  // ── Init Thread dataset & start network ───────────────────────────
  initThreadDataset();

  // ── Main loop ─────────────────────────────────────────────────────
  ESP_LOGI(TAG, "[THREAD] Menunggu join ke jaringan Thread...");
  unsigned long lastStatus  = 0;
  unsigned long lastBcast   = 0;

  while (true) {
    // Proses OpenThread events
    esp_openthread_process_next_event();

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

    // Tampilkan status Thread tiap 30 detik
    if (now - lastStatus >= 30000) {
      lastStatus = now;
      printThreadStatus();
    }

    // Kirim broadcast ke semua node tiap 10 detik
    if (now - lastBcast >= 10000) {
      lastBcast = now;
      otDeviceRole role = otThreadGetDeviceRole(sInstance);
      if (role == OT_DEVICE_ROLE_LEADER || role == OT_DEVICE_ROLE_ROUTER) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Hello from %04X! Up:%lu s",
          otThreadGetRloc16(sInstance), now / 1000);
        sendThreadBroadcast(msg);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
