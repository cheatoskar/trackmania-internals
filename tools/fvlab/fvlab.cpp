// tm-physics-re lab: input sweeps on top of the experimental physics sandbox.
//
// fvlab PACKS REPLAY seam T0_MS T_STEER_FROM_MS T_STEER_TO_MS HORIZON_TICKS
//   Restores the replay state at T0 - (T0 - T_STEER_FROM) and replaces the steering between
//   T_STEER_FROM and T_STEER_TO with a constant value from a sweep, then reports for each value the
//   maximum nose-down pitch angle, peak pitch/roll rates (car space) and the speed after HORIZON ticks.
// fvlab PACKS REPLAY probe T0_MS TICKS
//   Prints the state every tick from T0 for TICKS ticks (position, speed, pitch/roll, wheel contacts).

#include <forevervalidator/experimental/physics_sandbox.h>
#include <forevervalidator/input_state.h>
#include <forevervalidator/native.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

using namespace forevervalidator;
using namespace forevervalidator::experimental;

namespace tmphys_probe {  // defined in engine/scene/scene_vehicle_car_handling.cpp
extern float maxExponent;
extern float maxMagnitude;
extern unsigned donutPulls;
}  // namespace tmphys_probe

namespace {

struct Mat3 {
    float m[3][3];
};

Mat3 FromQuat(float x, float y, float z, float w) {
    Mat3 r{};
    r.m[0][0] = 1 - 2 * (y * y + z * z);
    r.m[0][1] = 2 * (x * y - z * w);
    r.m[0][2] = 2 * (x * z + y * w);
    r.m[1][0] = 2 * (x * y + z * w);
    r.m[1][1] = 1 - 2 * (x * x + z * z);
    r.m[1][2] = 2 * (y * z - x * w);
    r.m[2][0] = 2 * (x * z - y * w);
    r.m[2][1] = 2 * (y * z + x * w);
    r.m[2][2] = 1 - 2 * (x * x + y * y);
    return r;
}

struct Pose {
    float pitchDeg;  // nose down positive
    float rollDeg;
    float localW[3];  // angular speed in car space
};

Pose PoseOf(const PhysicsSandboxCarState &c) {
    Mat3 r = FromQuat(c.rotationX, c.rotationY, c.rotationZ, c.rotationW);
    // columns: car x (left), y (up), z (forward) in world
    float fwdY = r.m[1][2];
    float leftY = r.m[1][0];
    Pose p{};
    p.pitchDeg = std::asin(std::clamp(-fwdY, -1.0f, 1.0f)) * 57.29578f;
    p.rollDeg = std::asin(std::clamp(leftY, -1.0f, 1.0f)) * 57.29578f;
    const Vector3 &w = c.angularSpeed;
    for (int i = 0; i < 3; ++i) {
        p.localW[i] = r.m[0][i] * w.x + r.m[1][i] * w.y + r.m[2][i] * w.z;
    }
    return p;
}

float Speed(const PhysicsSandboxCarState &c) {
    return std::sqrt(c.linearSpeed.x * c.linearSpeed.x + c.linearSpeed.y * c.linearSpeed.y +
                     c.linearSpeed.z * c.linearSpeed.z);
}

PhysicsSandboxInputEvent Steer(std::int32_t timeMs, AnalogInputState value) {
    PhysicsSandboxInputEvent e;
    e.timeMs = timeMs;
    e.action = PhysicsSandboxInputAction::Steer;
    e.value.kind = PhysicsSandboxInputValueKind::Analog;
    e.value.analog = value;
    return e;
}

// Time-sorted events: keep only the last event per (time, action). Two events of one action on the same
// millisecond are ambiguous: the search and a reload from CSV (or TMInterface) may apply a different one.
void KeepLastPerTick(std::vector<PhysicsSandboxInputEvent> &events) {
    std::vector<PhysicsSandboxInputEvent> kept;
    kept.reserve(events.size());
    for (const auto &e : events) {
        if (!kept.empty() && kept.back().timeMs == e.timeMs) {
            auto it = std::find_if(kept.rbegin(), kept.rend(), [&](const auto &k) {
                return k.timeMs != e.timeMs || k.action == e.action;
            });
            if (it != kept.rend() && it->timeMs == e.timeMs && it->action == e.action) {
                *it = e;
                continue;
            }
        }
        kept.push_back(e);
    }
    events = std::move(kept);
}

int Fail(const char *what) {
    std::fprintf(stderr, "fvlab: %s\n", what);
    return 1;
}

}  // namespace

int main(int argc, char **argv) {
    if (argc < 4) {
        return Fail("usage: fvlab PACKS REPLAY seam|probe ...");
    }
    std::ifstream in(argv[2], std::ios::binary);
    std::vector<std::uint8_t> replay((std::istreambuf_iterator<char>(in)), {});
    if (replay.empty()) return Fail("cannot read replay");
    auto source = OpenInstalledPackDirectory(argv[1]);
    if (!source) return Fail("cannot open packs");
    PhysicsSandboxOptions options;
    options.backend = std::getenv("FVLAB_OPTIMIZED") != nullptr ? SimulationBackend::OptimizedCpu
                                                                : SimulationBackend::Reference;
    auto created = CreatePhysicsSandbox(std::move(source).Value(), options);
    if (!created) return Fail("cannot create sandbox");
    PhysicsSandbox sandbox = std::move(created).Value();
    ByteView view{reinterpret_cast<const std::byte *>(replay.data()), replay.size()};
    auto loaded = sandbox.LoadReplay(view, ReplayIdentity{argv[2]});
    if (!loaded) {
        std::fprintf(stderr, "%s\n", loaded.Error().diagnostic.c_str());
        return Fail("cannot load replay");
    }
    const std::string mode = argv[3];

    // FVLAB_INPUTS=<csv from "inputs" mode or opt output>: replace the replay's inputs before anything else.
    if (const char *inputsPath = std::getenv("FVLAB_INPUTS")) {
        std::ifstream f(inputsPath);
        std::string line;
        std::vector<PhysicsSandboxInputEvent> events;
        std::getline(f, line);
        while (std::getline(f, line)) {
            int t, a, k, sw, an;
            if (std::sscanf(line.c_str(), "%d,%d,%d,%d,%d", &t, &a, &k, &sw, &an) != 5) continue;
            PhysicsSandboxInputEvent e;
            e.timeMs = t;
            e.action = static_cast<PhysicsSandboxInputAction>(a);
            e.value.kind = static_cast<PhysicsSandboxInputValueKind>(k);
            e.value.switchState = static_cast<PhysicsSandboxSwitchState>(sw);
            e.value.analog = an;
            events.push_back(e);
        }
        auto replaced = sandbox.ReplaceInputs(std::move(events));
        if (!replaced) {
            std::fprintf(stderr, "%s\n", replaced.Error().diagnostic.c_str());
            return Fail("FVLAB_INPUTS rejected");
        }
    }

    auto advanceTo = [&](std::uint64_t timeMs) -> bool {
        for (;;) {
            auto s = sandbox.ReadState();
            if (!s) return false;
            if (s.Value().timeMs >= timeMs) return true;
            if (!sandbox.AdvanceTicks(1)) return false;
        }
    };

    // fvlab PACKS REPLAY opt SEED FROM_MS TO_MS SEGMENTS SECONDS OUT.csv
    //   Local search on the steering between FROM and TO for the earliest precise finish time.
    //   The window is cut into SEGMENTS; each attempt mutates the steer events of one segment of the
    //   current best (restoring the cached state at that segment's start) and keeps it if the finish
    //   is earlier. Writes the best inputs (inputs-mode CSV) to OUT.csv on every improvement.
    // fvlab PACKS REPLAY uber SEED FROM_MS TO_MS SECONDS OUT.csv HORIZON_MS FX FY FZ
    //   Search for an uber towards a target point (finish). Mutates steer, gas and brake events in
    //   [FROM, TO) of the current best and simulates until TO + HORIZON or the finish. Score (lower is
    //   better): finished -> finish time - 1e6; otherwise the closest distance to (FX,FY,FZ) reached,
    //   minus 0.1 per km/h of peak speed (capped at 1000) and 5 per unit of the largest donut-pull exponent (capped at 6),
    //   so ubers and near-ubers are rewarded before they point at the target.
    if (mode == "uber" && argc >= 13) {
        std::uint32_t rng = static_cast<std::uint32_t>(std::strtoul(argv[4], nullptr, 10)) * 2654435761u + 777u;
        auto next = [&rng]() {
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            return rng;
        };
        auto uniform = [&](int lo, int hi) { return lo + static_cast<int>(next() % static_cast<std::uint32_t>(hi - lo + 1)); };
        const std::int32_t from = std::atoi(argv[5]) / 10 * 10;
        const std::int32_t to = std::atoi(argv[6]) / 10 * 10;
        const double seconds = std::atof(argv[7]);
        const std::string outPath = argv[8];
        const std::int32_t horizon = std::atoi(argv[9]);
        const float fx = std::strtof(argv[10], nullptr), fy = std::strtof(argv[11], nullptr), fz = std::strtof(argv[12], nullptr);
        auto isControl = [](PhysicsSandboxInputAction a) {
            return a == PhysicsSandboxInputAction::Steer || a == PhysicsSandboxInputAction::Brake ||
                   a == PhysicsSandboxInputAction::Accelerate || a == PhysicsSandboxInputAction::Gas;
        };
        auto base = sandbox.ReadInputs();
        if (!base) return Fail("inputs");
        std::vector<PhysicsSandboxInputEvent> fixed, best;
        for (const auto &e : base.Value()) {
            // events exactly at `from` stay fixed: the captured start state has already applied that tick
            if (isControl(e.action) && e.timeMs > from && e.timeMs < to) best.push_back(e);
            else fixed.push_back(e);
        }
        if (!advanceTo(static_cast<std::uint64_t>(from))) return Fail("advance");
        auto start = sandbox.CaptureState();
        if (!start) return Fail("capture");
        // FVLAB_OBJECTIVE=nose: score the first landing after FVLAB_LAND_MIN ms (default TO): pitch error to
        // 90° nose-down plus 30 per rear wheel on the ground, minus 0.5 per km/h gained in the 300 ms after it.
        // FVLAB_RTA=1: keyboard steering only (-65536/0/65536) and >= 50 ms between events of one action.
        // FVLAB_OBJECTIVE=noselow: lowest landing speed at the first front-wheel landing after FVLAB_LAND_MIN
        // from which the car still reaches 950 km/h within 600 ms (a noseboost); runs without a boost score
        // 10000 + landing speed - gain, so only boosting runs can replace a boosting best.
        const bool noseLow = std::getenv("FVLAB_OBJECTIVE") != nullptr && std::string(std::getenv("FVLAB_OBJECTIVE")) == "noselow";
        const bool noseObjective = noseLow || (std::getenv("FVLAB_OBJECTIVE") != nullptr && std::string(std::getenv("FVLAB_OBJECTIVE")) == "nose");
        const bool rta = std::getenv("FVLAB_RTA") != nullptr;
        const bool noseposObjective = std::getenv("FVLAB_OBJECTIVE") != nullptr && std::string(std::getenv("FVLAB_OBJECTIVE")) == "nosepos";
        // FVLAB_OBJECTIVE=chain: a nose chain from trigger to trigger. FVLAB_WAYPOINTS="x,z,r;x,z,r;..." are reached
        // in order (horizontal distance < r). From FVLAB_CHAIN_FROM (default `from`) every tick off the nose (a rear
        // wheel down or the nose < 45 deg down) costs FVLAB_CHAIN_OFF_W (default 1), every km/h below
        // FVLAB_CHAIN_MINKMH (default 0) costs 0.01 per tick. Score: 1000 per waypoint left + metres to the next one
        // + those penalties; with all reached, the time of the last one (1 per 10 ms) instead of the distance.
        const bool chainObjective = std::getenv("FVLAB_OBJECTIVE") != nullptr && std::string(std::getenv("FVLAB_OBJECTIVE")) == "chain";
        static const std::vector<std::array<float, 3>> chainWps = [] {
            std::vector<std::array<float, 3>> w;
            if (const char *s = std::getenv("FVLAB_WAYPOINTS")) {
                std::string str(s);
                std::size_t p = 0;
                while (p < str.size()) {
                    std::array<float, 3> a{};
                    if (std::sscanf(str.c_str() + p, "%f,%f,%f", &a[0], &a[1], &a[2]) != 3) break;
                    w.push_back(a);
                    p = str.find(';', p);
                    if (p == std::string::npos) break;
                    ++p;
                }
            }
            return w;
        }();
        const std::int64_t chainFrom = std::getenv("FVLAB_CHAIN_FROM") ? std::atoll(std::getenv("FVLAB_CHAIN_FROM")) : from;
        static const float chainOffW = std::getenv("FVLAB_CHAIN_OFF_W") ? std::strtof(std::getenv("FVLAB_CHAIN_OFF_W"), nullptr) : 1.0f;
        static const float chainMinKmh = std::getenv("FVLAB_CHAIN_MINKMH") ? std::strtof(std::getenv("FVLAB_CHAIN_MINKMH"), nullptr) : 0.0f;
        const std::int64_t evalFrom = std::getenv("FVLAB_EVAL_FROM") ? std::atoll(std::getenv("FVLAB_EVAL_FROM")) : to;
        const std::int64_t evalTo = std::getenv("FVLAB_EVAL_TO") ? std::atoll(std::getenv("FVLAB_EVAL_TO")) : to + horizon;
        // FVLAB_BOOST_GAIN=<km/h>: for noselow, a boost counts when the speed rises by this much within 600 ms
        // (default: reaching 950 km/h)
        const float boostGain = std::getenv("FVLAB_BOOST_GAIN") ? std::strtof(std::getenv("FVLAB_BOOST_GAIN"), nullptr) : 0.0f;
        const std::int64_t landMin = std::getenv("FVLAB_LAND_MIN") ? std::atoll(std::getenv("FVLAB_LAND_MIN")) : to;
        struct Score { double value; double finishMs; float minDist; float maxExp; float maxKmh; float noseErr; float noseGain; };
        auto run = [&](const std::vector<PhysicsSandboxInputEvent> &cand) -> Score {
            Score sc{1e30, 0, 1e30f, -1e30f, 0, 1e9f, 0};
            float rawMaxKmh = 0.0f;  // unfiltered peak speed (FVLAB_RAW_KMH_WEIGHT)
            int wheelieRun = 0, wheelieBest = 0;  // FVLAB_WHEELIE_WEIGHT
            if (!sandbox.RestoreState(start.Value())) return sc;
            // (from, to): the tick at `from` is already applied in the start state, a base event at `to` must stay
            if (!sandbox.ReplaceInputWindow(from + 10, to - 10, cand)) return sc;
            tmphys_probe::maxExponent = -1e30f;
            tmphys_probe::maxMagnitude = 0.0f;
            std::int64_t landedAt = -1;
            float landKmh = 0;
            float noseLoss = 0;
            int holdRun = 0, holdBest = 0;
            int landRun = 0;  // FVLAB_LAND_BOX: consecutive ticks with >= 2 wheels down inside the box
            bool goalHit = false;
            unsigned prevCps = 0;
            std::vector<float> recentY;  // car y over the last 500 ms (FVLAB_GOAL_MAXY)
            bool stageFail = false;      // FVLAB_STAGE: the staged CP was taken elsewhere or the wrong way
            bool goalScored = false;     // FVLAB_GOAL_CPS: the goal tick was scored
            // starts at the CP count of the first simulated tick: a window that opens after the staged CP (CP n at
            // 23.05 s, inputs from 23.2 s) must not check that CP again at the wrong place
            unsigned stagePrevCps = ~0u;
            float noseposBest = 5000.0f;
            float lastKmh = 0.0f;  // speed at the last simulated tick (FVLAB_NOSE_NOLAND_KMH)
            // FVLAB_SPIKE_FILTER=n: a tick's speed counts as the minimum over this tick and the n ticks before it
            // (n = 1: the old two-tick filter; a kerb uber holds its speed for many ticks, a contact spike does not)
            static const int spikeTicks = std::getenv("FVLAB_SPIKE_FILTER") ? std::max(0, std::atoi(std::getenv("FVLAB_SPIKE_FILTER"))) : 0;
            std::vector<float> recentKmh;
            auto filteredKmh = [&](float cur) {
                for (float k : recentKmh) cur = std::min(cur, k);
                return cur;
            };
            std::int64_t boostAt = -1;  // first tick at FVLAB_BOOST_KMH (FVLAB_BOOST_BOX / FVLAB_BOOST_DIR)
            float boostPenalty = 0.0f;
            float viaMin = 1e9f;  // FVLAB_VIA: closest distance to the guide box (from FVLAB_BOOST_FROM on)
            std::size_t chainNext = 0;  // FVLAB_OBJECTIVE=chain: next waypoint, penalties, time of the last one
            float chainPenalty = 0.0f;
            std::int64_t chainDoneAt = -1;
            float chainDist = 1e9f;
            unsigned pullsAtLanding = 0;
            static const float targetMinKmh = std::getenv("FVLAB_TARGET_MINKMH") ? std::strtof(std::getenv("FVLAB_TARGET_MINKMH"), nullptr) : 0.0f;
            static const unsigned needCps =std::getenv("FVLAB_NEED_CPS") ? static_cast<unsigned>(std::atoi(std::getenv("FVLAB_NEED_CPS"))) : 0u;
            bool airborne = false;
            int airTicks = 0;  // gradient while no landing exists yet: more air time = closer to a flight
            static const float noseMinKmh = std::getenv("FVLAB_NOSE_MINKMH") ? std::strtof(std::getenv("FVLAB_NOSE_MINKMH"), nullptr) : 0.0f;
            // FVLAB_BOOST_BOX=x,y,z,sx,sy,sz (TMInterface corner + size): the first tick at FVLAB_BOOST_KMH
            // (default 600) must be in this box, + 3 per metre outside (where on the road the noseboost fires).
            // FVLAB_BOOST_DIR=w: 100 ms after that tick, + w per degree between the horizontal velocity and the
            // direction to the target point (aim the boost; the distance alone has no gradient while a small
            // input change can kill the boost). Without a boost: + 500. Both add to the uber score.
            static const std::vector<float> boostBox = [] {
                std::vector<float> b(6);
                const char *s = std::getenv("FVLAB_BOOST_BOX");
                if (!s || std::sscanf(s, "%f,%f,%f,%f,%f,%f", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) return std::vector<float>{};
                return b;
            }();
            static const float boostDirW = std::getenv("FVLAB_BOOST_DIR") ? std::strtof(std::getenv("FVLAB_BOOST_DIR"), nullptr) : 0.0f;
            static const float boostKmh = std::getenv("FVLAB_BOOST_KMH") ? std::strtof(std::getenv("FVLAB_BOOST_KMH"), nullptr) : 600.0f;
            // FVLAB_BOOST_BOX_W: penalty per metre outside the box (default 3; raise it above a CP goal's 1000)
            static const float boostBoxW = std::getenv("FVLAB_BOOST_BOX_W") ? std::strtof(std::getenv("FVLAB_BOOST_BOX_W"), nullptr) : 3.0f;
            // FVLAB_EXP_MINKMH=k: the near-uber exponent of a tick counts only while the car has >= k km/h, so a crash
            // or slow crawling contacts (which also raise the exponent) do not rank as uber set-ups
            static const float expMinKmh = std::getenv("FVLAB_EXP_MINKMH") ? std::strtof(std::getenv("FVLAB_EXP_MINKMH"), nullptr) : 0.0f;
            float gatedExp = -1e30f;
            // FVLAB_EXP_KEEP_MS=m (with FVLAB_EXP_MINKMH): a gated exponent counts only if m ms later the car is at least
            // as fast as before that tick; a hard impact raises the exponent as well but costs speed, an uber gains it
            static const int expKeepMs = std::getenv("FVLAB_EXP_KEEP_MS") ? std::atoi(std::getenv("FVLAB_EXP_KEEP_MS")) : 0;
            struct PendingExp { std::int64_t due; float exp; float kmhBefore; };
            std::vector<PendingExp> pendingExp;
            for (;;) {
                auto st = sandbox.ReadState();
                if (!st) return sc;
                const auto &v = st.Value();
                if (expMinKmh > 0.0f) {
                    const float gateKmh = filteredKmh(Speed(v.car) * 3.6f);
                    const std::int64_t tnow = static_cast<std::int64_t>(v.timeMs);
                    const float kmhNow = Speed(v.car) * 3.6f;
                    if (expKeepMs > 0) {
                        for (auto it = pendingExp.begin(); it != pendingExp.end();) {
                            if (tnow >= it->due) {
                                if (kmhNow >= it->kmhBefore) gatedExp = std::max(gatedExp, it->exp);
                                it = pendingExp.erase(it);
                            } else {
                                ++it;
                            }
                        }
                        if (gateKmh >= expMinKmh && tmphys_probe::maxExponent > -1e29f)
                            pendingExp.push_back({tnow + expKeepMs, tmphys_probe::maxExponent, lastKmh});
                    } else if (gateKmh >= expMinKmh) {
                        gatedExp = std::max(gatedExp, tmphys_probe::maxExponent);
                    }
                    tmphys_probe::maxExponent = -1e30f;
                }
                const float dx = v.car.position.x - fx, dy = v.car.position.y - fy, dz = v.car.position.z - fz;
                // FVLAB_STAGE=n,x,y,z,r,maxY,minKmh: the n-th CP must be collected within r m of (x, y, z) at >= minKmh,
                // with the car below maxY over the 500 ms before (e.g. a bug CP taken from below). Otherwise the run's
                // CPs stop counting for NEED_CPS / GOAL_CPS: skipping CP n and taking a later CP as the n-th CP (a ground
                // bugslide) or taking CP n from above scores nothing
                static const std::vector<float> stage = [] {
                    std::vector<float> g(7);
                    const char *s = std::getenv("FVLAB_STAGE");
                    if (!s || std::sscanf(s, "%f,%f,%f,%f,%f,%f,%f", &g[0], &g[1], &g[2], &g[3], &g[4], &g[5], &g[6]) != 7)
                        return std::vector<float>{};
                    return g;
                }();
                if (stagePrevCps == ~0u) stagePrevCps = v.checkpointsCollected;
                if (!stage.empty() && v.checkpointsCollected > stagePrevCps) {
                    if (v.checkpointsCollected == static_cast<unsigned>(stage[0])) {
                        const auto &q = v.car.position;
                        const float sx = q.x - stage[1], sy = q.y - stage[2], sz = q.z - stage[3];
                        bool ok = std::sqrt(sx * sx + sy * sy + sz * sz) <= stage[4] && Speed(v.car) * 3.6f >= stage[6];
                        for (float yy : recentY) if (yy >= stage[5]) ok = false;
                        if (!ok) stageFail = true;
                    }
                    stagePrevCps = v.checkpointsCollected;
                }
                const unsigned cpsNow = stageFail ? 0u : v.checkpointsCollected;
                // FVLAB_NEED_CPS=n: the distance to the target only counts once n checkpoints are collected
                // (e.g. through a ring first); before that the run scores 1000 m + its CP shortfall
                if (cpsNow >= needCps) {
                    // FVLAB_TARGET_MINKMH: arriving slowly (crashed, sliding on the side) is worth less:
                    // +0.5 m per km/h below the minimum
                    const float slow = std::max(0.0f, targetMinKmh - Speed(v.car) * 3.6f);
                    // FVLAB_LAND_BOX=x0,y0,z0,x1,y1,z1: the target is a landing on a surface, not a point. Two or
                    // more wheels on the ground inside the box score minus the km/h there; otherwise the distance
                    // to the box, with being below it counted 3x (flying under a road is not "close")
                    static const std::vector<float> landBox = [] {
                        std::vector<float> b;
                        if (const char *s = std::getenv("FVLAB_LAND_BOX")) {
                            std::string str(s);
                            std::size_t p = 0;
                            while (b.size() < 6 && p <= str.size()) {
                                b.push_back(std::strtof(str.c_str() + p, nullptr));
                                p = str.find(',', p);
                                if (p == std::string::npos) break;
                                ++p;
                            }
                        }
                        return b.size() == 6 ? b : std::vector<float>{};
                    }();
                    if (!landBox.empty()) {
                        const auto &q = v.car.position;
                        const float ox = std::max({landBox[0] - q.x, 0.0f, q.x - landBox[3]});
                        const float oz = std::max({landBox[2] - q.z, 0.0f, q.z - landBox[5]});
                        const float below = std::max(0.0f, landBox[1] - q.y), above = std::max(0.0f, q.y - landBox[4]);
                        const int wheels = v.car.wheelContact[0] + v.car.wheelContact[1] + v.car.wheelContact[2] + v.car.wheelContact[3];
                        // the landing counts only after >= 2 wheels stayed down inside the box for FVLAB_SPIKE_FILTER
                        // ticks, at the filtered (lowest) speed of those ticks: a one-tick touchdown at 421 km/h
                        // right before a crash to 78 km/h scored as a 421 km/h landing (seen in one search)
                        // one wheel is enough once it has to be held (a line may ride one wheel to the CP)
                        const bool inBox = ox == 0.0f && oz == 0.0f && below == 0.0f && above == 0.0f && wheels >= 1;
                        landRun = inBox ? landRun + 1 : 0;
                        const float d = (inBox && landRun > spikeTicks)
                                            ? -filteredKmh(Speed(v.car) * 3.6f)
                                            : std::sqrt(ox * ox + oz * oz) + 3.0f * below + above + 0.5f * slow;
                        sc.minDist = std::min(sc.minDist, d);
                    } else {
                        sc.minDist = std::min(sc.minDist, std::sqrt(dx * dx + dy * dy + dz * dz) + 0.5f * slow);
                    }
                } else {
                    sc.minDist = std::min(sc.minDist, 1000.0f + 100.0f * static_cast<float>(needCps - cpsNow));
                }
                // FVLAB_GOAL_CPS=n: collecting the n-th checkpoint is the goal: -1000 minus the km/h at that moment
                // (beats any distance), so a run that crashes into the CP gate's pillar no longer counts as "there"
                static const unsigned goalCps = std::getenv("FVLAB_GOAL_CPS") ? static_cast<unsigned>(std::atoi(std::getenv("FVLAB_GOAL_CPS"))) : 0u;
                // FVLAB_GOAL_KMH_MS=w: instead score the time of collection: -100000 + ms - w per km/h (earliest wins)
                static const float goalKmhMs = std::getenv("FVLAB_GOAL_KMH_MS") ? std::strtof(std::getenv("FVLAB_GOAL_KMH_MS"), nullptr) : -1.0f;
                // FVLAB_GOAL_NEAR=x,y,z,r: the goal CP counts only if the car is within r metres of (x, y, z) on the tick
                // it is collected (GOAL_CPS alone accepts any n-th CP: a search for one bug CP took a different CP instead)
                static const std::vector<float> goalNear = [] {
                    std::vector<float> g(4);
                    const char *s = std::getenv("FVLAB_GOAL_NEAR");
                    if (!s || std::sscanf(s, "%f,%f,%f,%f", &g[0], &g[1], &g[2], &g[3]) != 4) return std::vector<float>{};
                    return g;
                }();
                recentY.push_back(v.car.position.y);
                if (recentY.size() > 50) recentY.erase(recentY.begin());
                if (v.checkpointsCollected > prevCps) {
                    if (!goalNear.empty() && v.checkpointsCollected == goalCps) {
                        const auto &q = v.car.position;
                        const float gx = q.x - goalNear[0], gy = q.y - goalNear[1], gz = q.z - goalNear[2];
                        goalHit = std::sqrt(gx * gx + gy * gy + gz * gz) <= goalNear[3];
                        // FVLAB_GOAL_RISING=v: ... and the car must move up at >= v m/s (a bug CP from below; in one
                        // search it otherwise slid down the banked road edge into the CP from above)
                        static const float goalRising = std::getenv("FVLAB_GOAL_RISING") ? std::strtof(std::getenv("FVLAB_GOAL_RISING"), nullptr) : -1e9f;
                        if (v.car.linearSpeed.y < goalRising) goalHit = false;
                        // FVLAB_GOAL_MINKMH=k: ... at >= k km/h (a slow hop over the road edge after hitting the deck
                        // from above also rises into the trigger)
                        static const float goalMinKmh = std::getenv("FVLAB_GOAL_MINKMH") ? std::strtof(std::getenv("FVLAB_GOAL_MINKMH"), nullptr) : 0.0f;
                        if (Speed(v.car) * 3.6f < goalMinKmh) goalHit = false;
                        // FVLAB_GOAL_MAXY=y: ... and the car stayed below y over the 500 ms before (it came from below)
                        static const float goalMaxY = std::getenv("FVLAB_GOAL_MAXY") ? std::strtof(std::getenv("FVLAB_GOAL_MAXY"), nullptr) : 1e9f;
                        for (float yy : recentY) if (yy >= goalMaxY) goalHit = false;
                    }
                    prevCps = v.checkpointsCollected;
                }
                // scored once, on the tick the goal CP is collected, with the spike filter: before, every later tick
                // counted, so the best speed after the CP (or a contact spike) passed as "speed at the CP"
                if (goalCps > 0 && cpsNow >= goalCps && (goalNear.empty() || goalHit) && !goalScored) {
                    goalScored = true;
                    const float kmh = filteredKmh(Speed(v.car) * 3.6f);
                    sc.minDist = std::min(sc.minDist, goalKmhMs >= 0.0f
                                                          ? -100000.0f + static_cast<float>(v.timeMs) - goalKmhMs * std::min(kmh, 1000.0f)
                                                          : -1000.0f - kmh);
                }
                // FVLAB_SPIKE_FILTER=n (see recentKmh): one-tick speed spikes of hard contacts (e.g. 251 -> 774 -> 264 km/h
                // at an inflatable) neither raise the peak speed nor pass FVLAB_EXP_MINKMH
                const float tickKmh = filteredKmh(Speed(v.car) * 3.6f);
                sc.maxKmh = std::max(sc.maxKmh, tickKmh);
                rawMaxKmh = std::max(rawMaxKmh, Speed(v.car) * 3.6f);
                // FVLAB_WHEELIE_WEIGHT=w: the uber recipe of the public TMN TASes (C0, D-2, F-9, Pro A-1, ...): the car
                // rides 10-20 ticks on the rear wheels only, nose up 2-20 deg, roll under 30 deg, >= 230 km/h, gas and
                // brake held, then a front wheel touches and the speed jumps to 770-1000 km/h. Rewards the longest such
                // ride (up to 25 ticks); the speed jump itself is paid by SPEED_WEIGHT
                {
                    const Pose wp = PoseOf(v.car);
                    const bool rearOnly = (v.car.wheelContact[2] || v.car.wheelContact[3]) && !v.car.wheelContact[0] &&
                                          !v.car.wheelContact[1];
                    if (rearOnly && wp.pitchDeg <= -2.0f && wp.pitchDeg >= -20.0f && std::fabs(wp.rollDeg) < 30.0f &&
                        Speed(v.car) * 3.6f >= 230.0f) {
                        wheelieBest = std::max(wheelieBest, ++wheelieRun);
                    } else {
                        wheelieRun = 0;
                    }
                }
                const std::int64_t now = static_cast<std::int64_t>(v.timeMs);
                // the boost tick uses the spike filter (FVLAB_SPIKE_FILTER): a one-tick contact spike to 1000 km/h
                // counted as "the boost" (seen once: 627 -> 1000 -> 396 km/h) and won on BOOST_DIR
                // FVLAB_BOOST_FROM=ms: the boost tick is looked for from this race time on (a window that opens before
                // a checkpoint flies at ~470 km/h, which counted as "the uber" at the window start)
                static const std::int64_t boostFrom = std::getenv("FVLAB_BOOST_FROM") ? std::atoll(std::getenv("FVLAB_BOOST_FROM")) : 0;
                if (boostAt < 0 && now >= boostFrom && filteredKmh(Speed(v.car) * 3.6f) >= boostKmh) {
                    boostAt = now;
                    if (!boostBox.empty()) {
                        const auto &q = v.car.position;
                        const float ox = std::max({boostBox[0] - q.x, 0.0f, q.x - boostBox[0] - boostBox[3]});
                        const float oy = std::max({boostBox[1] - q.y, 0.0f, q.y - boostBox[1] - boostBox[4]});
                        const float oz = std::max({boostBox[2] - q.z, 0.0f, q.z - boostBox[2] - boostBox[5]});
                        boostPenalty += boostBoxW * std::sqrt(ox * ox + oy * oy + oz * oz);
                    }
                }
                // FVLAB_VIA=x,y,z,sx,sy,sz (TMI corner + size): a soft guide box the car should pass through, with no
                // speed or event condition (e.g. after a checkpoint: uber, flick or noseboost, whatever gives the angle to
                // the next hit and checkpoint). Scores FVLAB_VIA_W (default 1) per metre of the closest approach.
                static const std::vector<float> viaBox = [] {
                    std::vector<float> b(6);
                    const char *s = std::getenv("FVLAB_VIA");
                    if (!s || std::sscanf(s, "%f,%f,%f,%f,%f,%f", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) return std::vector<float>{};
                    return b;
                }();
                if (viaBox.size() == 6 && now >= boostFrom) {
                    const auto &q = v.car.position;
                    const float ox = std::max({viaBox[0] - q.x, 0.0f, q.x - viaBox[0] - viaBox[3]});
                    const float oy = std::max({viaBox[1] - q.y, 0.0f, q.y - viaBox[1] - viaBox[4]});
                    const float oz = std::max({viaBox[2] - q.z, 0.0f, q.z - viaBox[2] - viaBox[5]});
                    // FVLAB_VIA_MINKMH / FVLAB_VIA_KMHW: also 0.5 (KMHW) per km/h below MINKMH on that tick, so the
                    // box is reached with speed for the uber (a hard turn reached it at 216 km/h)
                    static const float viaMinKmh = std::getenv("FVLAB_VIA_MINKMH") ? std::strtof(std::getenv("FVLAB_VIA_MINKMH"), nullptr) : 0.0f;
                    static const float viaKmhW = std::getenv("FVLAB_VIA_KMHW") ? std::strtof(std::getenv("FVLAB_VIA_KMHW"), nullptr) : 0.5f;
                    const float slowVia = std::max(0.0f, viaMinKmh - filteredKmh(Speed(v.car) * 3.6f));
                    viaMin = std::min(viaMin, std::sqrt(ox * ox + oy * oy + oz * oz) + viaKmhW * slowVia);
                }
                if (boostDirW > 0.0f && boostAt >= 0 && now == boostAt + 100) {
                    const float tx = fx - v.car.position.x, tz = fz - v.car.position.z;
                    const float vx = v.car.linearSpeed.x, vz = v.car.linearSpeed.z;
                    const float n = std::sqrt((tx * tx + tz * tz) * (vx * vx + vz * vz));
                    boostPenalty += boostDirW * (n > 1e-6f ? std::acos(std::clamp((tx * vx + tz * vz) / n, -1.0f, 1.0f)) * 57.29578f : 180.0f);
                }
                const bool front = v.car.wheelContact[0] || v.car.wheelContact[1];
                // a landing needs a flight first: from a car that is still rolling at LAND_MIN, the first
                // front contact is just driving on
                if (now >= landMin && !front && !v.car.wheelContact[2] && !v.car.wheelContact[3]) {
                    airborne = true;
                    if (landedAt < 0) ++airTicks;
                }
                // FVLAB_NOSE_ONPITCH=deg: the nose phase may also start without a flight, on the first tick with a
                // front wheel down, both rear wheels up and the nose at least that far down (a ramm or a rear wheel
                // on a road border tips the car onto one front wheel while it keeps rolling: D-0, A15, C0 TAS)
                static const float noseOnPitch = std::getenv("FVLAB_NOSE_ONPITCH") ? std::strtof(std::getenv("FVLAB_NOSE_ONPITCH"), nullptr) : 0.0f;
                const bool tipped = noseOnPitch > 0.0f && front && !v.car.wheelContact[2] && !v.car.wheelContact[3] &&
                                    PoseOf(v.car).pitchDeg >= noseOnPitch;
                if (noseObjective && landedAt < 0 && now >= landMin && front && (noseOnPitch > 0.0f ? tipped : airborne)) {
                    landedAt = now;
                    landKmh = Speed(v.car) * 3.6f;
                    const Pose p = PoseOf(v.car);
                    sc.noseErr = std::fabs(90.0f - p.pitchDeg) + 30.0f * (v.car.wheelContact[2] + v.car.wheelContact[3]);
                    // FVLAB_NOSE_ROLL=w: + w per degree of roll at the landing (the confirmed noseboost landed at
                    // -0.8 deg, two landings without a boost at 4.4 and 9.6 deg)
                    static const float noseRollW = std::getenv("FVLAB_NOSE_ROLL") ? std::strtof(std::getenv("FVLAB_NOSE_ROLL"), nullptr) : 0.0f;
                    sc.noseErr += noseRollW * std::fabs(p.rollDeg);
                    // FVLAB_NOSE_SPIN=w: + w per deg/s of pitch rotation at the landing (a landing at 81 deg that still
                    // turns forward at ~150 deg/s goes past 90 deg 60 ms later and hits the body; the road-CP boost
                    // landed turning at ~15 deg/s)
                    static const float noseSpinW = std::getenv("FVLAB_NOSE_SPIN") ? std::strtof(std::getenv("FVLAB_NOSE_SPIN"), nullptr) : 0.0f;
                    sc.noseErr += noseSpinW * std::fabs(p.localW[1]) * 57.29578f;
                    // FVLAB_NOSE_BOX=x,y,z,sx,sy,sz (TMInterface corner + size): the landing must be in this box,
                    // + 3 per metre outside (e.g. the floor of a ring CP)
                    static const std::vector<float> noseBox = [] {
                        std::vector<float> b(6);
                        const char *s = std::getenv("FVLAB_NOSE_BOX");
                        if (!s || std::sscanf(s, "%f,%f,%f,%f,%f,%f", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) return std::vector<float>{};
                        return b;
                    }();
                    if (!noseBox.empty()) {
                        const auto &q = v.car.position;
                        const float ox = std::max({noseBox[0] - q.x, 0.0f, q.x - noseBox[0] - noseBox[3]});
                        const float oy = std::max({noseBox[1] - q.y, 0.0f, q.y - noseBox[1] - noseBox[4]});
                        const float oz = std::max({noseBox[2] - q.z, 0.0f, q.z - noseBox[2] - noseBox[5]});
                        sc.noseErr += 3.0f * std::sqrt(ox * ox + oy * oy + oz * oz);
                    }
                    // FVLAB_NOSE_MINKMH: a nose landing after crashing the speed away is worthless
                    if (landKmh < noseMinKmh) sc.noseErr += noseMinKmh - landKmh;
                    // FVLAB_NEED_CPS also gates the nose landing (e.g. the ring must be taken before it)
                    if (v.checkpointsCollected < needCps) sc.noseErr += 200.0f;
                    // FVLAB_LAND_MINY: the landing must be on the intended level (not on the ground far below)
                    static const float landMinY = std::getenv("FVLAB_LAND_MINY") ? std::strtof(std::getenv("FVLAB_LAND_MINY"), nullptr) : -1e9f;
                    if (v.car.position.y < landMinY) sc.noseErr += 200.0f + (landMinY - v.car.position.y);
                }
                // gain counts only while the rear wheels are off the ground (a kick after the car has fallen back
                // onto its wheels is an uber, not a nose boost); noselow keeps the plain 600 ms window
                const bool rearUp = !v.car.wheelContact[2] && !v.car.wheelContact[3] && PoseOf(v.car).pitchDeg > 45.0f;
                if (chainObjective && now >= chainFrom && chainDoneAt < 0 && !chainWps.empty()) {
                    if (!rearUp) chainPenalty += chainOffW;
                    // FVLAB_CHAIN_AIR_W=w: + w per tick with no front wheel on the ground. A nose chain stays on the
                    // ground (A01 TAS); rearUp alone also rewards hopping into the air with the nose down.
                    static const float chainAirW = std::getenv("FVLAB_CHAIN_AIR_W") ? std::strtof(std::getenv("FVLAB_CHAIN_AIR_W"), nullptr) : 0.0f;
                    if (!front) chainPenalty += chainAirW;
                    chainPenalty += 0.01f * std::max(0.0f, chainMinKmh - Speed(v.car) * 3.6f);
                    while (chainNext < chainWps.size()) {
                        const auto &w = chainWps[chainNext];
                        const float wx = v.car.position.x - w[0], wz = v.car.position.z - w[1];
                        const float d = std::sqrt(wx * wx + wz * wz);
                        if (d >= w[2]) {
                            chainDist = d;
                            break;
                        }
                        ++chainNext;
                        chainDist = 1e9f;
                    }
                    if (chainNext == chainWps.size()) chainDoneAt = now;
                }
                // FVLAB_GAIN_MS: how long after the landing the gain counts (default 300; a slow nose start near the
                // 440 km/h "gear" needs longer, the guides evaluate 0.3-0.7 s)
                static const std::int64_t gainMs = std::getenv("FVLAB_GAIN_MS") ? std::atoll(std::getenv("FVLAB_GAIN_MS")) : 300;
                if (landedAt >= 0 && now <= landedAt + (noseLow ? 600 : gainMs) && (noseLow || rearUp)) {
                    // spike-filtered (FVLAB_SPIKE_FILTER): a one-tick 2691 km/h contact spike counted as +2070 gain
                    sc.noseGain = std::max(sc.noseGain, filteredKmh(Speed(v.car) * 3.6f) - landKmh);
                }
                // FVLAB_NOSE_LOSS=w: + w per km/h lost in the 300 ms after the landing. Without it, a body hit at a
                // perfect nose angle that bounces the car off (453 -> 373 km/h, no boost) beats every real nose
                // ride with a few degrees more error.
                // FVLAB_LOSS_MS: how long after the landing the loss counts (default 300; a turn that bounces off a
                // kerb 0.4 s after the landing needs longer)
                static const std::int64_t lossMs = std::getenv("FVLAB_LOSS_MS") ? std::atoll(std::getenv("FVLAB_LOSS_MS")) : 300;
                if (landedAt >= 0 && now <= landedAt + lossMs) noseLoss = std::max(noseLoss, landKmh - Speed(v.car) * 3.6f);
                // FVLAB_NOSE_HOLD=w[,deg]: - w per tick of the longest run after the landing with a front wheel down,
                // both rear wheels up and the nose at least deg (default 82) down. One observed boost came after ~1 s
                // at 83-87 deg; the inflatable nose rides fall off after 0.4-0.6 s without a boost, and the plain gain
                // has no gradient towards a longer ride.
                static const std::vector<float> noseHold = [] {
                    std::vector<float> h{0.0f, 82.0f};
                    const char *s = std::getenv("FVLAB_NOSE_HOLD");
                    if (s) std::sscanf(s, "%f,%f", &h[0], &h[1]);
                    return h;
                }();
                if (noseHold[0] > 0.0f && landedAt >= 0 && now <= landedAt + gainMs) {
                    const bool held = front && !v.car.wheelContact[2] && !v.car.wheelContact[3] && PoseOf(v.car).pitchDeg >= noseHold[1];
                    holdRun = held ? holdRun + 1 : 0;
                    holdBest = std::max(holdBest, holdRun);
                }
                // FVLAB_NOSE_DIR=x,z,w,ms: ms after the landing, + w per degree between the horizontal velocity and
                // the direction to (x, z) (steer the nose ride / boost towards the next CP)
                static const std::vector<float> noseDir = [] {
                    std::vector<float> d(4);
                    const char *s = std::getenv("FVLAB_NOSE_DIR");
                    if (!s || std::sscanf(s, "%f,%f,%f,%f", &d[0], &d[1], &d[2], &d[3]) != 4) return std::vector<float>{};
                    return d;
                }();
                if (!noseDir.empty() && landedAt >= 0 && now == landedAt + static_cast<std::int64_t>(noseDir[3])) {
                    const float tx = noseDir[0] - v.car.position.x, tz = noseDir[1] - v.car.position.z;
                    const float vx = v.car.linearSpeed.x, vz = v.car.linearSpeed.z;
                    const float n = std::sqrt((tx * tx + tz * tz) * (vx * vx + vz * vz));
                    const float deg = n > 1e-6f ? std::acos(std::clamp((tx * vx + tz * vz) / n, -1.0f, 1.0f)) * 57.29578f : 180.0f;
                    sc.noseErr += noseDir[2] * deg;
                }
                // FVLAB_OBJECTIVE=nosepos (after Shweetz's TMInterface plugin BfNoseposPlus): in the air between
                // FVLAB_EVAL_FROM and FVLAB_EVAL_TO, angle = |90 - nose down| + |roll| + heading error beyond 90 deg
                // from the horizontal velocity; below FVLAB_ANGLE_MIN (default 5) the tick scores minus its km/h,
                // above it 1000 + angle. The best tick counts. A set-up stage before the landing search.
                // Generalised pose target (after Sai-Moen's RammFinder wheel states and the Nosepos+ presets):
                //   FVLAB_POSE_WHEELS  4 chars in wheelContact order (as `probe` prints contacts): G = must touch,
                //                      L = must be lifted, ? = any; default LLLL (in the air)
                //   FVLAB_POSE_PITCH   target nose-down pitch (default 90; Nosepos+ uber set-up: -25)
                //   FVLAB_POSE_ROLL    target roll (default 0)
                //   FVLAB_POSE_YAW     target angle between heading and velocity (default: anything up to 90 is free;
                //                      Nosepos+ uber set-up: 90)
                //   FVLAB_POSE_MINKMH  the tick counts only at this speed or more (a nose position below ~390 km/h
                //                      cannot be ridden up to the ~478 km/h boost)
                static const std::string poseWheels = std::getenv("FVLAB_POSE_WHEELS") ? std::getenv("FVLAB_POSE_WHEELS") : "LLLL";
                static const float posePitch = std::getenv("FVLAB_POSE_PITCH") ? std::strtof(std::getenv("FVLAB_POSE_PITCH"), nullptr) : 90.0f;
                static const float poseRoll = std::getenv("FVLAB_POSE_ROLL") ? std::strtof(std::getenv("FVLAB_POSE_ROLL"), nullptr) : 0.0f;
                static const float poseYaw = std::getenv("FVLAB_POSE_YAW") ? std::strtof(std::getenv("FVLAB_POSE_YAW"), nullptr) : -1.0f;
                static const float poseMinKmh = std::getenv("FVLAB_POSE_MINKMH") ? std::strtof(std::getenv("FVLAB_POSE_MINKMH"), nullptr) : 0.0f;
                //   FVLAB_POSE_SOFT=w  wheel states and speed floor become penalties instead of filters: + w per
                //                      wrong wheel and + w/50 per km/h below the floor, so a search that starts on
                //                      four wheels still has a gradient (default 0 = filters)
                static const float poseSoft = std::getenv("FVLAB_POSE_SOFT") ? std::strtof(std::getenv("FVLAB_POSE_SOFT"), nullptr) : 0.0f;
                int wrongWheels = 0;
                for (int i = 0; i < 4 && i < static_cast<int>(poseWheels.size()); ++i) {
                    if (poseWheels[i] == 'G' && !v.car.wheelContact[i]) ++wrongWheels;
                    if (poseWheels[i] == 'L' && v.car.wheelContact[i]) ++wrongWheels;
                }
                const float underKmh = std::max(poseMinKmh - Speed(v.car) * 3.6f, 0.0f);
                float posePenalty = poseSoft > 0.0f ? poseSoft * (wrongWheels + underKmh / 50.0f) : 0.0f;
                //   FVLAB_POSE_KMH_W=w  + w per km/h below FVLAB_POSE_MINKMH instead of POSE_SOFT/50 (in one search: with
                //                      0.4 per km/h the search kept a 210 km/h pose below the ~230 km/h uber minimum)
                static const float poseKmhW = std::getenv("FVLAB_POSE_KMH_W") ? std::strtof(std::getenv("FVLAB_POSE_KMH_W"), nullptr) : -1.0f;
                if (poseKmhW >= 0.0f && poseSoft > 0.0f) posePenalty += (poseKmhW - poseSoft / 50.0f) * underKmh;
                //   FVLAB_POSE_BOX=x0,z0,x1,z1,w  the pose must be inside this ground rectangle: + w per metre
                //                      outside (e.g. a noseboost on one stretch of the route, not just anywhere)
                static const std::vector<float> poseBox = [] {
                    std::vector<float> b(5);
                    const char *s = std::getenv("FVLAB_POSE_BOX");
                    if (!s || std::sscanf(s, "%f,%f,%f,%f,%f", &b[0], &b[1], &b[2], &b[3], &b[4]) != 5) return std::vector<float>{};
                    return b;
                }();
                if (!poseBox.empty()) {
                    const auto &q = v.car.position;
                    const float ox = std::max({poseBox[0] - q.x, 0.0f, q.x - poseBox[2]});
                    const float oz = std::max({poseBox[1] - q.z, 0.0f, q.z - poseBox[3]});
                    posePenalty += poseBox[4] * std::sqrt(ox * ox + oz * oz);
                }
                // cpsNow, not checkpointsCollected: with FVLAB_STAGE a pose after a CP taken the wrong way does not count
                if (noseposObjective && now >= evalFrom && now <= evalTo && cpsNow >= needCps &&
                    (poseSoft > 0.0f || (wrongWheels == 0 && underKmh == 0.0f))) {
                    const Pose p = PoseOf(v.car);
                    const Mat3 r = FromQuat(v.car.rotationX, v.car.rotationY, v.car.rotationZ, v.car.rotationW);
                    // heading: forward + up projected on the ground (forward when level, up when nose down)
                    const float hx = r.m[0][2] + r.m[0][1], hz = r.m[2][2] + r.m[2][1];
                    const float vx = v.car.linearSpeed.x, vz = v.car.linearSpeed.z;
                    float heading = 0.0f;
                    if (hx * hx + hz * hz > 1e-6f && vx * vx + vz * vz > 1e-6f) {
                        const float c = (hx * vx + hz * vz) / std::sqrt((hx * hx + hz * hz) * (vx * vx + vz * vz));
                        heading = std::acos(std::clamp(c, -1.0f, 1.0f)) * 57.29578f;
                    }
                    const float yawErr = poseYaw < 0.0f ? std::max(heading - 90.0f, 0.0f) : std::fabs(heading - poseYaw);
                    const float angle = std::fabs(posePitch - p.pitchDeg) + std::fabs(p.rollDeg - poseRoll) + yawErr + posePenalty;
                    static const float angleMin = std::getenv("FVLAB_ANGLE_MIN") ? std::strtof(std::getenv("FVLAB_ANGLE_MIN"), nullptr) : 5.0f;
                    const float tickValue = angle < angleMin ? -Speed(v.car) * 3.6f : 1000.0f + angle;
                    if (tickValue < noseposBest) {
                        noseposBest = tickValue;
                        sc.noseErr = angle;
                    }
                }
                if (noseLow && landedAt >= 0 && now > landedAt + 600) break;
                if (landedAt == now) pullsAtLanding = tmphys_probe::donutPulls;
                if (landedAt >= 0 && now == landedAt + 100 && tmphys_probe::donutPulls == pullsAtLanding) {
                    sc.noseErr += 40.0f;  // the donut (nosebug) did not start after the landing
                }
                if (v.raceCompleted) {
                    sc.finishMs = v.finishTime && v.finishTime->IsValid() ? v.finishTime->estimatedNs / 1e6
                                                                          : (v.finishTimeMs ? *v.finishTimeMs : 1e9);
                    break;
                }
                lastKmh = Speed(v.car) * 3.6f;
                if (spikeTicks > 0) {
                    recentKmh.push_back(lastKmh);
                    if (static_cast<int>(recentKmh.size()) > spikeTicks) recentKmh.erase(recentKmh.begin());
                }
                if (v.timeMs >= static_cast<std::uint64_t>(to + horizon)) break;
                if (!sandbox.AdvanceTicks(1)) break;
            }
            sc.maxExp = expMinKmh > 0.0f ? gatedExp : tmphys_probe::maxExponent;
            // FVLAB_NOSE_NOLAND_KMH=w: without a front-wheel landing the nose error is 1000 - w * (km/h at the end)
            // instead of 1000 - 5 per air tick, so runs that crash the body (no landing, speed gone) rank below
            // runs that keep the speed
            static const float noLandW = std::getenv("FVLAB_NOSE_NOLAND_KMH") ? std::strtof(std::getenv("FVLAB_NOSE_NOLAND_KMH"), nullptr) : 0.0f;
            // Peak speed counts only up to the 1000 km/h cap (post-kick spikes above it never move the car),
            // 0.1 m per km/h, so once an uber exists the distance to the target decides.
            // FVLAB_SPEED_WEIGHT (default 0.1) scales the speed bonus; 0 = distance (and near-uber exponent) only,
            // so a weaker, flatter uber is not penalised once one exists.
            static const double speedWeight = std::getenv("FVLAB_SPEED_WEIGHT") ? std::atof(std::getenv("FVLAB_SPEED_WEIGHT")) : 0.1;
            // FVLAB_EXP_WEIGHT (default 5): bonus per unit of the near-uber exponent; 0 once no uber is wanted (a
            // noseboost to a CP: donut pulls there are noise that outweighs metres of distance)
            static const double expWeight = std::getenv("FVLAB_EXP_WEIGHT") ? std::atof(std::getenv("FVLAB_EXP_WEIGHT")) : 5.0;
            if (chainObjective) {
                const float left = static_cast<float>(chainWps.size() - chainNext);
                sc.value = 1000.0 * left + chainPenalty +
                           (chainDoneAt >= 0 ? static_cast<double>(chainDoneAt - chainFrom) / 10.0 : std::min(chainDist, 1000.0f));
                sc.noseErr = chainPenalty;  // reported as "nose"
                sc.noseGain = static_cast<float>(chainNext);  // reported as "gain": waypoints reached
            } else if (noseposObjective) {
                sc.value = sc.finishMs > 0 ? sc.finishMs - 1e6 : noseposBest;
            } else if (noseLow) {
                sc.noseErr = landKmh;  // reported as "nose": the landing speed
                sc.value = landedAt < 0 ? 1e9
                           : ((boostGain > 0 ? sc.noseGain >= boostGain : landKmh + sc.noseGain >= 950.0f)
                                  ? landKmh
                                  : 10000.0 + landKmh - sc.noseGain);
            } else if (noseObjective) {
                if (landedAt < 0) {
                    sc.noseErr = noLandW > 0.0f ? 1000.0f - noLandW * std::min(lastKmh, 1000.0f)
                                                : 1000.0f - 5.0f * std::min(airTicks, 100);
                }
                static const float noseLossW = std::getenv("FVLAB_NOSE_LOSS") ? std::strtof(std::getenv("FVLAB_NOSE_LOSS"), nullptr) : 0.0f;
                if (landedAt >= 0) sc.noseErr += noseLossW * noseLoss;
                static const float noseHoldW = std::getenv("FVLAB_NOSE_HOLD") ? std::strtof(std::getenv("FVLAB_NOSE_HOLD"), nullptr) : 0.0f;
                if (landedAt >= 0) sc.noseErr -= noseHoldW * holdBest;
                sc.value = sc.finishMs > 0 ? sc.finishMs - 1e6
                                           : sc.noseErr - 0.5 * sc.noseGain + 0.05 * sc.minDist;
            } else {
                sc.value = sc.finishMs > 0 ? sc.finishMs - 1e6
                                           : sc.minDist - speedWeight * std::min(sc.maxKmh, 1000.0f) -
                                                     expWeight * std::clamp(static_cast<double>(sc.maxExp), 0.0, 6.0);
                // FVLAB_RAW_KMH_WEIGHT=w (with FVLAB_SPIKE_FILTER): w points per km/h of the unfiltered peak, so contact
                // spikes (an inflatable gives 251 -> 774 -> 264 km/h) guide the search towards a spike the car keeps,
                // which the filtered peak (SPEED_WEIGHT) then rewards fully
                static const double rawKmhW = std::getenv("FVLAB_RAW_KMH_WEIGHT") ? std::atof(std::getenv("FVLAB_RAW_KMH_WEIGHT")) : 0.0;
                if (sc.finishMs <= 0) sc.value -= rawKmhW * std::min(rawMaxKmh, 3000.0f);
                static const double wheelieW = std::getenv("FVLAB_WHEELIE_WEIGHT") ? std::atof(std::getenv("FVLAB_WHEELIE_WEIGHT")) : 0.0;
                if (sc.finishMs <= 0) sc.value -= wheelieW * std::min(wheelieBest, 25);
                sc.noseGain = static_cast<float>(wheelieBest);  // reported for the wheelie objective
                if (viaMin < 1e8f) {
                    static const float viaW = std::getenv("FVLAB_VIA_W") ? std::strtof(std::getenv("FVLAB_VIA_W"), nullptr) : 1.0f;
                    sc.value += viaW * viaMin;
                    sc.noseErr = viaMin;  // reported as "nose"
                }
                if (!boostBox.empty() || boostDirW > 0.0f) {
                    if (boostAt < 0) boostPenalty = 500.0f;
                    sc.value += boostPenalty;
                    sc.noseErr = boostPenalty;  // reported as "nose"
                }
            }
            return sc;
        };
        // keyboard-only and human spacing for FVLAB_RTA
        auto makeHuman = [&](std::vector<PhysicsSandboxInputEvent> &cand) {
            if (!rta) return;
            for (auto &e : cand) {
                if (e.action == PhysicsSandboxInputAction::Steer) {
                    e.value.analog = e.value.analog > 21845 ? 65536 : e.value.analog < -21845 ? -65536 : 0;
                }
            }
            std::stable_sort(cand.begin(), cand.end(), [](const auto &l, const auto &r) { return l.timeMs < r.timeMs; });
            std::array<std::int32_t, 16> last;
            last.fill(-100000);
            std::vector<PhysicsSandboxInputEvent> kept;
            for (const auto &e : cand) {
                auto &l = last[static_cast<std::size_t>(e.action) & 15u];
                if (e.timeMs - l < 50) continue;
                l = e.timeMs;
                kept.push_back(e);
            }
            cand = std::move(kept);
        };
        auto writeBest = [&]() {
            std::vector<PhysicsSandboxInputEvent> all = fixed;
            all.insert(all.end(), best.begin(), best.end());
            std::stable_sort(all.begin(), all.end(), [](const auto &l, const auto &r) { return l.timeMs < r.timeMs; });
            // a reader (scp, a virus scan) can hold the file for a moment: retry, a silently skipped write leaves
            // a CSV that does not match the reported best
            FILE *f = nullptr;
            for (int i = 0; i < 100 && f == nullptr; ++i) {
                f = std::fopen(outPath.c_str(), "w");
                if (f == nullptr) std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            if (f == nullptr) {
                std::fprintf(stderr, "could not write %s\n", outPath.c_str());
                return;
            }
            std::fprintf(f, "timeMs,action,kind,switch,analog\n");
            for (const auto &e : all) {
                std::fprintf(f, "%d,%d,%d,%d,%d\n", e.timeMs, static_cast<int>(e.action), static_cast<int>(e.value.kind),
                             static_cast<int>(e.value.switchState), static_cast<int>(e.value.analog));
            }
            std::fclose(f);
        };
        // FVLAB_FILL=1 (TMInterface's "fill missing steering input"): one steer event on every tick of the window
        // with the value held there, so a value change moves exactly one tick. The community default for
        // noseboosts and ubers.
        if (std::getenv("FVLAB_FILL")) {
            std::int32_t held = 0;
            for (const auto &e : fixed) {
                if (e.action == PhysicsSandboxInputAction::Steer && e.timeMs <= from) held = e.value.analog;
            }
            std::stable_sort(best.begin(), best.end(), [](const auto &l, const auto &r) { return l.timeMs < r.timeMs; });
            std::vector<PhysicsSandboxInputEvent> filled;
            std::size_t i = 0;
            for (std::int32_t t = from + 10; t < to; t += 10) {
                for (; i < best.size() && best[i].timeMs <= t; ++i) {
                    if (best[i].action == PhysicsSandboxInputAction::Steer) held = best[i].value.analog;
                    else filled.push_back(best[i]);
                }
                filled.push_back(Steer(t, held));
            }
            best = std::move(filled);
        }
        makeHuman(best);
        Score bestScore = run(best);
        std::printf("base: score %.2f finish %.2f ms, closest %.1f m, max exponent %.2f, max %.0f km/h, nose %.1f gain %.1f, %zu events\n",
                    bestScore.value, bestScore.finishMs, bestScore.minDist, bestScore.maxExp, bestScore.maxKmh,
                    bestScore.noseErr, bestScore.noseGain, best.size());
        std::fflush(stdout);
        writeBest();
        if (std::getenv("FVLAB_LEAKTEST") && std::string(std::getenv("FVLAB_LEAKTEST")) == "base") {
            // same inputs again, then after a run with all window inputs removed, then again
            const Score again = run(best);
            const Score other = run(std::vector<PhysicsSandboxInputEvent>{});
            const Score after = run(best);
            std::printf("leaktest: base %.4f again %.4f empty-window %.4f after %.4f\n", bestScore.value, again.value,
                        other.value, after.value);
            return 0;
        }
        const auto t0 = std::chrono::steady_clock::now();
        std::uint64_t attempts = 0;
        // FVLAB_KICK_STALL=s (iterated local search): the walker `cur` normally equals the best; after s seconds
        // without an improvement of the walker it jumps to the best with 8-20 random changes and continues from
        // there even if that is worse. Only the overall best is written. Gets a search out of a dead end that
        // small steps never leave.
        static const double kickStall = std::getenv("FVLAB_KICK_STALL") ? std::atof(std::getenv("FVLAB_KICK_STALL")) : 0.0;
        std::vector<PhysicsSandboxInputEvent> cur = best;
        Score curScore = bestScore;
        double lastImprove = 0.0;
        unsigned kicks = 0;
        for (;;) {
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            if (elapsed > seconds) break;
            const bool kick = kickStall > 0.0 && elapsed - lastImprove > kickStall;
            std::vector<PhysicsSandboxInputEvent> cand = kick ? best : cur;
            // FVLAB_CHANGES=n: 1..n changes per attempt (default 4; the guides use 10-20 for noseboosts in short windows)
            static const int maxChanges = std::getenv("FVLAB_CHANGES") ? std::max(1, std::atoi(std::getenv("FVLAB_CHANGES"))) : 4;
            const int changes = kick ? uniform(8, 20) : uniform(1, maxChanges);
            for (int c = 0; c < changes; ++c) {
                const int kind = uniform(0, 99);
                const std::int32_t t = from + 10 + uniform(0, std::max(0, (to - from) / 10 - 2)) * 10;
                if (cand.empty() || kind < 15) {
                    cand.push_back(Steer(t, uniform(0, 2) == 0 ? (uniform(0, 1) ? 65536 : -65536) : uniform(-65536, 65536)));
                } else if (kind < 25 && noseLow) {
                    // lift off the gas (or tap the brake) for 10 ms .. FVLAB_LIFT_MAX ms (default 100):
                    // lowers the speed without breaking the run
                    static const int liftMax = std::getenv("FVLAB_LIFT_MAX") ? std::atoi(std::getenv("FVLAB_LIFT_MAX")) : 100;
                    const bool brakeTap = uniform(0, 9) < 3;
                    PhysicsSandboxInputEvent e;
                    e.timeMs = t;
                    e.action = brakeTap ? PhysicsSandboxInputAction::Brake : PhysicsSandboxInputAction::Accelerate;
                    e.value.kind = PhysicsSandboxInputValueKind::Switch;
                    e.value.switchState = brakeTap ? PhysicsSandboxSwitchState::Pressed : PhysicsSandboxSwitchState::Released;
                    cand.push_back(e);
                    e.timeMs = std::min(to - 10, t + uniform(1, std::max(1, liftMax / 10)) * 10);
                    e.value.switchState = brakeTap ? PhysicsSandboxSwitchState::Released : PhysicsSandboxSwitchState::Pressed;
                    cand.push_back(e);
                } else if (kind < 25) {
                    PhysicsSandboxInputEvent e;
                    e.timeMs = t;
                    e.action = uniform(0, 1) ? PhysicsSandboxInputAction::Brake : PhysicsSandboxInputAction::Accelerate;
                    e.value.kind = PhysicsSandboxInputValueKind::Switch;
                    e.value.switchState = uniform(0, 1) ? PhysicsSandboxSwitchState::Pressed : PhysicsSandboxSwitchState::Released;
                    cand.push_back(e);
                } else {
                    auto &e = cand[static_cast<std::size_t>(uniform(0, static_cast<int>(cand.size()) - 1))];
                    if (kind < 65 && e.action == PhysicsSandboxInputAction::Steer) {
                        const int sigma = std::array<int, 4>{1000, 4000, 16000, 65536}[static_cast<std::size_t>(uniform(0, 3))];
                        e.value.analog = std::clamp(e.value.analog + uniform(-sigma, sigma), -65536, 65536);
                    } else if (kind < 95) {
                        e.timeMs = std::clamp(e.timeMs + uniform(-3, 3) * 10, from + 10, to - 10);
                    } else {
                        e.timeMs = -1;
                    }
                }
            }
            cand.erase(std::remove_if(cand.begin(), cand.end(), [](const auto &e) { return e.timeMs < 0; }), cand.end());
            std::stable_sort(cand.begin(), cand.end(), [](const auto &l, const auto &r) { return l.timeMs < r.timeMs; });
            KeepLastPerTick(cand);
            makeHuman(cand);
            ++attempts;
            const Score sc = run(cand);
            if (kick) {
                cur = cand;
                curScore = sc;
                lastImprove = elapsed;
                ++kicks;
            } else if (sc.value < curScore.value) {
                cur = cand;
                curScore = sc;
                lastImprove = elapsed;
            }
            if (sc.value < bestScore.value) {
                best = cand;
                bestScore = sc;
                writeBest();
                std::printf("%.1f s, attempt %llu: %.6f ms (segment 0) score %.2f finish %.2f closest %.1f m exp %.2f max %.0f km/h nose %.1f gain %.1f\n",
                            elapsed, static_cast<unsigned long long>(attempts),
                            sc.finishMs > 0 ? sc.finishMs : sc.value, sc.value, sc.finishMs, sc.minDist, sc.maxExp, sc.maxKmh,
                            sc.noseErr, sc.noseGain);
                std::fflush(stdout);
            }
        }
        writeBest();
        if (std::getenv("FVLAB_LEAKTEST")) {
            const Score re = run(best);
            std::printf("recheck in process: %.4f (search said %.4f)\n", re.value, bestScore.value);
        }
        if (kickStall > 0.0) std::printf("kicks: %u\n", kicks);
        std::printf("done: %llu attempts, best %.6f ms score %.4f finish %.2f closest %.1f m exp %.2f max %.0f km/h\n",
                    static_cast<unsigned long long>(attempts),
                    bestScore.finishMs > 0 ? bestScore.finishMs : bestScore.value, bestScore.value,
                    bestScore.finishMs, bestScore.minDist, bestScore.maxExp, bestScore.maxKmh);
        return 0;
    }

    if (mode == "opt" && argc >= 10) {
        std::uint32_t rng = static_cast<std::uint32_t>(std::strtoul(argv[4], nullptr, 10)) * 2654435761u + 12345u;
        auto next = [&rng]() {
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            return rng;
        };
        auto uniform = [&](int lo, int hi) { return lo + static_cast<int>(next() % static_cast<std::uint32_t>(hi - lo + 1)); };
        const std::int32_t from = std::atoi(argv[5]) / 10 * 10;
        const std::int32_t to = std::atoi(argv[6]) / 10 * 10;
        const int segments = std::max(1, std::atoi(argv[7]));
        const double seconds = std::atof(argv[8]);
        const std::string outPath = argv[9];
        // FVLAB_TARGET="x,y,z": tie-breaker point for finishes without a sub-tick time estimate
        float target[3] = {0, 0, 0};
        // FVLAB_TRIGGER="x,y,z,sx,sy,sz": optimise the earliest entry into this box instead of the finish
        float trig[6] = {0, 0, 0, 0, 0, 0};
        const bool hasTrigger = std::getenv("FVLAB_TRIGGER") != nullptr &&
                                std::sscanf(std::getenv("FVLAB_TRIGGER"), "%f,%f,%f,%f,%f,%f", &trig[0], &trig[1],
                                            &trig[2], &trig[3], &trig[4], &trig[5]) == 6;
        const bool triggerSpeed = hasTrigger && std::getenv("FVLAB_TRIGGER_SPEED") != nullptr;
        const bool hasTarget = std::getenv("FVLAB_TARGET") != nullptr &&
                               std::sscanf(std::getenv("FVLAB_TARGET"), "%f,%f,%f", &target[0], &target[1], &target[2]) == 3;

        auto base = sandbox.ReadInputs();
        if (!base) return Fail("inputs");
        std::vector<PhysicsSandboxInputEvent> fixed, best;
        for (const auto &e : base.Value()) {
            if (e.action == PhysicsSandboxInputAction::Steer && e.timeMs > from && e.timeMs < to) {
                best.push_back(e);
            } else {
                fixed.push_back(e);
            }
        }
        auto merged = [&](const std::vector<PhysicsSandboxInputEvent> &steer) {
            std::vector<PhysicsSandboxInputEvent> all = fixed;
            all.insert(all.end(), steer.begin(), steer.end());
            std::stable_sort(all.begin(), all.end(), [](const auto &l, const auto &r) { return l.timeMs < r.timeMs; });
            return all;
        };
        std::int32_t endMs = to;
        for (const auto &e : base.Value()) endMs = std::max(endMs, e.timeMs);
        endMs = (endMs / 10 + 200) * 10;

        if (!advanceTo(static_cast<std::uint64_t>(from))) return Fail("advance");
        auto start = sandbox.CaptureState();
        if (!start) return Fail("capture");
        std::vector<std::int32_t> segStart;
        for (int i = 0; i < segments; ++i) segStart.push_back((from + (to - from) * i / segments) / 10 * 10);

        // simulate candidate steering from segment k's cached state; returns finish ns or UINT64_MAX
        std::vector<PhysicsSandboxState> cache;
        double bestNs = 0;
        auto run = [&](const std::vector<PhysicsSandboxInputEvent> &steer, int k, double limitNs,
                       std::vector<PhysicsSandboxState> *capture) -> double {
            const PhysicsSandboxState &s0 = k == 0 ? start.Value() : cache[static_cast<std::size_t>(k)];
            if (!sandbox.RestoreState(s0)) return 1e30;
            std::vector<PhysicsSandboxInputEvent> all = merged(steer), win;
            // the tick at the restored state's time is already applied; a base event at `to` must stay
            const std::int32_t winFrom = segStart[static_cast<std::size_t>(k)] + 10;
            for (const auto &e : all) {
                if (e.timeMs >= winFrom && e.timeMs < to &&
                    (e.action == PhysicsSandboxInputAction::Steer || e.action == PhysicsSandboxInputAction::Accelerate ||
                     e.action == PhysicsSandboxInputAction::Gas || e.action == PhysicsSandboxInputAction::Brake)) {
                    win.push_back(e);
                }
            }
            auto replaced = sandbox.ReplaceInputWindow(winFrom, to - 10, win);
            if (!replaced) {
                std::fprintf(stderr, "window: %s\n", replaced.Error().diagnostic.c_str());
                return 1e30;
            }
            std::size_t nextSeg = static_cast<std::size_t>(k) + 1;
            float prevTargetDist = 5.0f;
            float prevPos[3] = {0, 0, 0};
            bool havePrev = false;
            for (;;) {
                auto st = sandbox.ReadState();
                if (!st) return 1e30;
                const auto &v = st.Value();
                if (capture != nullptr) {
                    while (nextSeg < segStart.size() && v.timeMs >= static_cast<std::uint64_t>(segStart[nextSeg])) {
                        auto c = sandbox.CaptureState();
                        if (c) (*capture)[nextSeg] = std::move(c).Value();
                        ++nextSeg;
                    }
                }
                if (v.raceCompleted) {
                    // an estimate sitting on the tick start (ns % 10 ms <= 1) carries no sub-tick information
                    if (v.finishTime && v.finishTime->IsValid() &&
                        !(hasTarget && v.finishTime->estimatedNs % 10000000u <= 1u)) {
                        return static_cast<double>(v.finishTime->estimatedNs);
                    }
                    // No sub-tick estimate (e.g. finishing while falling onto the block): break ties inside the
                    // tick with the distance to FVLAB_TARGET one tick before the finish, worth < 1 tick.
                    return v.finishTimeMs ? *v.finishTimeMs * 1e6 + std::min(prevTargetDist, 5.0f) * 1.9e6 : 1e30;
                }
                if (hasTrigger) {
                    // FVLAB_TRIGGER box (TMInterface style: corner + size): "finish" when the car position enters
                    // it; the entry time is interpolated along the last tick's segment for a sub-tick score
                    const float p[3] = {v.car.position.x, v.car.position.y, v.car.position.z};
                    bool inside = true;
                    for (int i = 0; i < 3; ++i) inside = inside && p[i] >= trig[i] && p[i] <= trig[i] + trig[i + 3];
                    // FVLAB_NEED_CPS=n: the box only counts once n checkpoints are collected (a box behind a CP
                    // line is otherwise reached by passing next to or under the CP)
                    static const unsigned optNeedCps =
                            std::getenv("FVLAB_NEED_CPS") ? static_cast<unsigned>(std::atoi(std::getenv("FVLAB_NEED_CPS"))) : 0u;
                    inside = inside && v.checkpointsCollected >= optNeedCps;
                    if (inside) {
                        if (!havePrev) return static_cast<double>(v.timeMs) * 1e6;
                        float tEnter = 0.0f;  // fraction of the segment prev -> p where the box is entered
                        for (int i = 0; i < 3; ++i) {
                            const float d = p[i] - prevPos[i];
                            if (std::fabs(d) < 1e-6f) continue;
                            const float lo = (trig[i] - prevPos[i]) / d, hi = (trig[i] + trig[i + 3] - prevPos[i]) / d;
                            tEnter = std::max(tEnter, std::min(lo, hi));
                        }
                        tEnter = std::clamp(tEnter, 0.0f, 1.0f);
                        const double enterMs = static_cast<double>(v.timeMs) - 10.0 + 10.0 * tEnter;
                        // FVLAB_TRIGGER_SPEED: highest speed at the entry instead; printed "ms" = 2000 - km/h,
                        // the entry time only breaks ties
                        if (triggerSpeed) return (2000.0 - Speed(v.car) * 3.6) * 1e6 + enterMs;
                        // FVLAB_TRIGGER_KMH_MS=w: entry time minus w ms per km/h at the entry (time and speed
                        // both count; printed "ms" is shifted by -w*km/h)
                        static const double kmhMs =
                                std::getenv("FVLAB_TRIGGER_KMH_MS") ? std::atof(std::getenv("FVLAB_TRIGGER_KMH_MS")) : 0.0;
                        if (kmhMs != 0.0) return (enterMs - kmhMs * Speed(v.car) * 3.6) * 1e6;
                        return enterMs * 1e6;
                    }
                    for (int i = 0; i < 3; ++i) prevPos[i] = p[i];
                    havePrev = true;
                }
                if (hasTarget) {
                    const float dx = v.car.position.x - target[0], dy = v.car.position.y - target[1],
                                dz = v.car.position.z - target[2];
                    prevTargetDist = std::sqrt(dx * dx + dy * dy + dz * dz);
                }
                // with the tie-breaker a run may end in the same tick as the best and still be better;
                // with FVLAB_TRIGGER_KMH_MS the score is shifted by up to -w*1000 (speed below 1000 km/h)
                static const double kmhSlackNs =
                        std::getenv("FVLAB_TRIGGER_KMH_MS") ? std::atof(std::getenv("FVLAB_TRIGGER_KMH_MS")) * 1000.0 * 1e6 : 0.0;
                if ((!triggerSpeed && static_cast<double>(v.timeMs) * 1e6 > limitNs + kmhSlackNs + (hasTarget ? 1e7 : 0.0)) ||
                    v.timeMs >= static_cast<std::uint64_t>(endMs)) return 1e30;
                if (!sandbox.AdvanceTicks(1)) return 1e30;
            }
        };
        auto writeBest = [&]() {
            // a reader (scp, a virus scan) can hold the file for a moment: retry, a silently skipped write leaves
            // a CSV that does not match the reported best
            FILE *f = nullptr;
            for (int i = 0; i < 100 && f == nullptr; ++i) {
                f = std::fopen(outPath.c_str(), "w");
                if (f == nullptr) std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            if (f == nullptr) {
                std::fprintf(stderr, "could not write %s\n", outPath.c_str());
                return;
            }
            std::fprintf(f, "timeMs,action,kind,switch,analog\n");
            for (const auto &e : merged(best)) {
                std::fprintf(f, "%d,%d,%d,%d,%d\n", e.timeMs, static_cast<int>(e.action), static_cast<int>(e.value.kind),
                             static_cast<int>(e.value.switchState), static_cast<int>(e.value.analog));
            }
            std::fclose(f);
        };
        cache.assign(segStart.size(), start.Value());
        bestNs = run(best, 0, 1e30, &cache);
        std::printf("base finish %.6f ms, %zu steer events in window, segments %d\n", bestNs / 1e6, best.size(), segments);
        std::fflush(stdout);
        if (bestNs >= 1e29) return Fail("base run does not finish");
        writeBest();

        const auto t0 = std::chrono::steady_clock::now();
        std::uint64_t attempts = 0, rejectedDrift = 0;
        for (;;) {
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            if (elapsed > seconds) break;
            const int k = uniform(0, segments - 1);
            const std::int32_t a = segStart[static_cast<std::size_t>(k)] + 10;  // not on the restored tick
            const std::int32_t b = k + 1 < segments ? segStart[static_cast<std::size_t>(k) + 1] : to;
            std::vector<PhysicsSandboxInputEvent> cand = best;
            std::vector<std::size_t> inSeg;
            for (std::size_t i = 0; i < cand.size(); ++i) if (cand[i].timeMs >= a && cand[i].timeMs < b) inSeg.push_back(i);
            const int changes = uniform(1, 3);
            for (int c = 0; c < changes; ++c) {
                const int kind = uniform(0, 99);
                if (inSeg.empty() || kind < 20) {
                    const std::int32_t t = (a + uniform(0, std::max(0, (b - a) / 10 - 1)) * 10);
                    const int pick = uniform(0, 2);
                    const AnalogInputState v = pick == 0 ? 65536 : pick == 1 ? -65536 : uniform(-65536, 65536);
                    cand.push_back(Steer(t, v));
                } else {
                    auto &e = cand[inSeg[static_cast<std::size_t>(uniform(0, static_cast<int>(inSeg.size()) - 1))]];
                    if (kind < 70) {
                        const int sigma = std::array<int, 4>{1000, 4000, 16000, 65536}[static_cast<std::size_t>(uniform(0, 3))];
                        e.value.analog = std::clamp(e.value.analog + uniform(-sigma, sigma), -65536, 65536);
                    } else if (kind < 95) {
                        e.timeMs = std::clamp(e.timeMs + uniform(-3, 3) * 10, a, b - 10);
                    } else {
                        e.timeMs = -1;  // delete
                    }
                }
            }
            cand.erase(std::remove_if(cand.begin(), cand.end(), [](const auto &e) { return e.timeMs < 0; }), cand.end());
            std::stable_sort(cand.begin(), cand.end(), [](const auto &l, const auto &r) { return l.timeMs < r.timeMs; });
            KeepLastPerTick(cand);
            ++attempts;
            const double ns = run(cand, k, bestNs, nullptr);
            if (ns < bestNs) {
                // Re-simulate the whole window from the start state: a restored mid-run snapshot can drift
                // from a continuous run, so only a full run decides and refreshes every cached segment.
                std::vector<PhysicsSandboxState> fresh(segStart.size(), start.Value());
                const double full = run(cand, 0, 1e30, &fresh);
                if (!(full < bestNs)) {
                    ++rejectedDrift;
                    continue;
                }
                best = cand;
                bestNs = full;
                cache = std::move(fresh);
                writeBest();
                std::printf("%.1f s, attempt %llu: %.6f ms (segment %d)\n", elapsed,
                            static_cast<unsigned long long>(attempts), bestNs / 1e6, k);
                std::fflush(stdout);
            }
        }
        std::printf("done: %llu attempts, best %.6f ms (%llu candidates rejected by the full re-run)\n",
                    static_cast<unsigned long long>(attempts), bestNs / 1e6,
                    static_cast<unsigned long long>(rejectedDrift));
        return 0;
    }

    // fvlab PACKS REPLAY blocks: placed blocks and trigger objects with their world position
    if (mode == "blocks") {
        auto scene = sandbox.ReadRenderScene();
        if (!scene || !scene.Value()) return Fail("render scene");
        // bxx,bxz,bzx,bzz: world x/z of the block's local x and z axes (rotation; the local origin is a corner)
        std::printf("block,purpose,component,x,y,z,bxx,bxz,bzx,bzz\n");
        for (const auto &inst : scene.Value()->instances) {
            const auto &p = inst.provenance;
            if (p.blockName.empty()) continue;
            const auto &w = inst.worldTransform;
            const auto &t = w.translation;
            std::printf("%s,%d,%u,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f\n", p.blockName.c_str(),
                        static_cast<int>(inst.purpose), p.componentIndex, t.x, t.y, t.z, w.basisX.x, w.basisX.z,
                        w.basisZ.x, w.basisZ.z);
        }
        return 0;
    }

    // fvlab PACKS REPLAY triggers X Y Z RADIUS [PURPOSES]: render triangles of the trigger-like instances (default
    // purposes 4 = Helper and 5 = CheckpointTrigger; PURPOSES e.g. "4,5") with a vertex within RADIUS of (X,Y,Z),
    // in world coordinates (worldTransform x mesh vertex). One "#" line per instance with its block and world bounds.
    if (mode == "triggers" && argc >= 8) {
        const float cx = std::strtof(argv[4], nullptr), cy = std::strtof(argv[5], nullptr),
                    cz = std::strtof(argv[6], nullptr), rad = std::strtof(argv[7], nullptr);
        std::vector<int> purposes{4, 5};
        if (argc >= 9) {
            purposes.clear();
            std::string s(argv[8]);
            std::size_t p = 0;
            while (p < s.size()) {
                purposes.push_back(std::atoi(s.c_str() + p));
                p = s.find(',', p);
                if (p == std::string::npos) break;
                ++p;
            }
        }
        auto scene = sandbox.ReadRenderScene();
        if (!scene || !scene.Value()) return Fail("render scene");
        const auto &rs = *scene.Value();
        auto world = [](const PhysicsSandboxTransform &w, const Vector3 &v) {
            return Vector3{w.translation.x + w.basisX.x * v.x + w.basisY.x * v.y + w.basisZ.x * v.z,
                           w.translation.y + w.basisX.y * v.x + w.basisY.y * v.y + w.basisZ.y * v.z,
                           w.translation.z + w.basisX.z * v.x + w.basisY.z * v.y + w.basisZ.z * v.z};
        };
        auto near = [&](const Vector3 &p) {
            const float dx = p.x - cx, dy = p.y - cy, dz = p.z - cz;
            return dx * dx + dy * dy + dz * dz < rad * rad;
        };
        std::printf("purpose,ax,ay,az,bx,by,bz,cx,cy,cz\n");
        for (const auto &inst : rs.instances) {
            const int pu = static_cast<int>(inst.purpose);
            if (std::find(purposes.begin(), purposes.end(), pu) == purposes.end()) continue;
            if (inst.meshIndex >= rs.meshes.size()) continue;
            const auto &mesh = rs.meshes[inst.meshIndex];
            std::vector<Vector3> wv;
            wv.reserve(mesh.vertices.size());
            bool any = false;
            Vector3 lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};
            for (const auto &v : mesh.vertices) {
                const Vector3 p = world(inst.worldTransform, v.position);
                wv.push_back(p);
                any = any || near(p);
                lo = Vector3{std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
                hi = Vector3{std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
            }
            if (!any) continue;
            std::printf("# purpose %d block %s component %u mesh %u: %zu vertices, %zu triangles, world bounds "
                        "(%.2f, %.2f, %.2f) .. (%.2f, %.2f, %.2f)\n",
                        pu, inst.provenance.blockName.c_str(), inst.provenance.componentIndex, inst.meshIndex,
                        mesh.vertices.size(), mesh.indices.size() / 3, lo.x, lo.y, lo.z, hi.x, hi.y, hi.z);
            for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
                const auto ia = mesh.indices[i], ib = mesh.indices[i + 1], ic = mesh.indices[i + 2];
                if (ia >= wv.size() || ib >= wv.size() || ic >= wv.size()) continue;
                const Vector3 &a = wv[ia], &b = wv[ib], &c = wv[ic];
                std::printf("%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n", pu, a.x, a.y, a.z, b.x, b.y, b.z,
                            c.x, c.y, c.z);
            }
        }
        return 0;
    }

    // fvlab PACKS REPLAY scene X Y Z RADIUS: collision triangles with a vertex within RADIUS of (X,Y,Z)
    if (mode == "scene" && argc >= 8) {
        const float cx = std::strtof(argv[4], nullptr), cy = std::strtof(argv[5], nullptr),
                    cz = std::strtof(argv[6], nullptr), rad = std::strtof(argv[7], nullptr);
        auto scene = sandbox.ReadScene();
        if (!scene) return Fail("scene");
        auto near = [&](const Vector3 &p) {
            const float dx = p.x - cx, dy = p.y - cy, dz = p.z - cz;
            return dx * dx + dy * dy + dz * dz < rad * rad;
        };
        std::printf("ax,ay,az,bx,by,bz,cx,cy,cz\n");
        for (const auto &t : scene.Value().collisionTriangles) {
            if (near(t.a) || near(t.b) || near(t.c)) {
                std::printf("%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n", t.a.x, t.a.y, t.a.z, t.b.x,
                            t.b.y, t.b.z, t.c.x, t.c.y, t.c.z);
            }
        }
        return 0;
    }

    if (mode == "inputs") {
        auto events = sandbox.ReadInputs();
        if (!events) return Fail("inputs");
        std::printf("timeMs,action,kind,switch,analog\n");
        for (const auto &e : events.Value()) {
            std::printf("%d,%d,%d,%d,%d\n", e.timeMs, static_cast<int>(e.action),
                        static_cast<int>(e.value.kind), static_cast<int>(e.value.switchState),
                        static_cast<int>(e.value.analog));
        }
        return 0;
    }

    if (mode == "probe" && argc >= 6) {
        const std::uint64_t t0 = std::strtoull(argv[4], nullptr, 10);
        const std::uint32_t ticks = std::strtoul(argv[5], nullptr, 10);
        if (!advanceTo(t0)) return Fail("advance");
        std::printf("timeMs,kmh,px,py,pz,pitch,roll,wx_l,wy_l,wz_l,contacts,gas,brake,steer,cps\n");
        for (std::uint32_t i = 0; i < ticks; ++i) {
            auto s = sandbox.ReadState().Value();
            Pose p = PoseOf(s.car);
            std::printf("%llu,%.2f,%.3f,%.3f,%.3f,%.2f,%.2f,%.3f,%.3f,%.3f,%d%d%d%d,%.2f,%.2f,%.3f,%u\n",
                        static_cast<unsigned long long>(s.timeMs), Speed(s.car) * 3.6f,
                        s.car.position.x, s.car.position.y, s.car.position.z, p.pitchDeg, p.rollDeg,
                        p.localW[0], p.localW[1], p.localW[2], s.car.wheelContact[0],
                        s.car.wheelContact[1], s.car.wheelContact[2], s.car.wheelContact[3],
                        s.accelerate, s.brake, s.steering, s.checkpointsCollected);
            std::printf("    q,%.6f,%.6f,%.6f,%.6f\n", s.car.rotationX, s.car.rotationY,
                        s.car.rotationZ, s.car.rotationW);
            if (!sandbox.AdvanceTicks(1)) break;
        }
        return 0;
    }

    // fvlab PACKS REPLAY keys FROM_MS TO_MS HORIZON_TICKS
    //   Keyboard steering test: inside [FROM, TO) the steering is replaced by full left / none / full
    //   right (a), switched once to (b) at time ts (every 10 ms); gas/brake events stay as recorded.
    //   Prints a, b, ts and the speed after HORIZON ticks from FROM.
    if (mode == "keys" && argc >= 7) {
        const std::int32_t from = std::atoi(argv[4]);
        const std::int32_t to = std::atoi(argv[5]);
        const std::uint32_t horizon = std::strtoul(argv[6], nullptr, 10);
        if (!advanceTo(static_cast<std::uint64_t>(from) - 10)) return Fail("advance");
        auto snapshot = sandbox.CaptureState();
        auto baseInputs = sandbox.ReadInputs();
        if (!snapshot || !baseInputs) return Fail("capture");
        AnalogInputState lastSteer = 0;
        std::vector<PhysicsSandboxInputEvent> kept;
        for (const auto &e : baseInputs.Value()) {
            if (e.action == PhysicsSandboxInputAction::Steer && e.timeMs <= to) lastSteer = e.value.analog;
            if (e.action != PhysicsSandboxInputAction::Steer && e.timeMs >= from && e.timeMs < to) kept.push_back(e);
        }
        if (std::getenv("FVLAB_HOLD") != nullptr) {
            // simplest human input: gas and brake held down through the whole window
            kept.clear();
            for (auto action : {PhysicsSandboxInputAction::Accelerate, PhysicsSandboxInputAction::Brake}) {
                PhysicsSandboxInputEvent e;
                e.timeMs = from;
                e.action = action;
                e.value.kind = PhysicsSandboxInputValueKind::Switch;
                e.value.switchState = PhysicsSandboxSwitchState::Pressed;
                kept.push_back(e);
            }
        }
        std::printf("a,b,ts,kmh_end,kmh_max\n");
        const AnalogInputState keys[3] = {-65536, 0, 65536};
        for (AnalogInputState a : keys) {
            for (AnalogInputState b : keys) {
                for (std::int32_t ts = from; ts < to; ts += 10) {
                    if (a == b && ts != from) continue;
                    std::vector<PhysicsSandboxInputEvent> win = kept;
                    win.push_back(Steer(from, a));
                    if (ts != from) win.push_back(Steer(ts, b));
                    win.push_back(Steer(to, lastSteer));
                    std::stable_sort(win.begin(), win.end(),
                                     [](const auto &l, const auto &r) { return l.timeMs < r.timeMs; });
                    if (!sandbox.RestoreState(snapshot.Value())) return Fail("restore");
                    if (!sandbox.ReplaceInputWindow(from, to + 10, win)) return Fail("window");
                    float vmax = 0, v = 0;
                    for (std::uint32_t i = 0; i < horizon; ++i) {
                        auto r = sandbox.AdvanceTicks(1);
                        if (!r) break;
                        v = Speed(r.Value().car) * 3.6f;
                        vmax = std::max(vmax, v);
                    }
                    std::printf("%d,%d,%d,%.1f,%.1f\n", a, b, ts, v, vmax);
                }
            }
        }
        return 0;
    }

    if (mode == "seam" && argc >= 8) {
        const std::int64_t t0 = std::strtoll(argv[4], nullptr, 10);
        const std::int64_t steerFrom = std::strtoll(argv[5], nullptr, 10);
        const std::int64_t steerTo = std::strtoll(argv[6], nullptr, 10);
        const std::uint32_t horizon = std::strtoul(argv[7], nullptr, 10);
        if (!advanceTo(static_cast<std::uint64_t>(std::min(t0, steerFrom)) - 10)) return Fail("advance");
        auto snapshot = sandbox.CaptureState();
        if (!snapshot) return Fail("capture");
        auto baseInputs = sandbox.ReadInputs();
        if (!baseInputs) return Fail("inputs");
        // last steer value before steerTo, restored at steerTo
        AnalogInputState lastSteer = 0;
        for (const auto &e : baseInputs.Value()) {
            if (e.action == PhysicsSandboxInputAction::Steer && e.timeMs <= steerTo &&
                e.value.kind == PhysicsSandboxInputValueKind::Analog) {
                lastSteer = e.value.analog;
            }
        }
        std::printf("steer,max_pitch_down,min_pitch,max_abs_roll,peak_wx,peak_wz,kmh_end,airborne_ticks\n");
        auto envInt = [](const char *name, int fallback) {
            const char *value = std::getenv(name);
            return value != nullptr ? std::atoi(value) : fallback;
        };
        const int steerMin = envInt("FVLAB_STEER_MIN", -65536);
        const int steerMax = envInt("FVLAB_STEER_MAX", 65536);
        const int steerStep = std::max(1, envInt("FVLAB_STEER_STEP", 4096));
        for (int v = steerMin; v <= steerMax; v += steerStep) {
            std::vector<PhysicsSandboxInputEvent> win;
            win.push_back(Steer(static_cast<std::int32_t>(steerFrom), v));
            win.push_back(Steer(static_cast<std::int32_t>(steerTo), lastSteer));
            auto restored = sandbox.RestoreState(snapshot.Value());
            if (!restored) {
                std::fprintf(stderr, "%s\n", restored.Error().diagnostic.c_str());
                return Fail("restore");
            }
            auto replaced = sandbox.ReplaceInputWindow(steerFrom, steerTo, win);
            if (!replaced) {
                std::fprintf(stderr, "%s\n", replaced.Error().diagnostic.c_str());
                return Fail("window");
            }
            float maxPitch = -1e9f, minPitch = 1e9f, maxRoll = 0, peakWx = 0, peakWz = 0;
            int air = 0;
            PhysicsSandboxStateView s{};
            for (std::uint32_t i = 0; i < horizon; ++i) {
                auto r = sandbox.AdvanceTicks(1);
                if (!r) break;
                s = r.Value();
                Pose p = PoseOf(s.car);
                maxPitch = std::max(maxPitch, p.pitchDeg);
                minPitch = std::min(minPitch, p.pitchDeg);
                maxRoll = std::max(maxRoll, std::fabs(p.rollDeg));
                if (std::fabs(p.localW[0]) > std::fabs(peakWx)) peakWx = p.localW[0];
                if (std::fabs(p.localW[2]) > std::fabs(peakWz)) peakWz = p.localW[2];
                bool any = s.car.wheelContact[0] || s.car.wheelContact[1] || s.car.wheelContact[2] ||
                           s.car.wheelContact[3];
                if (!any) ++air;
            }
            std::printf("%d,%.2f,%.2f,%.2f,%.3f,%.3f,%.1f,%d\n", v, maxPitch, minPitch, maxRoll, peakWx,
                        peakWz, Speed(s.car) * 3.6f, air);
            std::fflush(stdout);
        }
        return 0;
    }

    // fvlab PACKS REPLAY random SEED STARTS TICKS GAIN_KMH
    //   From STARTS evenly spaced replay times, drive TICKS ticks with random gas/brake/steer
    //   (new random input every 50-300 ms) and print every tick whose speed gain exceeds GAIN_KMH.
    // fvlab PACKS REPLAY replay SEED START_MS TICKS
    //   Re-runs one random variant (same generator) so it can be traced with FV_TRACE.
    if ((mode == "random" && argc >= 8) || (mode == "replay" && argc >= 7)) {
        const std::uint32_t seed = std::strtoul(argv[4], nullptr, 10);
        auto baseInputs = sandbox.ReadInputs();
        if (!baseInputs) return Fail("inputs");
        std::int32_t endMs = 0;
        for (const auto &e : baseInputs.Value()) endMs = std::max(endMs, e.timeMs);
        std::vector<std::int32_t> starts;
        std::uint32_t ticks = 0;
        float gainLimit = 0;
        if (mode == "random") {
            const int count = std::atoi(argv[5]);
            ticks = std::strtoul(argv[6], nullptr, 10);
            gainLimit = std::strtof(argv[7], nullptr);
            for (int i = 0; i < count; ++i) {
                starts.push_back((1000 + (endMs - 3000) * i / std::max(1, count)) / 10 * 10);
            }
        } else {
            starts.push_back(std::atoi(argv[5]));
            ticks = std::strtoul(argv[6], nullptr, 10);
        }
        std::printf("start,seed,timeMs,kmh_before,kmh_after,gain,gas,brake,steer,contacts\n");
        auto restart = sandbox.CaptureState();
        if (!restart) return Fail("capture");
        for (std::int32_t start : starts) {
            if (!sandbox.RestoreState(restart.Value())) return Fail("restore");
            sandbox.ReplaceInputs(baseInputs.Value());
            if (!advanceTo(static_cast<std::uint64_t>(start))) {
                std::fprintf(stderr, "advance %d failed\n", start);
                continue;
            }
            std::uint32_t rng = seed * 2654435761u + static_cast<std::uint32_t>(start);
            auto next = [&rng]() {
                rng ^= rng << 13;
                rng ^= rng >> 17;
                rng ^= rng << 5;
                return rng;
            };
            std::vector<PhysicsSandboxInputEvent> win;
            for (std::int32_t t = start; t < start + static_cast<std::int32_t>(ticks) * 10;) {
                auto mk = [&](PhysicsSandboxInputAction a, bool on) {
                    PhysicsSandboxInputEvent e;
                    e.timeMs = t;
                    e.action = a;
                    e.value.kind = PhysicsSandboxInputValueKind::Switch;
                    e.value.switchState = on ? PhysicsSandboxSwitchState::Pressed : PhysicsSandboxSwitchState::Released;
                    return e;
                };
                win.push_back(mk(PhysicsSandboxInputAction::Accelerate, (next() % 100) < 80));
                win.push_back(mk(PhysicsSandboxInputAction::Brake, (next() % 100) < 45));
                win.push_back(Steer(t, static_cast<AnalogInputState>(static_cast<int>(next() % 131073) - 65536)));
                t += 50 + static_cast<std::int32_t>((next() % 6) * 50);
            }
            auto replaced = sandbox.ReplaceInputWindow(start, start + ticks * 10, win);
            if (!replaced) {
                std::fprintf(stderr, "window %d: %s\n", start, replaced.Error().diagnostic.c_str());
                continue;
            }
            float prev = Speed(sandbox.ReadState().Value().car) * 3.6f;
            for (std::uint32_t i = 0; i < ticks; ++i) {
                auto r = sandbox.AdvanceTicks(1);
                if (!r) break;
                const auto &s = r.Value();
                float kmh = Speed(s.car) * 3.6f;
                if (mode == "replay" || kmh - prev > gainLimit) {
                    std::printf("%d,%u,%llu,%.1f,%.1f,%.1f,%.2f,%.2f,%.3f,%d%d%d%d\n", start, seed,
                                static_cast<unsigned long long>(s.timeMs), prev, kmh, kmh - prev, s.accelerate,
                                s.brake, s.steering, s.car.wheelContact[0], s.car.wheelContact[1],
                                s.car.wheelContact[2], s.car.wheelContact[3]);
                }
                prev = kmh;
                if (s.raceCompleted) break;
            }
            std::fflush(stdout);
        }
        return 0;
    }
    return Fail("bad mode or arguments");
}
