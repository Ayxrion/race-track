#pragma once
// Decision making only: none of these states grants engine power or tyre grip.
// Included by cars.h after Car, career helpers and the roster are defined.
enum BattlePhase : uint8_t { BP_FOLLOW, BP_PREPARE, BP_COMMIT, BP_ALONGSIDE, BP_RECOVER };

// Separate each driver's random stream from rendering, other drivers and loop order.
struct DriverRandomScope {
  uint32_t& state; uint32_t saved;
  explicit DriverRandomScope(uint32_t& s) : state(s), saved(_rng) { _rng = s ? s : 1; }
  ~DriverRandomScope() { state = _rng; _rng = saved; }
};

static float driverTrait(const char* code, uint32_t salt) {
  return (hash32(codeHash(code) ^ salt) >> 8) * (1.0f / 16777216.0f);
}

static void setupRacecraft(Car& c, uint32_t seed) {
  c.aiRng = hash32(codeHash(c.code) ^ seed ^ 0x7625u) | 1u;
  c.aggression = driverTrait(c.code, 11);
  c.tyreCare = driverTrait(c.code, 23);
  const float wet = driverTrait(c.code, 37), precision = driverTrait(c.code, 41);
  c.driver = DRV;
  c.driver.brake = 0.97f + 0.06f * c.aggression;
  c.driver.corner = 0.99f + 0.02f * precision;
  c.driver.wetAdapt = 0.94f + 0.06f * wet;
  c.driver.sigma = 0.018f + 0.012f * (1.0f - precision);
  c.driver.passMargin = 0.8f + 1.0f * (1.0f - c.aggression);
  c.driver.passGap = 2.6f + 0.8f * c.aggression;
  c.driver.passRoom = 0.6f * (1.0f - c.aggression);
  c.driver.mistake = 0.85f + 0.3f * c.aggression;
  c.stApex = -0.35f + 0.7f * driverTrait(c.code, 53);
  c.stKerb = -0.55f + 0.85f * driverTrait(c.code, 67);
  c.stIn = -0.35f + 0.7f * driverTrait(c.code, 79);
  c.stOut = -0.35f + 0.7f * driverTrait(c.code, 83);
  c.stVar = 0.4f + 0.4f * (1.0f - precision);
  c.passCar = c.prepCar = c.lastPassCar = -1;
  c.cutbackCorner = -1;
  c.battlePhase = BP_FOLLOW;
}

static void endAttack(Car& c) {
  if (c.passCar >= 0) { c.lastPassCar = c.passCar; c.pairUntil = raceTime + 4.0f; }
  c.passT = c.passAge = c.clearT = c.prepareT = 0;
  c.passCar = c.prepCar = -1;
  c.passSide = c.outMove = 0;
  c.passCd = fmx(c.passCd, 1.25f);
  c.battlePhase = BP_RECOVER;
}

static bool pairReady(const Car& c, int i, const Car& other, int j) {
  return !(c.lastPassCar == j && c.pairUntil > raceTime) &&
         !(other.lastPassCar == i && other.pairUntil > raceTime);
}

// Aero assistance is available only in a real wake, never while overlapping.
static float wakeStrength(float gap, float lateral, float halfLengths, float halfWidths, float speed) {
  if (gap <= halfLengths + 1.0f || fabsf(lateral) >= halfWidths || speed < 35.0f) return 0;
  const float t = gap / fmx(speed, 60.0f);
  return clampf((1.2f - t) / 0.7f, 0, 1) * (1.0f - fabsf(lateral) / halfWidths);
}

// A bounded estimate of a flying lap using this circuit's curvature, braking,
// acceleration, class, driver and current tyre/weather grip. No position input.
static float qualifyingLap(const Car& c) {
  float* speed = _tmpA; // track-generation scratch, reused only during grid setup
  const ClassSpec& cs = CLS[c.cls];
  const float wx = (c.wets ? 1.0f - WET_WETS0 - (WET_WETS1 - WET_WETS0) * wetAmt : 1.0f - WET_SLICK * wetAmt) * (1.0f - 0.14f * snowCover);
  const float grip = cs.grip * c.perf * c.perf * powf(wx, c.driver.wetAdapt) * c.driver.corner;
  const float top = V_TOP * cs.top * c.perf;
  const float brake = A_BRK * c.driver.brake * sqrtf(cs.grip) * c.perf * wx;
  const float accel = A_ACC * cs.acc * c.perf * wx * 0.7f;
  for (int k = 0; k < trk.n; ++k) speed[k] = fmn(top, tp[k].vc * sqrtf(grip));
  // Two circuits settle the start/finish boundary as well as the braking zones.
  for (int k = 2 * trk.n - 1; k >= 0; --k) {
    const int i = k % trk.n, j = (i + 1) % trk.n;
    speed[i] = fmn(speed[i], sqrtf(speed[j] * speed[j] + 2.0f * brake * trk.ds));
  }
  for (int k = 0; k < 2 * trk.n; ++k) {
    const int i = k % trk.n, j = (i + 1) % trk.n;
    speed[j] = fmn(speed[j], sqrtf(speed[i] * speed[i] + 2.0f * accel * trk.ds));
  }
  float lap = 0;
  for (int i = 0; i < trk.n; ++i) lap += 2.0f * trk.ds / fmx(speed[i] + speed[(i + 1) % trk.n], 2.0f);
  return lap;
}
