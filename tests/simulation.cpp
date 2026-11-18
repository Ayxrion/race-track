static int checks = 0;
static void check(bool ok, const char* name) {
  ++checks; if (!ok) { std::fprintf(stderr, "FAIL: %s\n", name); std::exit(1); }
}
static void setupTest(int preset = 0, int count = 8, int series = SER_FORMULA, uint32_t seed = 42) {
  _rng = seed; simTime = raceTime = 0; frameCount = 0;
  weatherOverride = WEATHER_SUNNY; showersOn = false; wetAmt = snowCover = 0;
  buildPresetTrack(preset); careerReset(); rosterOn = true; rosterSeed = 123;
  raceLaps = 10; startRace(count, (uint8_t)series, seed);
}
static void testRules() {
  setupTest();
  check(wakeStrength(10,0,14,6,150) == 0, "overlapping cars cannot draft");
  check(wakeStrength(40,7,14,6,150) == 0, "pulling out removes wake assistance");
  check(wakeStrength(40,0,14,6,150) > 0, "a following car has a real wake");
  check(wakeStrength(40,0,14,6,0) == 0, "no slipstream launch boost");
  Car a = cars[0], b = cars[1];
  a.passT=0; a.passCar=1; a.outMove=1; a.battlePhase=BP_COMMIT;
  endAttack(a);
  check(a.passCar == -1 && a.passT == 0 && a.outMove == 0 && a.passCd > 0, "expired attacks clean up and recover");
  check(!pairReady(a,0,b,1) && !pairReady(b,1,a,0), "pair cooldown protects both participants");
  raceTime = 5;
  check(pairReady(a,0,b,1), "pair can race again after recovery");
  Car stable = cars[0]; setupRacecraft(stable,999);
  check(stable.aggression == cars[0].aggression && stable.stApex == cars[0].stApex && stable.tyreCare == cars[0].tyreCare, "identity traits survive new race seeds");
  check(stable.aiRng != cars[0].aiRng, "race execution varies by seed");
  float dry=qualifyingLap(cars[0]); wetAmt=1;
  check(qualifyingLap(cars[0]) > dry, "qualifying reflects wet grip"); wetAmt=0;
  Car slower=cars[0]; slower.perf *= 0.95f;
  check(qualifyingLap(slower) > dry, "qualifying rewards underlying pace");
  uint32_t global = _rng, stream=77;
  { DriverRandomScope random(stream); frand(); }
  check(_rng == global && stream != 77, "driver randomness does not consume the shared stream");
  Car tyre=cars[0]; tyre.wets=false; tyre.compound=0;
  float soft=tyreGrip(tyre); tyre.compound=2;
  check(soft>tyreGrip(tyre) && TYRE_WEAR[0]>TYRE_WEAR[2],"soft grip comes with greater wear");
  tyre.wets=true; float wet=tyreGrip(tyre); tyre.compound=0;
  check(wet==tyreGrip(tyre),"dry compound does not grant wet-tyre grip");
  tyre.baseWearK=0.14f; tyre.wets=false; setStint(tyre,3);
  check(tyre.compound==0,"short finishing sprint can use soft tyres");
  setStint(tyre,8); check(tyre.compound==2,"long stint requires durable tyres");
}
static void testRecords() {
  setupTest(0,3);
  memset(&champ,0,sizeof(champ)); memset(&recs,0,sizeof(recs));
  champ.ver=recs.ver=CHAMP_VER; champ.season=1; champLive=true; champTest=false;
  for(int i=0;i<3;++i) { order[i]=i; cars[i].out=i==2; cars[i].finished=i!=2; gridSlot[i]=i+1; }
  champRecordRace();
  const RecRow* retired=recFind(cars[2].code,false);
  check(retired && retired->pods == 0 && retired->wins == 0 && resPts[2] == 0, "retired third place gets no podium or points");
  check(resPts[0]==25 && resPts[1]==18, "existing scoring remains intact");
  uint16_t points=champFind(cars[0].code,false)->pts;
  champSave(); memset(&champ,0,sizeof(champ)); champLoad();
  check(champFind(cars[0].code,false)->pts == points && career_ok, "championship and career persistence round trip");
  for(int i=0;i<3;++i) { cars[i].out=true; cars[i].finished=false; }
  const uint16_t wins=recFind(cars[0].code,false)->wins;
  champRecordRace();
  check(recFind(cars[0].code,false)->wins == wins, "an all-retired class has no winner");
  phase=RP_RACE; updateRace(0.033f);
  check(phase==RP_RESULTS,"all-retired race reaches results");
}
static void straightScenario() {
  setupTest(0,2); trk.n=512; trk.ds=10; trk.L=5120; trk.nCorners=0; trk.hasPits=false;
  for(int k=0;k<trk.n;++k) { tp[k]={}; tp[k].x=k*10.0f; tp[k].y=100; tp[k].tx=1; tp[k].v=tp[k].vc=V_TOP; }
  setupCars(2,SER_FORMULA,123); raceTime=20; scState=SC_NONE;
  for(int i=0;i<2;++i) {
    Car& c=cars[i]; c.s=c.dist=200+i*40.0f; c.d=c.dPath=i?4:-4; c.v=150;
    c.perf=c.skill=c.commit=1; c.driver=DRV; c.vTop=V_TOP; c.heading=0;
    c.react=0; c.nextCorner=0; c.passCar=-1; c.aiRng=123+i; c.wear=c.wearK=0;
    placeOnTrack(c);
  }
  updateOrder();
}
static void testFairness() {
  straightScenario(); Car initial[2]={cars[0],cars[1]};
  updateCars(0.033f,true,false); Car normal[2]={cars[0],cars[1]};
  cars[0]=initial[0]; cars[1]=initial[1];
  cars[0].pos=20; cars[0].dist-=trk.L; // rank and a whole lap deficit cannot buy pace
  updateCars(0.033f,true,false);
  check(fabsf(cars[0].v-normal[0].v)<1e-5f && fabsf(cars[0].vTop-normal[0].vTop)<1e-5f, "last place and lap deficit grant no speed or acceleration");
  cars[0]=initial[1]; cars[1]=initial[0]; updateOrder();
  updateCars(0.033f,true,false);
  check(fabsf(cars[1].v-normal[0].v)<1e-5f && fabsf(cars[0].v-normal[1].v)<1e-5f, "swapping update order preserves independent motion");
  float solo[120];
  cars[0]=initial[0]; cars[1]=initial[1];
  for(int t=0;t<120;++t) { updateCars(0.033f,true,false); solo[t]=cars[0].v; }
  cars[0]=initial[0]; cars[1]=initial[1];
  for(int t=0;t<120;++t) {
    cars[0].passCar=1; cars[0].passT=2.5f; cars[0].passSide=-1; cars[0].passCd=0; cars[0].pairUntil=0;
    updateCars(0.033f,true,false);
    check(cars[0].v <= solo[t]+0.001f, "repeated attack activation cannot manufacture straight-line pace");
  }
  straightScenario(); logRaceStart();
  for(int t=0;t<20;++t) { cars[0].dist=240+(t%2?1:-1); raceTime+=0.033f; updateOrder(); logRaceStep(); }
  check(logPasses==0,"alongside position chatter is not counted as passing");
  cars[0].dist=280;
  for(int t=0;t<20;++t) { raceTime+=0.033f; updateOrder(); logRaceStep(); }
  check(logPasses==1,"a clear sustained overtake is counted exactly once");
  straightScenario(); cars[0].passT=0.001f; cars[0].passAge=12; cars[0].passCar=1; cars[0].passSide=-1;
  updateCars(0.033f,true,false);
  check(cars[0].passCar==-1 && cars[0].passCd>0,"timed-out attack recovers in the actual simulation");
  straightScenario(); cars[0].passT=1; cars[0].passCar=1; cars[0].out=true;
  updateCars(0.033f,true,false);
  check(cars[0].passCar==-1,"retirement cancels an active battle");
}
static void testRaces(int count) {
  int poles=0, gains=0, totalPasses=0, retirements=0;
  for(int r=0;r<count;++r) {
    setupTest(r%NUM_PRESETS, 8+(r%3)*3, r%SER_COUNT, 101+r*97);
    const int weather=(r/SER_COUNT)%3;
    if(weather==1) { weatherOverride=WEATHER_RAINY; startRace(numCars,raceSeries,101+r*97); }
    if(weather==2) { weatherOverride=WEATHER_FOGGY; startRace(numCars,raceSeries,101+r*97); }
    int steps=0;
    while(phase != RP_RESULTS && steps++ < 40000) {
      simTime+=0.033f; ++frameCount; testSurfaceWeather(); updateRace(0.033f);
      for(int i=0;i<numCars;++i) {
        check(std::isfinite(cars[i].v) && std::isfinite(cars[i].dist) && std::isfinite(cars[i].d), "race remains finite");
        if (!(cars[i].v >= 0 && cars[i].v < 400)) std::fprintf(stderr,"Bad speed race %d car %d v %f state %d pit %d time %f\n",r,i,cars[i].v,cars[i].state,cars[i].pit,raceTime);
        check(cars[i].v >= 0 && cars[i].v < 400, "speed remains physically bounded");
      }
    }
    check(phase==RP_RESULTS, "race reaches results without hanging");
    poles += order[0]==poleCar;
    gains += gridSlot[order[0]] >= numCars/2;
    totalPasses+=logPasses;
    for(int i=0;i<numCars;++i) retirements += cars[i].out;
    int stops=0; for(int i=0;i<numCars;++i) stops+=cars[i].pitDone;
    std::printf("Race %d: %s, %s, %s grid %d, passes %d, stops %d, time %.1f\n",r+1,trk.name,SERIES_NAMES[raceSeries],cars[order[0]].code,gridSlot[order[0]],logPasses,stops,raceTime);
  }
  std::printf("Batch: %d races, %d pole wins, %d wins from back half, %d passes, %d retirements\n",count,poles,gains,totalPasses,retirements);
}
static void testRecovery() {
  int wins=0, podiums=0;
  for(int r=0;r<NUM_PRESETS*6;++r) {
    setupTest(r%NUM_PRESETS,8,SER_FORMULA,521+r*73);
    const int scenario=(r/NUM_PRESETS)%3;
    if(scenario==1) { weatherOverride=WEATHER_RAINY; startRace(8,SER_FORMULA,521+r*73); }
    const int quick=poleCar, last=order[numCars-1];
    // A poor qualifying result, with exactly the same underlying driver/car.
    const int ids[2]={quick,last};
    for(int k=0;k<2;++k) {
      Car& c=cars[ids[k]]; int slot=k==0?numCars-1:0;
      c.dist=gridSlotS(slot); c.s=wrapS(c.dist); c.d=c.dPath=gridSlotD(slot);
      placeOnTrack(c); TrackFrame f; trackAt(c.s,f); c.heading=atan2f(f.ty,f.tx);
      c.px=c.x; c.py=c.y;
    }
    updateOrder();
    int steps=0;
    while(phase!=RP_RESULTS && steps++<40000) {
      if(scenario==1 && cars[order[0]].lap>=3) weatherOverride=WEATHER_SUNNY;
      if(scenario==2 && cars[order[0]].lap>=3) weatherOverride=WEATHER_RAINY;
      simTime+=0.033f; ++frameCount; testSurfaceWeather(); updateRace(0.033f);
    }
    check(phase==RP_RESULTS,"recovery scenario finishes");
    wins+=cars[quick].pos==1; podiums+=cars[quick].pos<=3;
    std::printf("Recovery: %s, P8 to P%d\n",trk.name,cars[quick].pos);
  }
  std::printf("Recovery batch: %d wins, %d podiums from last, without pace bonuses\n",wins,podiums);
  check(podiums>0,"a quick driver can recover from the back");
}
static void testFieldLimits() {
  for(int n : {2, MAX_CARS}) {
    setupTest(0,n,SER_ENDURANCE,567+n);
    buildRandomTrack(812+n);
    weatherOverride=WEATHER_SNOWY; raceLaps=0; startRace(n,SER_ENDURANCE,567+n);
    for(int t=0;t<1200;++t) {
      simTime+=0.033f; ++frameCount; testSurfaceWeather(); updateRace(0.033f);
      for(int i=0;i<n;++i) check(std::isfinite(cars[i].v) && std::isfinite(cars[i].dist),"generated snowy circuit stays finite at field-size limits");
    }
    check(!allowRetire && phase==RP_RACE,"endless races remain endless at both field-size limits");
  }
  setupTest(0,8); simLaps=3;
  for(int i=0;i<numCars;++i) { planTyres(cars[i],i); check(cars[i].compound==0 && cars[i].pitLap<0,"short races select a tyre that can reach the flag without a stop"); }
}
int main(int argc, char** argv) {
  allocTrack(); testRules(); testRecords(); testFairness(); testFieldLimits(); testRaces(argc>1?std::atoi(argv[1]):18);
  if(argc>2) testRecovery();
  std::printf("PASS: %d simulation checks\n",checks);
}
