// Hardware boundaries for the host simulation. Never included in firmware.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#define PI 3.14159265358979323846f
#define PI2 6.28318530718f
#define IRAM_ATTR
#define MALLOC_CAP_SPIRAM 0
#define MALLOC_CAP_INTERNAL 0
#define MALLOC_CAP_8BIT 0
#define FRAME_MS 33
#define WORLD_W 1600
#define WORLD_H 960
#define HALF_W 800
#define HALF_H 480
static float simTime = 0;
static uint32_t frameCount = 0;
static void* heap_caps_malloc(size_t n, int) { return std::malloc(n); }
static void heap_caps_free(void* p) { std::free(p); }
static uint32_t millis() { return uint32_t(simTime * 1000); }
static uint32_t esp_random() { static uint32_t x = 10; return ++x * 2654435761u; }
struct SerialStub {
  template<class... A> void printf(const char*, A...) {}
  void println(const char*) {}
} Serial;
static int currentWeather = WEATHER_SUNNY;
enum PType : uint8_t { PT_DUST, PT_SMOKE, PT_SPRAY, PT_GRASS, PT_FLASH, PT_SPARK };
template<class... A> static void spawnParticle(A...) {}
static void initEffects() {}
static void replayReset() {}
static void replayRecord() {}
template<class... A> static void replayRequest(A...) {}
static void replayNoteLeader(int) {}
static void replayCheckLeader(int) {}
#define SETTINGS_NS "race"
struct PreferencesStub {
  std::map<std::string, std::vector<uint8_t>> data;
  void begin(const char*, bool) {}
  void end() {}
  size_t putBytes(const char* key, const void* p, size_t n) { data[key] = std::vector<uint8_t>((const uint8_t*)p, (const uint8_t*)p+n); return n; }
  size_t getBytesLength(const char* key) { return data[key].size(); }
  size_t getBytes(const char* key, void* p, size_t n) { auto& d=data[key]; size_t k=std::min(n,d.size()); std::memcpy(p,d.data(),k); return k; }
} prefs;
