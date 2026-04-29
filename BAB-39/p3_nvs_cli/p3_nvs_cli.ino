/*
 * BAB 39 - Program 3: Interactive Settings Manager via Serial CLI
 *
 * Perintah yang tersedia:
 *   get         → Tampilkan semua pengaturan saat ini
 *   set bright <0-100> → Ubah brightness
 *   set temp <nilai>   → Ubah threshold suhu
 *   set alarm <on/off> → Aktif/nonaktifkan alarm
 *   set name <teks>    → Ubah nama perangkat
 *   reset       → Hapus NVS dan kembali ke default
 *   help        → Tampilkan daftar perintah
 *
 * Hardware: Hanya ESP32
 * Buka Serial Monitor dengan Line Ending: "Newline" atau "Both NL & CR"
 */

#include <Preferences.h>
#include <string.h>
#include <stdlib.h>

Preferences prefs;

#define NS "cfg"

// ── Struktur pengaturan ───────────────────────────────────────────
uint8_t  cfgBright   = 75;
float    cfgTemp     = 35.0f;
bool     cfgAlarm    = true;
char     cfgName[16] = "Bluino-IoT"; // Maks 15 karakter + null terminator

// ── Write-on-Change tracker (Debounce NVS Wear-Leveling) ──────────
bool settingsChanged = false;
unsigned long tLastChange = 0;
const unsigned long SAVE_DEBOUNCE_MS = 2000UL;

void loadSettings() {
  prefs.begin(NS, true);
  cfgBright = prefs.getUChar ("bright",  75);
  cfgTemp   = prefs.getFloat ("temp",    35.0f);
  cfgAlarm  = prefs.getBool  ("alarm",   true);
  
  String nameLoaded = prefs.getString("name", "Bluino-IoT");
  strncpy(cfgName, nameLoaded.c_str(), sizeof(cfgName) - 1);
  cfgName[sizeof(cfgName) - 1] = '\0';
  
  prefs.end();
}

void saveSettings() {
  prefs.begin(NS, false);
  prefs.putUChar ("bright", cfgBright);
  prefs.putFloat ("temp",   cfgTemp);
  prefs.putBool  ("alarm",  cfgAlarm);
  prefs.putString("name",   cfgName);
  prefs.end();
  
  settingsChanged = false;
  Serial.println("[NVS] ✅ Pengaturan disimpan ke Flash.");
}

void resetSettings() {
  prefs.begin(NS, false);
  prefs.clear();
  prefs.end();
  
  cfgBright = 75;
  cfgTemp   = 35.0f;
  cfgAlarm  = true;
  strncpy(cfgName, "Bluino-IoT", sizeof(cfgName));
  
  Serial.println("[NVS] ⚠️  NVS dihapus. Nilai kembali ke default.");
}

void printSettings() {
  Serial.println();
  Serial.println("┌──────────────────────────────────────┐");
  Serial.println("│         Pengaturan Saat Ini           │");
  Serial.println("├──────────────────────────────────────┤");
  Serial.printf ("│  brightness : %d%%\n",             cfgBright);
  Serial.printf ("│  temp_alarm : %.1f °C\n",           cfgTemp);
  Serial.printf ("│  alarm      : %s\n",               cfgAlarm ? "ON" : "OFF");
  Serial.printf ("│  name       : %s\n",               cfgName);
  Serial.println("└──────────────────────────────────────┘");
  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("=== Perintah CLI NVS ===");
  Serial.println("  get                → Tampilkan semua pengaturan");
  Serial.println("  set bright <0-100> → Ubah brightness");
  Serial.println("  set temp <nilai>   → Ubah threshold suhu (°C)");
  Serial.println("  set alarm <on/off> → Aktif/nonaktifkan alarm");
  Serial.println("  set name <teks>    → Ubah nama (maks 15 karakter)");
  Serial.println("  reset              → Hapus NVS dan kembali ke default");
  Serial.println("  help               → Tampilkan panduan ini");
  Serial.println();
}

// ── Parser Zero-Heap (Berbasis pointer C-String murni) ────────────
void handleCommand(char* cmd) {
  // Hapus spasi di akhir jika ada (trim right)
  int len = strlen(cmd);
  while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\r')) {
    cmd[len - 1] = '\0';
    len--;
  }
  
  if (len == 0) return;
  Serial.printf("> %s\n", cmd);

  if (strcmp(cmd, "get") == 0) {
    printSettings();

  } else if (strncmp(cmd, "set bright ", 11) == 0) {
    int val = atoi(cmd + 11);
    if (val < 0 || val > 100) {
      Serial.println("[ERR] Brightness harus antara 0–100");
      return;
    }
    cfgBright = (uint8_t)val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] brightness = %d%%\n", cfgBright);

  } else if (strncmp(cmd, "set temp ", 9) == 0) {
    float val = atof(cmd + 9);
    if (val < -40.0f || val > 125.0f) {
      Serial.println("[ERR] Suhu harus antara -40 s/d 125 °C");
      return;
    }
    cfgTemp = val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] temp_alarm = %.1f °C\n", cfgTemp);

  } else if (strncmp(cmd, "set alarm ", 10) == 0) {
    char* val = cmd + 10;
    if (strcasecmp(val, "on") == 0) {
      cfgAlarm = true;
    } else if (strcasecmp(val, "off") == 0) {
      cfgAlarm = false;
    } else {
      Serial.println("[ERR] Gunakan 'on' atau 'off'");
      return;
    }
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] alarm = %s\n", cfgAlarm ? "ON" : "OFF");

  } else if (strncmp(cmd, "set name ", 9) == 0) {
    char* val = cmd + 9;
    if (strlen(val) == 0 || strlen(val) > 15) {
      Serial.println("[ERR] Nama harus 1–15 karakter");
      return;
    }
    strncpy(cfgName, val, sizeof(cfgName) - 1);
    cfgName[sizeof(cfgName) - 1] = '\0';
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] name = %s\n", cfgName);

  } else if (strcmp(cmd, "reset") == 0) {
    resetSettings();

  } else if (strcmp(cmd, "help") == 0) {
    printHelp();

  } else {
    Serial.printf("[ERR] Perintah tidak dikenal: '%s'. Ketik 'help'.\n", cmd);
  }
}

// ── Buffer Serial Zero-Heap ───────────────────────────────────────
char serialBuf[64];
size_t bufIdx = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  loadSettings();

  Serial.println("=== BAB 39 Program 3: NVS Serial CLI ===");
  Serial.println("Ketik 'help' untuk melihat daftar perintah.");
  Serial.println("Pengaturan disimpan otomatis 2 detik setelah perubahan.");
  printSettings();
}

void loop() {
  unsigned long now = millis();

  // ── Baca input Serial (Non-blocking, anti fragmentasi RAM) ──────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufIdx > 0) {
        serialBuf[bufIdx] = '\0'; // Tutup string
        handleCommand(serialBuf);
        bufIdx = 0; // Reset buffer setelah perintah diproses
      }
    } else {
      if (bufIdx < sizeof(serialBuf) - 1) {
        serialBuf[bufIdx++] = c;
      }
    }
  }

  // ── Auto-save dengan debounce ────────────────────────────────────
  if (settingsChanged && (now - tLastChange >= SAVE_DEBOUNCE_MS)) {
    saveSettings();
  }
}
