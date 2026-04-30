/*
 * BAB 56 - Program 2: Matter Light Bulb — REAL MATTER
 *
 * PLATFORM: ESP32-H2 atau ESP32-C6 SAJA!
 * BUKAN untuk ESP32 Classic!
 *
 * Development Environment:
 *   - ESP-IDF v5.1.2+
 *   - esp-matter v1.3+
 *   - Build: idf.py set-target esp32h2 && idf.py build && idf.py flash
 *
 * Setelah flash:
 *   1. Buka Google Home / Apple Home
 *   2. Tambah perangkat baru
 *   3. Scan QR Code yang tampil di Serial Monitor
 *   4. Ikuti alur commissioning
 *   5. Relay bisa dikontrol dari aplikasi!
 *
 * Hardware: Relay/LED → IO4
 */

// ── Includes esp-matter SDK ────────────────────────────────────────────
#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>
#include <esp_log.h>
#include <driver/gpio.h>

static const char *TAG = "BAB56_P2";

// ── Konfigurasi Pin ────────────────────────────────────────────────────
#define RELAY_GPIO  GPIO_NUM_4

// ── Matter Node & Endpoint handle ─────────────────────────────────────
static esp_matter::node::config_t  node_cfg;
static esp_matter::endpoint::on_off_light::config_t light_cfg;
static uint16_t light_endpoint_id = 0;

// ── Callback: dipanggil saat attribute berubah (dari Commissioner) ─────
static esp_err_t app_attribute_update_cb(
    esp_matter::attribute::callback_type_t type,
    uint16_t endpoint_id, uint32_t cluster_id,
    uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
  // Filter: hanya proses OnOff cluster (0x0006), attribute OnOff (0x0000)
  if (endpoint_id == light_endpoint_id
      && cluster_id    == chip::app::Clusters::OnOff::Id
      && attribute_id  == chip::app::Clusters::OnOff::Attributes::OnOff::Id
      && type          == esp_matter::attribute::PRE_UPDATE)
  {
    bool new_state = val->val.b;
    gpio_set_level(RELAY_GPIO, new_state ? 1 : 0);
    ESP_LOGI(TAG, "OnOff → %s", new_state ? "ON" : "OFF");
  }
  return ESP_OK;
}

// ── Callback: device events (commissioning, connectivity) ─────────────
static void app_event_cb(
    const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg)
{
  switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
      ESP_LOGI(TAG, "✅ Commissioning selesai! Device tergabung ke Fabric.");
      break;
    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
      ESP_LOGW(TAG, "⚠️ Device dilepas dari Fabric.");
      break;
    default: break;
  }
}

extern "C" void app_main() {
  // ── Init GPIO ──────────────────────────────────────────────────────
  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << RELAY_GPIO),
    .mode         = GPIO_MODE_OUTPUT,
  };
  gpio_config(&io_conf);
  gpio_set_level(RELAY_GPIO, 0);  // Mulai OFF

  ESP_LOGI(TAG, "\n=== BAB 56 Program 2: Matter Light Bulb ===");

  // ── Buat Matter Node ───────────────────────────────────────────────
  esp_matter::node_t *node = esp_matter::node::create(
    &node_cfg, app_attribute_update_cb, app_event_cb);

  if (!node) {
    ESP_LOGE(TAG, "Gagal membuat Matter node!");
    return;
  }

  // ── Buat Endpoint: On/Off Light ────────────────────────────────────
  light_cfg.on_off.on_off            = false; // Default: OFF
  light_cfg.on_off.lighting.start_up_on_off = 0;

  esp_matter::endpoint_t *endpoint = esp_matter::endpoint::on_off_light::create(
    node, &light_cfg, ENDPOINT_FLAG_NONE, NULL);

  light_endpoint_id = esp_matter::endpoint::get_id(endpoint);
  ESP_LOGI(TAG, "OnOff Light Endpoint ID: %u", light_endpoint_id);

  // ── Start Matter Stack ─────────────────────────────────────────────
  esp_matter::start(app_event_cb);

  // ── Tampilkan QR Code Commissioning di Serial ──────────────────────
  // (esp-matter otomatis print QR code saat startup)
  ESP_LOGI(TAG, "✅ Matter device aktif! Scan QR Code di atas untuk commissioning.");
  ESP_LOGI(TAG, "Buka Google Home / Apple Home → Tambah Perangkat → Scan QR Code");
}
