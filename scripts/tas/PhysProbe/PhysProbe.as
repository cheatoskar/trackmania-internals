// PhysProbe - own TMInterface plugin, MIT
// PhysProbe: logs one CSV line per physics tick (10 ms) for physics RE.
//
// Works in normal play (OnRunStep), replay validation and bruteforce/simulation (OnSimulationStep).
// Plugin API has no real file IO, so lines are collected in memory and written with
// CommandList::Save(), which writes a text file into Documents\TMInterface\Scripts\ (see README.md).
// Every API symbol used here is listed with its doc URL in README.md.
//
// Console variables (also editable in Settings > Plugins > PhysProbe):
//   physprobe_enabled       bool    log on/off
//   physprobe_file          string  output file name inside Scripts\
//   physprobe_burnout_only  bool    log only ticks with burnout_state != 0
//   physprobe_console       bool    also log() every line to the TMI console
//   physprobe_flush_ticks   double  rewrite the file every N logged lines (0 = only at end/finish/restart)
//   physprobe_max_lines     double  stop logging after N lines (memory guard)

// wheel order in the CSV: FrontLeft, FrontRight, BackRight, BackLeft (the named getters of SimulationWheels)

string csv = "";
uint lineCount = 0;
int lastRunTime = -1000000;
bool finishFlushed = false;
bool capWarned = false;
uint sinceFlush = 0;

PluginInfo@ GetPluginInfo()
{
    PluginInfo info;
    info.Author = "tm-physics-re";
    info.Name = "PhysProbe";
    info.Description = "Per-tick CSV log of car state (burnout_state, wheel contact/material/sliding/damper)";
    info.Version = "0.1";
    return info;
}

void Main()
{
    RegisterVariable("physprobe_enabled", true);
    RegisterVariable("physprobe_file", "physprobe.csv");
    RegisterVariable("physprobe_burnout_only", false);
    RegisterVariable("physprobe_console", false);
    RegisterVariable("physprobe_flush_ticks", 500);
    RegisterVariable("physprobe_max_lines", 200000);
    RegisterSettingsPage("PhysProbe", RenderSettings);
    ResetBuffer();
}

void OnDisabled()
{
    Flush();
}

void RenderSettings()
{
    UI::CheckboxVar("Enabled", "physprobe_enabled");
    UI::InputTextVar("Output file (in Scripts folder)", "physprobe_file");
    UI::CheckboxVar("Only when burnout_state != 0", "physprobe_burnout_only");
    UI::CheckboxVar("Also print lines to the console", "physprobe_console");
    UI::InputIntVar("Rewrite file every N lines (0 = at end only)", "physprobe_flush_ticks", 100);
    UI::InputIntVar("Max lines", "physprobe_max_lines", 10000);
    UI::TextDimmed("Lines buffered: " + lineCount);
    if (UI::Button("Write file now")) {
        Flush();
    }
}

// ---------------------------------------------------------------- buffer

string Header()
{
    string h = "ctx,race_time,speed_kmh,lv_x,lv_y,lv_z,yaw,pitch,roll,gas,brake,steer,burnout_state,is_sliding,has_lateral_contact";
    array<string> names = {"fl", "fr", "br", "bl"};
    for (uint i = 0; i < names.Length; i++) {
        string n = names[i];
        h += "," + n + "_ground," + n + "_material," + n + "_sliding," + n + "_damper";
    }
    return h + "\n";
}

void ResetBuffer()
{
    csv = Header();
    lineCount = 0;
    sinceFlush = 0;
    finishFlushed = false;
    capWarned = false;
}

void Flush()
{
    if (lineCount == 0) return;
    CommandList list;
    list.Content = csv;
    if (!list.Save(GetVariableString("physprobe_file"))) {
        log("PhysProbe: could not save " + GetVariableString("physprobe_file"), Severity::Error);
    }
    sinceFlush = 0;
}

// ---------------------------------------------------------------- sampling

string WheelFields(TM::SceneVehicleCar::SimulationWheel@ w)
{
    // Units: material = TM::PlugSurfaceMaterialId as int, damper = RTState.DamperAbsorbVal (wheel struct @180).
    return "," + (w.RTState.HasGroundContact ? 1 : 0)
         + "," + int(w.RTState.get_ContactMaterialId())
         + "," + (w.RTState.IsSliding ? 1 : 0)
         + "," + w.RTState.DamperAbsorbVal;
}

void Sample(SimulationManager@ simManager, int ctx)
{
    if (!GetVariableBool("physprobe_enabled")) return;

    int t = simManager.RaceTime;
    if (t < 0) return;                                  // countdown
    if (simManager.TickTime > simManager.RaceTime) return; // ticks after the finish repeat the last time

    TM::SceneVehicleCar@ car = simManager.SceneVehicleCar;

    // burnout_state: SceneVehicleCar.Burnout, the named API field for car offset 1692 (0x69c):
    // 0 none, 1 burnout, 2 donut, 3 after burnout (enum TM::SceneVehicleCar::BurnoutState).
    int burnout = int(car.Burnout);
    if (GetVariableBool("physprobe_burnout_only") && burnout == 0) return;

    if (double(lineCount) >= GetVariableDouble("physprobe_max_lines")) {
        if (!capWarned) {
            log("PhysProbe: physprobe_max_lines reached, no more lines are logged", Severity::Warning);
            capWarned = true;
        }
        return;
    }

    // linear speed of the car (m/s, world frame): Dyna.CurrentState.LinearSpeed; km/h = m/s * 3.6
    float speedKmh = simManager.Dyna.CurrentState.LinearSpeed.Length() * 3.6f;
    // car-local speed (x lateral, z forward): SceneVehicleCar.CurrentLocalSpeed = car offset 1804
    vec3 lv = car.CurrentLocalSpeed;
    // orientation in radians (same call as Shweetz/BfNoseposPlus.as, which converts with Math::ToDeg)
    float yaw = 0.0f; float pitch = 0.0f; float roll = 0.0f;
    simManager.Dyna.CurrentState.Location.Rotation.GetYawPitchRoll(yaw, pitch, roll);

    SimulationWheels@ wheels = simManager.Wheels;

    string line = "" + ctx + "," + t + "," + speedKmh
        + "," + lv.x + "," + lv.y + "," + lv.z
        + "," + yaw + "," + pitch + "," + roll
        // inputs as the car sees them: SceneVehicleCar.InputGas/InputBrake/InputSteer (car offsets 80/84/88)
        + "," + car.InputGas + "," + car.InputBrake + "," + car.InputSteer
        + "," + burnout
        + "," + (car.IsSliding ? 1 : 0)              // car offset 1576
        + "," + (car.HasAnyLateralContact ? 1 : 0);  // car offset 1500
    line += WheelFields(wheels.FrontLeft);
    line += WheelFields(wheels.FrontRight);
    line += WheelFields(wheels.BackRight);
    line += WheelFields(wheels.BackLeft);

    csv += line + "\n";
    lineCount++;
    sinceFlush++;
    if (GetVariableBool("physprobe_console")) {
        log(line);
    }

    double every = GetVariableDouble("physprobe_flush_ticks");
    if (every > 0 && double(sinceFlush) >= every) {
        Flush();
    }
}

// ---------------------------------------------------------------- callbacks

void OnRunStep(SimulationManager@ simManager)
{
    int t = simManager.RaceTime;
    if (t < lastRunTime) {
        // a new attempt started: write the previous one, then start a fresh buffer
        Flush();
        ResetBuffer();
    }
    if (t == lastRunTime) return;
    lastRunTime = t;

    Sample(simManager, 1);

    if (simManager.PlayerInfo.RaceFinished && !finishFlushed) {
        finishFlushed = true;
        Flush();
    }
}

void OnSimulationBegin(SimulationManager@ simManager)
{
    ResetBuffer();
}

void OnSimulationStep(SimulationManager@ simManager, bool userCancelled)
{
    Sample(simManager, 0);
}

void OnSimulationEnd(SimulationManager@ simManager, SimulationResult result)
{
    Flush();
}
