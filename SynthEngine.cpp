#include <vector>
#include "SynthEngine.h"
#include "Menu.h"

using namespace std;

float gAudioSampleRate = 44100.0f;

SignalPtr SynthPart::buildSynth(Registry* registry, SynthParameters *menu, int partId, ParameterInfo* delayParams[], Signal **fxBus) {

  registry->setPartAndVoiceTag(partId,0);

  // 1.059 is the half tone detune
  o1detune = new StaticSignal(registry, 1.0f);
  o1Oct = new StaticSignalDiscrete(registry, 1.0f); 
  o2Oct = new StaticSignalDiscrete(registry, 1.0f);
  o1vol = new StaticSignal(registry, 1.0f);
  o2vol = new StaticSignal(registry, 1.0f);
  o1wave = new StaticSignal(registry, 1.0f);
  o2wave = new StaticSignal(registry, 1.0f);
  o1pw = new StaticSignal(registry, 0.5f);
  o2pw = new StaticSignal(registry, 0.5f);

  subVol = new StaticSignal(registry, 0.0f);
  subPhase = new StaticSignal(registry, 1.0f);

  envA = new StaticSignal(registry, 0.0f);
  envD = new StaticSignal(registry, 68.0f);
  envS = new StaticSignal(registry, 0.5f);
  envR = new StaticSignal(registry, 68.0f);
  fenvA = new StaticSignal(registry, 13.0f);
  fenvD = new StaticSignal(registry, 71.0f);
  fenvS = new StaticSignal(registry, 0.3f);
  fenvR = new StaticSignal(registry, 71.0f);
  cutoff = new StaticSignal(registry, 0.1f);
  resonance = new StaticSignal(registry, 0.6f);
  envFilterAmount = new StaticSignal(registry, 0.49f);

  panSpread = new StaticSignal(registry, 0.0f);

  lfoFreq = new StaticSignal(registry, 1.f);
  lfoWave = new StaticSignalDiscrete(registry, 1);
  lfo = new LFO(registry, lfoFreq, lfoWave);
  lfoToPitch = new StaticSignal(registry, 0.f);
  lfoToCutoff = new StaticSignal(registry, 0.f);
  lfoToPW1 = new StaticSignal(registry, 0.f);
  lfoToPW2 = new StaticSignal(registry, 0.f);

  StaticSignal *delaySend = new StaticSignal(NULL, 0);
  reduction = new StaticSignal(NULL, 0);
  rateReduction = new StaticSignal(NULL, 1);
  overdriveGain = new StaticSignal(NULL, 1); 


  ParameterInfo *pO1Oct = new ParameterInfoDiscrete("1Oct", 0.0f, 4.0f, o1Oct, "o1Oct");
  ParameterInfo *pO2Oct = new ParameterInfoDiscrete("2Oct", 0.0f, 4.0f, o2Oct, "o2Oct");
  ParameterInfo *pDetune = new ParameterInfo(" Det", 0.931f, 1.059f, o1detune, "o1detune");
  ParameterInfo *pO1Vol = new ParameterInfo(" Vol", 0.0f, 1.0f, o1vol, "o1vol");
  ParameterInfo *pO2Vol = new ParameterInfo(" Vol", 0.0f, 1.0f, o2vol, "o2vol");
  ParameterInfo *pO1Wave = new ParameterInfo(" Wav", 0.0f, 1.0f, o1wave, "o1wave");
  ParameterInfo *pO2Wave = new ParameterInfo(" Wav", 0.0f, 1.0f, o2wave, "o2wave");
  ParameterInfo *pO1Pw = new ParameterInfo("1PW", 0.01f, 0.99f, o1pw, "o1pw");
  ParameterInfo *pO2Pw = new ParameterInfo("2PW", 0.01f, 0.99f, o2pw, "o2pw");
  ParameterInfo *pSubVol = new ParameterInfo(" Sub", 0.0f, 1.0f, subVol, "subVol");
  ParameterInfo *pSubPhase = new ParameterInfo(" sPh", 0.0f, 1.0f, subPhase, "subPhase");

  ParameterInfo *pCutoff = new ParameterInfo("Cut ", 0, 1, cutoff, "cutoff");
  ParameterInfo *pResonance = new ParameterInfo("Res ", 0, 1, resonance, "resonance");
  ParameterInfo *pEnvFilterAmount = new ParameterInfo("Env ", 0, 1, envFilterAmount, "l");
  ParameterInfo *pEnvA = new ParameterInfo("ampA", 0, 100, envA, "envA");
  ParameterInfo *pEnvD = new ParameterInfo(" D  ", 0, 100, envD, "envD");
  ParameterInfo *pEnvS = new ParameterInfo(" S  ", 0, 1, envS, "envS");
  ParameterInfo *pEnvR = new ParameterInfo(" R  ", 0, 100, envR, "envR");
  ParameterInfo *pFEnvA = new ParameterInfo("filA", 0, 100, fenvA, "fenvA");
  ParameterInfo *pFEnvD = new ParameterInfo(" D  ", 0, 100, fenvD, "fenvD");
  ParameterInfo *pFEnvS = new ParameterInfo(" S  ", 0, 1, fenvS, "fenvS");
  ParameterInfo *pFEnvR = new ParameterInfo(" R  ", 0, 100, fenvR, "fenvR");

  ParameterInfo *pPanSpread = new ParameterInfo("Pan  ", 0, 1, panSpread, "panSpread");

  std::vector<std::string> lfoTypes = {"saw", "tri"};
  ParameterInfo *pLfoWave = new ParameterInfoDiscrete("LFO  ", 0, 1, lfoWave, "lfoWave", lfoTypes);
  ParameterInfo *pLfoFreq = new ParameterInfo("Frq  ", 0.1, 10, lfoFreq, "lfoFreq");
  ParameterInfo *pLfoToPitch = new ParameterInfo("Vib  ", -0.12, 0.12, lfoToPitch, "lfoToPitch"); // TODO: try to avoid exp function
  ParameterInfo *pLfoToCutoff = new ParameterInfo("Cut  ", -1, 1, lfoToCutoff, "lfoToCutoff");
  ParameterInfo *pLfoToPW1 = new ParameterInfo("PWM  ", 0, 1, lfoToPW1, "lfoToPW1");
  ParameterInfo *pLfoToPW2 = new ParameterInfo("PWM  ", 0, 1, lfoToPW2, "lfoToPW2");

  ParameterInfo *pDelaySend = new ParameterInfo("DEL", 0, 1, delaySend, "delaySend");
  ParameterInfo *pReduction = new ParameterInfo("BIT", 0, 1, reduction, "bitred");
  ParameterInfo *pRateReduction = new ParameterInfo("SRR", 1, 20, rateReduction, "srred");
  ParameterInfo *pOverdriveGain = new ParameterInfo("OD", 0, 10, overdriveGain, "odGain");


  ParameterInfo *pDummy = new DummyParameterInfo("    ");

  menu->addPage(vector<ParameterInfo *>{ pO1Oct, pO1Wave, pO1Vol, pDetune }, 0);
  menu->addPage(vector<ParameterInfo *>{ pO1Pw, pLfoToPW1, pSubVol, pSubPhase}, 0);
  menu->addPage(vector<ParameterInfo *>{ pO2Oct, pO2Wave, pO2Vol, pDummy }, 0);
  menu->addPage(vector<ParameterInfo *>{ pO2Pw, pLfoToPW2, pDummy, pDummy }, 0); // noise, crossmod

  menu->addPage(vector<ParameterInfo *>{ pCutoff, pResonance, pEnvFilterAmount, pDummy }, 1);
  menu->addPage(vector<ParameterInfo *>{ pFEnvA, pFEnvD, pFEnvS, pFEnvR }, 2);
  menu->addPage(vector<ParameterInfo *>{ pEnvA, pEnvD, pEnvS, pEnvR }, 2);

  menu->addPage(vector<ParameterInfo *>{ pLfoWave, pLfoFreq, pLfoToPitch, pLfoToCutoff }, 3);
  menu->addPage(vector<ParameterInfo *>{ pPanSpread, pDummy, pDummy, pDummy }, 3);

  menu->addPage(vector<ParameterInfo *>{ delayParams[0], delayParams[1], delayParams[2], delayParams[3]}, 4);
  menu->addPage(vector<ParameterInfo *>{ pDelaySend, pOverdriveGain, pReduction, pRateReduction}, 4);


  for (int i = 0; i < maxNbVoices; i++) {
    registry->setPartAndVoiceTag(partId,i);
    createSynthVoice(i, registry);
  }

  registry->setPartAndVoiceTag(partId,0);
  Signal *outputSignal = new MixerStereo(registry, signals, maxNbVoices, 0.5f);
  outputSignal = new VcaStereo(registry, outputSignal, this->partVolume);

  *fxBus = new VCA(registry, outputSignal, delaySend); // FIXME: only takes the left channel so far

  return outputSignal;
}

void SynthPart::createSynthVoice(int i, Registry *registry) {

  // create the chain of 'Signals'
  StaticSignal *baseFreq = new StaticSignal(registry, midiToFreq(59));
  this->baseFreqs[i] = baseFreq;

  Signal *pwm1 = new VCA(registry, lfo, lfoToPW1);
  Signal *totalO1PW = new Mixer(registry, {o1pw, pwm1}, 1.0f);

  // pitch
  Signal *lfoFactor = new Mixer(registry, {new StaticSignal(registry, 1), new VCA(registry, lfo, lfoToPitch)}, 1);
  Signal *vibratoPitchFreq = new VCA(registry, baseFreq, lfoFactor);

  Signal *detunedPitchFreq = new VCA(registry, vibratoPitchFreq, o1detune);
  Signal *octavedFreq1 = new VCA(registry, detunedPitchFreq, new Octaver(registry, o1Oct));
  // lfo pitch mod: normally we would use 2^mod, but maybe we could use baseFreq * (1 + s * lfo)

  SawOsc *osc = new SawOsc(registry, octavedFreq1,totalO1PW, o1wave, subVol, subPhase);

  Signal *octavedFreq2 = new VCA(registry, vibratoPitchFreq, new Octaver(registry, o2Oct));

  Signal *pwm2 = new VCA(registry, lfo, lfoToPW2);
  Signal *totalO2PW = new Mixer(registry, {o2pw, pwm2}, 1.0f);

  SawOsc *osc2 = new SawOsc(registry, octavedFreq2, totalO2PW, o2wave, NULL, NULL);

  VCA *scaledOsc1 = new VCA(registry, osc, o1vol);
  VCA *scaledOsc2 = new VCA(registry, osc2, o2vol);

  Mixer *oscMixer = new Mixer(registry, { scaledOsc1, scaledOsc2 }, 1.0f);  
  //Mixer* oscMixer = new Mixer(registry, {saw}, 0.1f); // scale down so it doesn't distort

  Env *env = new Env(registry, envA, envD, envS, envR);
  this->envs[i] = env;
  Env *fenv = new Env(registry, fenvA, fenvD, fenvS, fenvR);
  this->fenvs[i] = fenv;

  Signal *totalCutoff = new Mixer(registry, { cutoff, new VCA(registry, fenv, envFilterAmount), new VCA(registry, lfo, lfoToCutoff) }, 1);

  // Filters:
  //Digital2Pole*  filter= new Digital2Pole(registry, oscMixer, totalCutoff, resonance);
  //SimperSVF *filter = new SimperSVF(registry, oscMixer, totalCutoff, resonance);
  FourPole *filter = new FourPole(registry, oscMixer, totalCutoff, resonance);


  VCA *vca = new VCA(registry, filter, env);

  Signal *od = new Overdrive(registry, vca, overdriveGain);
  Signal *bitred = new Reduction(registry, od, reduction, rateReduction);

  // distribute voices evenly over stereo width
  float position = -1 + 2.0f * (float(i) / this->maxNbVoices);
  //float position = -1 + 2.0f * (float(i) / this->activeNbVoices); // division by zero?
  Signal *sPosition = new VCA(registry, new StaticSignal(registry, position), panSpread);
  Signal *pan = new Pan(registry, bitred, sPosition);

  // store the last element of the chain -> output
  signals[i] = pan;
}

SignalPtr DrumPart::buildSynth(Registry* registry, SynthParameters *menu, int partId, ParameterInfo* delayParams[], Signal **fxBus) {
  registry->setPartAndVoiceTag(partId,0);

  StaticSignal *clickLPF = new StaticSignal(registry, 0.5f);
  StaticSignal *clickVol = new StaticSignal(registry, 1.0f);

  o1Oct = new StaticSignal(registry, 1.0f); 
  drumOscType = new StaticSignalDiscrete(registry, 0);
  pitchEnvDR = new StaticSignal(registry, 1.0f);
  envPitchAmount = new StaticSignal(registry, 0.0f); // Now in Octaves (0-5)

  ampEnvDR = new StaticSignal(registry, 5.0f);

  sinVol = new StaticSignal(registry, 0.5f);
  noiseVol = new StaticSignal(registry, 0.5f);

  hiPassCutoff = new StaticSignal(registry, 0.0f);
  hiPassRes = new StaticSignal(registry, 0.0f);
  //hiPassCutoff2 = new StaticSignal(registry, 0.0f);
  loPassCutoff = new StaticSignal(registry, 1.0f);

  overdriveGain = new StaticSignal(registry, 1.0f);
  StaticSignal *reductionAmount = new StaticSignal(NULL, 0);
  StaticSignal *rateReduction = new StaticSignal(NULL, 1);

  StaticSignal *hiPassCutoff2 = new StaticSignal(registry, 0.0f);
  StaticSignal *hiPassRes2 = new StaticSignal(registry, 0.0f);

  this->stutterEnable = new StaticSignalDiscrete(registry, 0);
  StaticSignal* stutterStart = new StaticSignal(registry, 0.0f);
  StaticSignal* stutterEnd = new StaticSignal(registry, 0.5f);
  this->stutterMove = new StaticSignal(registry, 0.0f);

  //StaticSignal* bpmTemp = new StaticSignalDiscrete(registry, 120); // TODO: take actual tempo of the sequencer here
  StaticSignalDiscrete *noiseType = new StaticSignalDiscrete(registry, 0);

  StaticSignal *delaySend = new StaticSignal(NULL, 0);

  /*StaticSignal *delayMs = new StaticSignal(NULL, 0);
  StaticSignalDiscrete *delayBeat = new StaticSignalDiscrete(NULL, 1);
  StaticSignal *delayFeedback = new StaticSignal(NULL, 0);
  StaticSignal *delayWet = new StaticSignal(NULL, 0);*/

  ParameterInfo *pO1Oct = new ParameterInfo("Oct", 0.25f, 4.0f, o1Oct, "o1Oct");
  ParameterInfo *pDrumOscType = new ParameterInfoDiscrete("TYP", 0, 3, drumOscType, "drumOscType");
  ParameterInfo *pPitchEnvDR = new ParameterInfo("PDR ", 0, 10, pitchEnvDR, "pitchEnvDR");
  ParameterInfo *pEnvPitchAmount = new ParameterInfo("Env ", 0, 5, envPitchAmount, "envPitchAmount"); // 0 to 5 octaves

  ParameterInfo *pAmpEnvDR = new ParameterInfo("ADR ", 0, 30, ampEnvDR, "ampEnvDR");

  ParameterInfo *pNoiseVol = new ParameterInfo("Noi ", 0, 1, noiseVol, "noiseVol");
  ParameterInfo *pSinVol = new ParameterInfo("Osc ", 0, 1, sinVol, "sinVol");

  ParameterInfo *pHiPassCutoff = new ParameterInfo("HP ", 0, 1, hiPassCutoff, "hiPassCutoff");
  ParameterInfo *pHiPassRes = new ParameterInfo("HPR ", 0, 1, hiPassRes, "hiPassRes");
  ParameterInfo *pLoPassCutoff = new ParameterInfo("LP ", 0, 1, loPassCutoff, "loPassCutoff");

  ParameterInfo *pOverdriveGain = new ParameterInfo("OD", 0, 10, overdriveGain, "overdriveGain");
  ParameterInfo *pReductionAmount = new ParameterInfo("BIT", 0, 1, reductionAmount, "bitRed"); 
  ParameterInfo *pRateReduction = new ParameterInfo("SRR", 1, 10, rateReduction, "srRed");

  ParameterInfo *pHiPassCutoff2 = new ParameterInfo("HP2", 0, 1, hiPassCutoff2, "hiPassCutoff2");
  ParameterInfo *pHiPassRes2 = new ParameterInfo("HPR ", 0, 1, hiPassRes2, "hiPassRes2");

  ParameterInfo *pStutterEnable = new ParameterInfoDiscrete("STU", 0, 1, stutterEnable, "stutterEnable");
  ParameterInfo *pStutterStart = new ParameterInfo("STA ", 0, 1, stutterStart, "stutterStart");
  ParameterInfo *pStutterEnd = new ParameterInfo("END ", 0, 1, stutterEnd, "stutterEnd");
  ParameterInfo *pStutterMove = new ParameterInfo("MOV ", -0.2, 0.2, stutterMove, "stutterMove");

  ParameterInfo *pNoiseType = new ParameterInfoDiscrete("NTP", 0, 2, noiseType, "noiseType");

  ParameterInfo *pClickVol = new ParameterInfo("CLK", 0, 1, clickVol, "clickVol");
  ParameterInfo *pClickLPF = new ParameterInfo("CLP", 0, 1, clickLPF, "clickLPF");

  ParameterInfo *pDelaySend = new ParameterInfo("DEL", 0, 1, delaySend, "delaySend");

  /*ParameterInfo *pDelayMs = new ParameterInfo("Ms", 0, 50, delayMs, "delayMs");
  ParameterInfo *pDelayBeat = new ParameterInfoDiscrete("del", 0, 4, delayBeat, "delayBeat");
  ParameterInfo *pDelayFb = new ParameterInfo("Fb", 0, 1, delayFeedback, "delayFeedback");
  ParameterInfo *pDelayWet = new ParameterInfo("Wet", 0, 1, delayWet, "delayWet");*/


  StaticSignal *dummyS = new StaticSignal(registry, 0.0f);
  ParameterInfo *pDummy = new DummyParameterInfo("    ");
  //ParameterInfo *pDummy = new ParameterInfo("....", 0, 0.1f, dummyS, "dummyS");
  //ParameterInfo *pDummy = new DummyParameterInfo("....");

  // pages: (global decay, filter) (pitch, pitchenv, amount, sinVol) (noiseVol, [noiseDecay]), (click?, fm, )
  //menu->addPage(vector<ParameterInfo *>{ pSinVol, pO1Oct, pPitchEnvDR, pEnvPitchAmount }, 0);
  //menu->addPage(vector<ParameterInfo *>{ pNoiseVol, pAmpEnvDR, pHiPassCutoff, pLoPassCutoff}, 1);
  menu->addPage(vector<ParameterInfo *>{ pSinVol, pNoiseVol, pClickVol, pAmpEnvDR }, 0);
  menu->addPage(vector<ParameterInfo *>{ pO1Oct, pDrumOscType, pPitchEnvDR, pEnvPitchAmount}, 1);

  menu->addPage(vector<ParameterInfo *>{ pNoiseType, pHiPassCutoff, pHiPassRes, pClickLPF }, 1);

  menu->addPage(vector<ParameterInfo *>{ pLoPassCutoff, pOverdriveGain,  pHiPassCutoff2, pHiPassRes2, pDummy }, 2);
  menu->addPage(vector<ParameterInfo *>{ pStutterEnable, pStutterStart, pStutterEnd, pStutterMove }, 3);
  menu->addPage(vector<ParameterInfo *>{ delayParams[0], delayParams[1], delayParams[2], delayParams[3]}, 4);
  menu->addPage(vector<ParameterInfo *>{ pDelaySend, pReductionAmount, pRateReduction, pDummy}, 4);

  // create the chain of 'Signals'
  StaticSignal *baseFreq = new StaticSignal(registry, midiToFreq(59));
  this->baseFreqs[0] = baseFreq;

  // pitch
  Signal *octavedFreq1 = new VCA(registry, baseFreq, o1Oct);
  // lfo pitch mod: normally we would use 2^mod, but maybe we could use baseFreq * (1 + s * lfo)

  DrumEnv *pitchEnv = new DrumEnv(registry, pitchEnvDR);
  DrumEnv *ampEnv = new DrumEnv(registry, ampEnvDR);

  // Modulation in the Pitch domain: BaseFreq * 2^(Env * Amount)
  Signal *envInOctaves = new VCA(registry, pitchEnv, envPitchAmount);
  Signal *totalFreq = new VCA(registry, octavedFreq1, new ContinuousOctaver(registry, envInOctaves));

  Signal* drumOsc = new MultiOsc(registry, totalFreq, drumOscType); // 

  Signal* osc = new VCA(registry, drumOsc, sinVol);


  Signal* noise = new VCA(registry, new MultiNoise(registry, noiseType),noiseVol);

  noise = new DigitalHiPass(registry, noise, hiPassCutoff, hiPassRes);

  this->envs[0] = ampEnv;
  this->fenvs[0] = pitchEnv; // TODO: misusing filter env slot for pitch env here

  this->click = new Click(registry);
  //Signal *clickLP = new Digital2Pole(registry, click, clickLPF, new StaticSignal(NULL, 0));
  Signal *clickVCA = new VCA(registry, this->click, clickVol);

  Signal *loPassed = new Digital2Pole(registry, new Mixer(registry, {noise, osc, clickVCA}, 1), loPassCutoff, dummyS);

  Signal *hip = new DigitalHiPass(registry, loPassed, hiPassCutoff2, hiPassRes2);

  VCA *vca = new VCA(registry, hip, ampEnv);

  Signal *over = new Overdrive(registry, vca , overdriveGain);
  Signal *bitRed = new Reduction(registry, over, reductionAmount, rateReduction);

  this->stutter = new Stutter(registry, bitRed, stutterStart, stutterEnd, this->bpm, this->stutterEnable, this->stutterMove);

  *fxBus = new VCA(registry, stutter, delaySend);

  Signal *output = new VCA(registry, stutter, this->partVolume);

  return output;
}

void Part::noteOn(int note, int velo) {
  //baseFreq->setValue(midiToFreq(note));
  //env->retrigger();

  // TODO:
  // - don't allocate multiple voices on the same note

  // Algo:
  // - OK when no voices left, don't trigger a note
  // - OK if possible allocate unused voice
  // - OK when not possible steal (retrigger) voices in release phase
  // - LRU logic, array with the last-released note, and 2 pointers to it

  /* kind of a juno poly assignment logic, almost like a mono synth as long as only one key is pressed
  int i=0;
  for (i=0;i<nbVoices;i++) { // loop once through all voices to find an empty one
    if(envs[nextVoice]->isDone()) {
      break;
    }
    nextVoice = (nextVoice + 1) % nbVoices;
  }

  if(!envs[nextVoice]->isDone()) { // we have not found an empty voice
    return;
  }

  baseFreqs[nextVoice]->setValue(midiToFreq(note));
  envs[nextVoice]->retrigger();
  fenvs[nextVoice]->retrigger();
  notes[nextVoice] = note;
  */

  if (nbAvailableVoices < 1) {
    return;  // no voice available
  }

  int selectedVoiceId = lru[leastRecentlyReleasedVoiceId];
  baseFreqs[selectedVoiceId]->setValue(midiToFreq(note));
  //envs[selectedVoiceId]->retrigger();
  //fenvs[selectedVoiceId]->retrigger();
  retriggerVoice(selectedVoiceId);
  notes[selectedVoiceId] = note;

  lru[leastRecentlyReleasedVoiceId] = -lru[leastRecentlyReleasedVoiceId];
  leastRecentlyReleasedVoiceId = (leastRecentlyReleasedVoiceId + 1) % activeNbVoices;
  nbAvailableVoices--;

  // debug logging
  /*
  for(int i=0;i<activeNbVoices;i++) {
    if (i==leastRecentlyReleasedVoiceId) {
      CBLog.print("E");
    }
    CBLog.print(lru[i]);
    CBLog.print("_");
  }
  CBLog.print("nbVoicesAvailable: ");
  CBLog.println(nbAvailableVoices);
  */
  
}

void Part::noteOff(int note, int velo) {

  // polyphonic case: kill all voices that are currently playing this notes
  for (int i = 0; i < maxNbVoices; i++) {
    if (notes[i] == note) {
      //envs[i]->release();
      //fenvs[i]->release();
      releaseVoice(i);
      notes[i] = -1;

      int returnedVoiceSpot = (leastRecentlyReleasedVoiceId + nbAvailableVoices) % activeNbVoices;
      lru[returnedVoiceSpot] = i;  // mark which voice it was that just got released
      nbAvailableVoices++;
    }
  }

  // debug logging
  /*
  for(int i=0;i<activeNbVoices;i++) {
    if (i==leastRecentlyReleasedVoiceId) {
      CBLog.print("E");
    }
    CBLog.print(lru[i]);
    CBLog.print("_");
  }
  CBLog.print("nbVoicesAvailable: ");
  CBLog.println(nbAvailableVoices);
  */
  
}

void SynthEngine::buildEngine(std::vector<SynthParameters*> allParams, StaticSignal *bpm) {

  // create parameters for the global modules, i.e. FX

  StaticSignal *delayMs = new StaticSignal(NULL, 0);
  StaticSignalDiscrete *delayBeat = new StaticSignalDiscrete(NULL, 1);
  StaticSignal *delayFeedback = new StaticSignal(NULL, 0);
  StaticSignal *delayVol = new StaticSignal(NULL, 1);

  ParameterInfo *pDelayMs = new ParameterInfo("Ms", 0, 50, delayMs, "delayMs");
  ParameterInfo *pDelayBeat = new ParameterInfoDiscrete("del", 0, 4, delayBeat, "delayBeat");
  ParameterInfo *pDelayFb = new ParameterInfo("Fb", 0, 1, delayFeedback, "delayFeedback");
  ParameterInfo *pDelayVol = new ParameterInfo("Vol", 0, 1, delayVol, "delayWet");

  // to pass the parameters to all the parts
  ParameterInfo* delayParams[4] = {pDelayBeat, pDelayMs, pDelayFb, pDelayVol};
  // to return the FX bus signal
  Signal *fxBus;

  for (int i=0;i<nbParts;i++) {

    this->partVolumes[i] = new StaticSignal(NULL, 1);
    parts[i] = new SynthPart(partVolumes[i]);
    parts[i+nbParts] = new DrumPart(partVolumes[i], bpm);

    signals[i]=parts[i]->buildSynth(&registry, allParams[i], i, delayParams, &fxBus);
    effectsBusSignals[i] = fxBus;
    signals[i+nbParts]=parts[i+nbParts]->buildSynth(&registry, allParams[i+nbParts], i+nbParts, delayParams, &fxBus);
    effectsBusSignals[i+nbParts] = fxBus;
    CBLog.println("created synth part");


  }

  registry.setPartAndVoiceTag(0,-1);

  Signal *fxBusMixer = new Mixer(&registry, effectsBusSignals, nbParts*nbPartTypes, 1.0f);
  signals[nbParts*nbPartTypes] = new GlobalDelay8bit(&registry, fxBusMixer, delayMs, bpm, delayBeat, delayFeedback, delayVol);
  outputSignal = new MixerStereo(&registry, signals, nbParts*nbPartTypes+1, 0.5f);
  //outputSignal = new MixerStereo(&registry, signals, nbParts*nbPartTypes, 0.5f);

  parts[0]->setActiveNbVoices(3);
  //parts[2]->setActiveNbVoices(1);
  //parts[3]->setActiveNbVoices(3);
  //parts[0 + 6]->setActiveNbVoices(1);
  parts[1 + 6]->setActiveNbVoices(1);
  parts[2 + 6]->setActiveNbVoices(1);
  parts[3 + 6]->setActiveNbVoices(1);

  markRequiredSignals();
}

void SynthEngine::renderBlock(double** outputs, int nFrames, int nChannels) {
  if (outputs == nullptr || nFrames <= 0 || nChannels <= 0) return;
  if (outputSignal == nullptr) return;

  const int availableChannels = std::max(0, nChannels);

  for (int i = 0; i < nFrames; i++) {
    // Update all active signals in dependency order once per sample.
    for (int s = 0; s < this->registry.nbActiveSignals; s++) {
#if defined(ARDUINO)
      this->registry.activeSignals[s]->update_instrumented();
#else
      this->registry.activeSignals[s]->update();
#endif
    }

    float mainL = this->outputSignal->getValue(0);
    float mainR = this->outputSignal->getValue(1);

    const float absL = std::abs(mainL);
    if (absL > this->maxSignalLevel) {
      this->maxSignalLevel = absL;
    }
    const float absR = std::abs(mainR);
    if (absR > this->maxSignalLevel) {
      this->maxSignalLevel = absR;
    }

    // Keep the same soft-clipping as the Teensy renderer.
    mainL = 0.5f * fast_tanh(mainL * 2.0f);
    mainR = 0.5f * fast_tanh(mainR * 2.0f);

    if (availableChannels > 0) {
      outputs[0][i] = static_cast<double>(mainL);
    }
    if (availableChannels > 1 && outputs[1] != nullptr) {
      outputs[1][i] = static_cast<double>(mainR);
    }

    // Optional direct part outs: channels 2..7 mirror Teensy USB channel routing.
    /*for (int part = 0; part < NB_PARTS; part++) {
      const int ch = part + 2;
      if (ch >= availableChannels || outputs[ch] == nullptr) continue;

      float direct = this->signals[part]->getValue() + this->signals[part + NB_PARTS]->getValue();
      direct = 0.5f * fast_tanh(direct * 2.0f);
      outputs[ch][i] = static_cast<double>(direct);
    }*/
  }
}

void SawOsc::update() {

  //float increment = freq->getValue() * rateInverse;
  float increment = freq->value * rateInverse;

  //float pw = pulseWidth->getValue();
  //float waveblend = this->waveblend->getValue();
  float pw = pulseWidth->value;
  float waveblend = this->waveblend->value;

  phase = phase + increment;
  if (phase > 1) {
    phase -= 1;
    subHalf = (subHalf + 1) % 2;
  }

  /*value = phase - 0.5f;
  value = value * 2;
  value = value - blep(phase, increment);  // assuming signal from -1 to 1
  value = value / 2;*/

  float tempblep = blep(phase, increment);
  float saw = phase - 0.5f;
  //saw = saw * 2;
  saw = saw - 0.5f * tempblep;  // blep is assuming signal from -1 to 1
  //saw = saw / 2;

  float pulse = (phase - pw <= 0.f) ? -1.f : 1.f;
  pulse = pulse - tempblep;
  pulse = pulse - blep2(phase, increment, pw);
  pulse = pulse * 0.5f;

  value = waveblend * saw + (1-waveblend) * pulse;

  if (subLevel != NULL) {
    //float sl = subLevel->getValue();
    float sl = subLevel->value;
    //if(sl > 0.1f) // disable sub if not needed
    { 
      //float sp = this->subPhase->getValue();
      float sp = this->subPhase->value;

      float subPhase = phase * 0.5f + subHalf * 0.5f;
      subPhase = subPhase + sp;
      if (subPhase > 1) subPhase -= 1;
      float subValue = (subPhase <= 0.5f) ? -1.f : 1.f;
      subValue = subValue - blep(subPhase, increment);
      subValue = subValue - blep2(subPhase, increment, 0.5f);
      subValue = subValue * 0.5f;
      value = value + subValue * sl;
    }
  }

}

inline float blep(float t, float dt) {
  if (t < dt) {
    float temp = t / dt;
    return temp + temp - temp * temp - 1.0f;

  } else if (t > 1.0f - dt) {
    float temp = (t - 1.0f) / dt;
    return temp * temp + temp + temp + 1.0f;

  } else {
    return 0;
  }
}


inline float  blep2(float t, float dt, float pw) { // second slope
  if ((t > pw) && (t < pw + dt)) { // second slope
      float temp = (t - pw) / dt;
      return  -temp-temp + temp*temp + 1.f;

  } else if ((t <= pw) && (t > pw - dt)) { // second slope
      float temp = (t - pw) / dt;
      return -temp*temp - temp-temp - 1.f;
  } else {
      return 0;
  }
}
