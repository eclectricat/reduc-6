#include <vector>
#include "SynthEngine.h"
#include "Menu.h"

using namespace std;

SignalPtr SynthPart::buildSynth(Registry* registry, SynthParameters *menu, int partId) {

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

  ParameterInfo *pLfoWave = new ParameterInfoDiscrete("LFO  ", 0, 1, lfoWave, "lfoWave");
  ParameterInfo *pLfoFreq = new ParameterInfo("Frq  ", 0.1, 10, lfoFreq, "lfoFreq");
  ParameterInfo *pLfoToPitch = new ParameterInfo("Vib  ", -0.12, 0.12, lfoToPitch, "lfoToPitch"); // TODO: try to avoid exp function
  ParameterInfo *pLfoToCutoff = new ParameterInfo("Cut  ", -1, 1, lfoToCutoff, "lfoToCutoff");
  ParameterInfo *pLfoToPW1 = new ParameterInfo("PWM  ", 0, 1, lfoToPW1, "lfoToPW1");
  ParameterInfo *pLfoToPW2 = new ParameterInfo("PWM  ", 0, 1, lfoToPW2, "lfoToPW2");


  StaticSignal *dummyS = new StaticSignal(registry, 1.0f);
  ParameterInfo *pDummy = new ParameterInfo("....", 0, 10, dummyS, "dummyS");

  menu->addPage(vector<ParameterInfo *>{ pO1Oct, pO1Wave, pO1Vol, pDetune }, 0);
  menu->addPage(vector<ParameterInfo *>{ pO1Pw, pLfoToPW1, pSubVol, pSubPhase}, 0);
  menu->addPage(vector<ParameterInfo *>{ pO2Oct, pO2Wave, pO2Vol, pDummy }, 0);
  menu->addPage(vector<ParameterInfo *>{ pO2Pw, pLfoToPW2, pDummy, pDummy }, 0); // noise, crossmod

  menu->addPage(vector<ParameterInfo *>{ pCutoff, pResonance, pEnvFilterAmount, pDummy }, 1);
  menu->addPage(vector<ParameterInfo *>{ pFEnvA, pFEnvD, pFEnvS, pFEnvR }, 2);
  menu->addPage(vector<ParameterInfo *>{ pEnvA, pEnvD, pEnvS, pEnvR }, 2);

  menu->addPage(vector<ParameterInfo *>{ pLfoWave, pLfoFreq, pLfoToPitch, pLfoToCutoff }, 3);

  menu->addPage(vector<ParameterInfo *>{ pPanSpread, pDummy, pDummy, pDummy }, 3);


  for (int i = 0; i < maxNbVoices; i++) {
    registry->setPartAndVoiceTag(partId,i);
    createSynthVoice(i, registry);
  }

  registry->setPartAndVoiceTag(partId,0);
  Signal *outputSignal = new MixerStereo(registry, signals, maxNbVoices, 0.5f);
  outputSignal = new VcaStereo(registry, outputSignal, this->partVolume);

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

  // distribute voices evenly over stereo width
  float position = -1 + 2.0f * (float(i) / this->maxNbVoices);
  //float position = -1 + 2.0f * (float(i) / this->activeNbVoices); // division by zero?
  Signal *sPosition = new VCA(registry, new StaticSignal(registry, position), panSpread);
  Signal *pan = new Pan(registry, vca, sPosition);

  // store the last element of the chain -> output
  signals[i] = pan;
}

SignalPtr DrumPart::buildSynth(Registry* registry, SynthParameters *menu, int partId) {
  registry->setPartAndVoiceTag(partId,0);

  StaticSignal *clickLPF = new StaticSignal(registry, 0.5f);
  StaticSignal *clickVol = new StaticSignal(registry, 1.0f);

  o1Oct = new StaticSignal(registry, 1.0f); 
  pitchEnvDR = new StaticSignal(registry, 1.0f);
  envPitchAmount = new StaticSignal(registry, 1.0f);

  ampEnvDR = new StaticSignal(registry, 5.0f);

  sinVol = new StaticSignal(registry, 0.5f);
  noiseVol = new StaticSignal(registry, 0.5f);

  hiPassCutoff = new StaticSignal(registry, 0.0f);
  hiPassRes = new StaticSignal(registry, 0.0f);
  //hiPassCutoff2 = new StaticSignal(registry, 0.0f);
  loPassCutoff = new StaticSignal(registry, 1.0f);

  overdriveGain = new StaticSignal(registry, 1.0f);

  StaticSignal *hiPassCutoff2 = new StaticSignal(registry, 0.0f);
  StaticSignal *hiPassRes2 = new StaticSignal(registry, 0.0f);

  StaticSignal* stutterFraction = new StaticSignalDiscrete(registry, 0);
  //StaticSignal* bpmTemp = new StaticSignalDiscrete(registry, 120); // TODO: take actual tempo of the sequencer here
  StaticSignalDiscrete *noiseType = new StaticSignalDiscrete(registry, 0);

  StaticSignal *delayMs = new StaticSignal(NULL, 0);
  StaticSignalDiscrete *delayBeat = new StaticSignalDiscrete(NULL, 1);
  StaticSignal *delayFeedback = new StaticSignal(NULL, 0);
  StaticSignal *delayWet = new StaticSignal(NULL, 0);

  ParameterInfo *pO1Oct = new ParameterInfo("Oct", 0.25f, 4.0f, o1Oct, "o1Oct");
  ParameterInfo *pPitchEnvDR = new ParameterInfo("PDR ", 0, 10, pitchEnvDR, "pitchEnvDR");
  ParameterInfo *pEnvPitchAmount = new ParameterInfo("Env ", 0, 1000, envPitchAmount, "envPitchAmount");

  ParameterInfo *pAmpEnvDR = new ParameterInfo("ADR ", 0, 30, ampEnvDR, "ampEnvDR");

  ParameterInfo *pNoiseVol = new ParameterInfo("Noi ", 0, 1, noiseVol, "noiseVol");
  ParameterInfo *pSinVol = new ParameterInfo("Sin ", 0, 1, sinVol, "sinVol");

  ParameterInfo *pHiPassCutoff = new ParameterInfo("HP ", 0, 1, hiPassCutoff, "hiPassCutoff");
  ParameterInfo *pHiPassRes = new ParameterInfo("HPR ", 0, 1, hiPassRes, "hiPassRes");
  ParameterInfo *pLoPassCutoff = new ParameterInfo("LP ", 0, 1, loPassCutoff, "loPassCutoff");

  ParameterInfo *pOverdriveGain = new ParameterInfo("OD", 1, 10, overdriveGain, "overdriveGain");

  ParameterInfo *pHiPassCutoff2 = new ParameterInfo("HP2", 0, 1, hiPassCutoff2, "hiPassCutoff2");
  ParameterInfo *pHiPassRes2 = new ParameterInfo("HPR ", 0, 1, hiPassRes2, "hiPassRes2");

  ParameterInfo *pStutterFraction = new ParameterInfoDiscrete("STU", 0, 16, stutterFraction, "stutterFraction");

  ParameterInfo *pNoiseType = new ParameterInfoDiscrete("TYP", 0, 2, noiseType, "noiseType");

  ParameterInfo *pClickVol = new ParameterInfo("CLK", 0, 1, clickVol, "clickVol");
  ParameterInfo *pClickLPF = new ParameterInfo("CLP", 0, 1, clickLPF, "clickLPF");

  ParameterInfo *pDelayMs = new ParameterInfo("Ms", 1, 100, delayMs, "delayMs");
  ParameterInfo *pDelayBeat = new ParameterInfoDiscrete("del", 0, 4, delayBeat, "delayBeat");
  ParameterInfo *pDelayFb = new ParameterInfo("Fb", 0, 1, delayFeedback, "delayFeedback");
  ParameterInfo *pDelayWet = new ParameterInfo("Wet", 0, 1, delayWet, "delayWet");


  StaticSignal *dummyS = new StaticSignal(registry, 0.0f);
  ParameterInfo *pDummy = new ParameterInfo("....", 0, 0.1f, dummyS, "dummyS");

  // pages: (global decay, filter) (pitch, pitchenv, amount, sinVol) (noiseVol, [noiseDecay]), (click?, fm, )
  //menu->addPage(vector<ParameterInfo *>{ pSinVol, pO1Oct, pPitchEnvDR, pEnvPitchAmount }, 0);
  //menu->addPage(vector<ParameterInfo *>{ pNoiseVol, pAmpEnvDR, pHiPassCutoff, pLoPassCutoff}, 1);
  menu->addPage(vector<ParameterInfo *>{ pSinVol, pNoiseVol, pClickVol, pAmpEnvDR }, 0);
  menu->addPage(vector<ParameterInfo *>{ pO1Oct, pPitchEnvDR, pEnvPitchAmount, pClickLPF }, 1);
  menu->addPage(vector<ParameterInfo *>{ pHiPassCutoff, pHiPassRes, pLoPassCutoff, pNoiseType }, 2);
  menu->addPage(vector<ParameterInfo *>{ pStutterFraction, pOverdriveGain, pHiPassCutoff2, pHiPassRes2}, 3);
  menu->addPage(vector<ParameterInfo *>{ pDelayMs, pDelayBeat, pDelayFb, pDelayWet}, 4);

  // create the chain of 'Signals'
  StaticSignal *baseFreq = new StaticSignal(registry, midiToFreq(59));
  this->baseFreqs[0] = baseFreq;

  // pitch
  Signal *octavedFreq1 = new VCA(registry, baseFreq, o1Oct);
  // lfo pitch mod: normally we would use 2^mod, but maybe we could use baseFreq * (1 + s * lfo)

  Env *pitchEnv = new Env(registry, dummyS, pitchEnvDR, dummyS, pitchEnvDR);
  Env *ampEnv = new Env(registry, dummyS, ampEnvDR, dummyS, ampEnvDR);

  Signal *totalFreq = new Mixer(registry, { octavedFreq1, new VCA(registry, pitchEnv, envPitchAmount)}, 1);

  //Signal *osc = new SawOsc(registry, totalFreq,new StaticSignal(registry, 0.5f), new StaticSignal(registry, 0), dummyS, dummyS);
  LFO* lfoAsOsc = new LFO(registry, totalFreq, new StaticSignalDiscrete(registry, 1));
  lfoAsOsc->subsample = 1; // update at audio freq

  Signal* osc = new VCA(registry, lfoAsOsc, sinVol);

  //Signal* noise = new VCA(registry, new NoiseOsc(registry),noiseVol);
  //Signal* noise = new VCA(registry, new Noise808(registry),noiseVol);
  Signal* noise = new VCA(registry, new MultiNoise(registry, noiseType),noiseVol);

  noise = new DigitalHiPass(registry, noise, hiPassCutoff, hiPassRes);
  noise = new Digital2Pole(registry, noise, loPassCutoff, dummyS);
  
  this->envs[0] = ampEnv;
  this->fenvs[0] = pitchEnv; // TODO: misusing filter env slot for pitch env here

  this->click = new Click(registry);
  Signal *clickLP = new Digital2Pole(registry, click, clickLPF, new StaticSignal(NULL, 0));
  Signal *clickVCA = new VCA(registry, clickLP, clickVol);

  
  Signal *hip = new DigitalHiPass(registry, new Mixer(registry, {noise, osc, clickVCA}, 1), hiPassCutoff2, hiPassRes2);

  VCA *vca = new VCA(registry, hip, ampEnv);

  Signal *over = new Overdrive(registry, vca , overdriveGain);

  this->stutter = new Stutter(registry, over, stutterFraction, this->bpm);

  Signal *delay = new Delay(registry, this->stutter, delayMs, this->bpm, delayBeat, delayFeedback, delayWet);

  Signal *output = new VCA(registry, delay, this->partVolume);
  //Signal *output = this->stutter;

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
      Serial.print("E");
    }
    Serial.print(lru[i]);
    Serial.print("_");
  }
  Serial.print("nbVoicesAvailable: ");
  Serial.println(nbAvailableVoices);
  */
  
}

void Part::noteOff(int note, int velo) {

  // polyphonic case: kill all voices that are currently playing this notes
  for (int i = 0; i < maxNbVoices; i++) {
    if (notes[i] == note) {
      envs[i]->release();
      fenvs[i]->release();
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
      Serial.print("E");
    }
    Serial.print(lru[i]);
    Serial.print("_");
  }
  Serial.print("nbVoicesAvailable: ");
  Serial.println(nbAvailableVoices);
  */
  
}

void SynthEngine::update(void) {
  unsigned int i;  //, n;
  audio_block_t *block;
  audio_block_t *blockR;


  // allocate the audio blocks to transmit
  block = allocate();
  if (block == NULL) return;
  blockR = allocate();
  if (blockR == NULL) return;
  if (outputSignal == NULL) return;

  float currentSample[2];
  float tempval;

  for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {

    // update all Signals in order
    //for (int s = 0; s < registry.nbSignals; s++) {
    for (int s = 0; s < registry.nbActiveSignals; s++) {
      //registry.signals[s]->update();
      //registry.signals[s]->update_instrumented();
      registry.activeSignals[s]->update_instrumented();
      
    }

    //block->data[i] = (short)(55000 * signal->getValue()); // TODO: do all the scaling in a single place
    //currentSample[0] = outputSignal->getValue(0);
    //currentSample[1] = outputSignal->getValue(1);

    for (int s = 0; s < 2; s++) {
      tempval = outputSignal->getValue(s);
      if (abs(tempval) > maxSignalLevel) {
        maxSignalLevel = abs(tempval);
      }

      /*if (currentSample[s] > 0.5) {
        currentSample[s] = 0.5;
      } else if (currentSample[s] < -0.5) {
        currentSample[s] = -0.5;
      }*/
      //currentSample[s] = std::min(0.5f, std::max(-0.5f, currentSample[s]));
      //currentSample[s] = tempval <= -0.5f ? -0.5f :  tempval >= 0.5f ? 0.5f : tempval;
      currentSample[s] = 0.5f * fast_tanh(tempval * 2); // softclip
    }

    block->data[i] = (short)(55000 * currentSample[0]);    // TODO: do all the scaling in a single place
    blockR->data[i] = (short)(55000 * currentSample[1]);  // TODO: do all the scaling in a single place
  }
  transmit(block);
  transmit(blockR, 1);

  release(block);
  release(blockR);
}

void SynthEngine::buildEngine(std::vector<SynthParameters*> allParams, StaticSignal *bpm) {
//void SynthEngine::buildEngine(std::vector<SynthParameters*> allParams) {
  for (int i=0;i<nbParts;i++) {

    this->partVolumes[i] = new StaticSignal(NULL, 1);
    parts[i] = new SynthPart(partVolumes[i]);
    parts[i+nbParts] = new DrumPart(partVolumes[i], bpm);
    //parts[i] = new SynthPart();
    //parts[i+nbParts] = new DrumPart(bpm);
    //parts[i+nbParts] = new DrumPart();
    signals[i]=parts[i]->buildSynth(&registry, allParams[i], i);
    signals[i+nbParts]=parts[i+nbParts]->buildSynth(&registry, allParams[i+nbParts], i+nbParts);
    Serial.println("created synth part");

    
  }

  registry.setPartAndVoiceTag(0,-1);
  outputSignal = new MixerStereo(&registry, signals, nbParts*nbPartTypes, 0.5f);
  //outputSignal = new MixerStereo(&registry, signals, nbParts, 0.5f);

  parts[0]->setActiveNbVoices(3);
  //parts[2]->setActiveNbVoices(1);
  //parts[3]->setActiveNbVoices(3);
  //parts[0 + 6]->setActiveNbVoices(1);
  parts[1 + 6]->setActiveNbVoices(1);
  parts[2 + 6]->setActiveNbVoices(1);
  parts[3 + 6]->setActiveNbVoices(1);


  markRequiredSignals();
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