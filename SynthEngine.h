#ifndef SYNTHENGINE_H
#define SYNTHENGINE_H


#include <Arduino.h>
#include <AudioStream.h>
#include <initializer_list>
#include <algorithm>
#include <cmath>
#include <bits/stl_tree.h>
#include <bits/stl_map.h>
#include <arm_math.h>
#include <vector>

#include "Utils.h"
#include <ArduinoJson.h>



class Signal;  // fw dec
class Registry;
class Menu;
class SynthParameters;
class Env;
class LFO;
typedef Signal* SignalPtr;

extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char* __brkval;

#define NB_PARTS 6
#define NB_PART_TYPES 2


class Registry {

public:
  void enroll(SignalPtr newSignal) {

    signals[nbSignals] = newSignal;
    partIds[nbSignals] = partId;
    voiceIds[nbSignals] = voiceId;
    nbSignals++;
    Serial.print("Enrolled Signal; now in total ");
    Serial.println(nbSignals);
    Serial.println(freeram());
  }

  int freeram() {
    return (char*)&_heap_end - __brkval;
  }

  // debugging
  SignalPtr* getSignals() {
    return signals;
  }
  int getNbSignals() {
    return nbSignals;
  }

  // signals registered after this call will be tagged with these ids
  // partId needs to be valid
  // voice id can be -1 (always active), a very large number, always inactive (but for static signals it does not matter)
  void setPartAndVoiceTag(int partId, int voiceId) {
    this->partId = partId;
    this->voiceId = voiceId;
  }

private:
  const static int maxNumSignals = 10000;
  SignalPtr signals[maxNumSignals];  // all signals
  int partIds[maxNumSignals];        //for each signal: which part and voice does it belong to?
  int voiceIds[maxNumSignals];
  int nbSignals = 0;  // total number of signals registered

  // during registration of the signals, the signals will be tagged with the part/voice that is set here
  // voice -1 means it always active, e.g. for global mixer signals etc
  // this only works because there is no multithreading
  int partId = 0;
  int voiceId = 0;


  SignalPtr activeSignals[maxNumSignals];
  int nbActiveSignals = 0;

  friend class SynthEngine;
};

// instrumentation
extern std::map<String, int> profiling;
extern JsonDocument jprofiling;


class Signal {

public:
  Signal(Registry* r) {  // part id and voice id
    if (r != NULL) {
      r->enroll(this);
    }
  }

  virtual void update_instrumented() {
    //int start = micros();
    uint32_t start = ARM_DWT_CYCCNT;
    update();
    lastSpentTime += ARM_DWT_CYCCNT - start;
    //profiling[signame()]=duration ; // profiling[signame()] + duration;
    //profiling["test"]=duration ; // profiling[signame()] + duration;
  }

  virtual String signame() const {
    return "signal";
  }

  virtual void update() = 0;
  virtual float getValue(int channel = 0) = 0;


  uint32_t getAndResetLastSpentTime() {
    uint32_t result = lastSpentTime;
    lastSpentTime = 0;
    return result;
  }

  float value = 0;  // make this public for speed reasons

protected:
  float rateInverse = 1 / AUDIO_SAMPLE_RATE;

  uint32_t lastSpentTime = 0;  // save time that was spent in the last update() of this object
};


float blep(float t, float dt);
float blep2(float t, float dt, float pw);

class SawOsc : public Signal {
public:
  SawOsc(Registry* r, Signal* f, Signal* pw, Signal* wave, Signal* subL, Signal* subP)
    : Signal(r) {
    freq = f;
    pulseWidth = pw;
    waveblend = wave;
    subLevel = subL;
    subPhase = subP;
    value = 0;
    phase = 0;
  }

  void update() override;

  float getValue(int channel = 0) {
    return value;
  }

  virtual String signame() const {
    return "osc";
  }

private:
  Signal* freq;
  Signal* pulseWidth;
  Signal* waveblend;
  Signal* subLevel = NULL;
  Signal* subPhase = NULL;
  //float value;
  float phase;
  int subHalf = 0;  // 0 or 1
};

class StaticSignal : public Signal {
public:
  StaticSignal(Registry* r, float v)
    : Signal(r) {
    value = v;
  }

  void update() {}

  float getValue(int channel = 0) {
    return value;
  }

  virtual void setValue(float newValue) {
    value = newValue;
  }

  virtual String signame() const {
    return "static";
  }

protected:
  //float value;
};

// to store discrete parameters (e.g. on/off) while still fulfilling Signal interface
class StaticSignalDiscrete : public StaticSignal {
public:
  StaticSignalDiscrete(Registry* r, float v)
    : StaticSignal(r, v) {
    setValue(v);
  }

  void setValue(float v) override {
    intValue = std::lround(v);
    value = intValue;
  }

  int getValueDiscrete() {
    return intValue;
  }

  virtual String signame() const {
    return "staticdiscrete";
  }

private:

  int intValue = 0;
};

class MixerStereo : public Signal {
public:
  MixerStereo(Registry* r, SignalPtr* signals, int nbSignals, float gain)
    : Signal(r) {

    // TODO check that we don't pass more than 10
    this->nbSignals = nbSignals;
    this->gain = gain;
    for (int i = 0; i < nbSignals; i++) {
      this->signals[i] = signals[i];
    }
  }

  MixerStereo(Registry* r, std::initializer_list<SignalPtr> signals, float gain)
    : Signal(r) {
    this->nbSignals = signals.size();
    this->gain = gain;
    int i = 0;
    std::initializer_list<SignalPtr>::iterator it;
    for (it = signals.begin(); it != signals.end(); it++, i++) {
      this->signals[i] = *it;
    }
  }

  float getValue(int channel = 0) {
    switch (channel) {
      case 0:
        return value;
        break;
      case 1:
        return value1;
        break;
      default:
        return value;
        break;
    }
    //return value;
  }

  void update() {
    value = 0;
    value1 = 0;
    for (int i = 0; i < nbSignals; i++) {
      value += signals[i]->getValue(0);
    }
    for (int i = 0; i < nbSignals; i++) {
      value1 += signals[i]->getValue(1);
    }
    value *= gain;
    value1 *= gain;
  }

  virtual String signame() const {
    return "mixerstereo";
  }

protected:
  int nbSignals;
  SignalPtr signals[15];
  //float value = 0;
  float value1 = 0;  // for stereo
  float gain = 1;
};

class Mixer : public MixerStereo {
public:
  Mixer(Registry* r, SignalPtr* signals, int nbSignals, float gain)
    : MixerStereo(r, signals, nbSignals, gain) {
  }

  Mixer(Registry* r, std::initializer_list<SignalPtr> signals, float gain)
    : MixerStereo(r, signals, gain) {
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {
    value = 0;
    for (int i = 0; i < nbSignals; i++) {
      //value += signals[i]->getValue(0);
      value += signals[i]->value;
    }
    value *= gain;
  }

  virtual String signame() const {
    return "mixermono";
  }

private:
};

// take a mono signal as input, and distribute it to both channels according to pan position
class Pan : public Signal {
public:
  Pan(Registry* r, Signal* in, Signal* position)
    : Signal(r) {
    this->in = in;
    this->position = position;
  }

  void update() {
    float pos = this->position->getValue();
    pos = pos / 2 + 0.5f;

    float intemp = in->getValue();

    values[0] = (1 - pos) * intemp;
    values[1] = (pos)*intemp;
  }

  // position -1 is left, position 1 is right
  /*float getValue(int channel=0) {
      float pos = this->position->getValue();
      pos = pos/2 + 0.5f;

      if (channel==0) {
        return (1-pos) * in->getValue();
      } else {
        return (pos) * in->getValue();
      }

    }*/
  float getValue(int channel = 0) {
    /*if (channel==0) {
        return value0;
      } else {
        return value1;
      }*/
    return values[channel];
  }


  virtual String signame() const {
    return "pan";
  }


protected:
  Signal* in;
  Signal* position;

  //float value0 = 0;
  //float value1 = 0;
  float values[2] = { 0, 0 };
};


// this is just a multiplication of 2 signals
// let's keep it generic and allow negative values for gain as well
class VCA : public Signal {
public:
  VCA(Registry* r, Signal* signal, Signal* gain)
    : Signal(r) {
    this->signal = signal;
    this->gain = gain;
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {
    //value = signal->getValue() * gain->getValue();
    value = signal->value * gain->value;
  }

  virtual String signame() const {
    return "vca";
  }

protected:
  Signal* signal;
  Signal* gain;
  //float value = 0;
};


class VcaStereo : public VCA {
  public:
   VcaStereo(Registry* r, Signal* signal, Signal* gain): VCA(r, signal, gain) {}

  float getValue(int channel = 0) {
    if (channel == 0 ) {
      return value;
      } else {
      return valueR;
    }
  }

  void update() {
    value = signal->getValue(0) * gain->getValue(0);
    valueR = signal->getValue(1) * gain->getValue(0);
  }

  virtual String signame() const {
    return "vcaStereo";
  }

  private:
  float valueR = 0;
};


class NoiseOsc : public Signal {
public:
  NoiseOsc(Registry* r)
    : Signal(r) {
    
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {
   
    double b_noiselast = b_noise;
    b_noise = b_noise + 19;
    b_noise = b_noise * b_noise;
    b_noise = b_noise + ((-b_noise + b_noiselast) * 0.5);
    int i_noise = (int)b_noise; // b_noise.floor.toInt;
    b_noise = b_noise - i_noise;

    value = ((float) b_noise) - 0.5f;
  }

  virtual String signame() const {
    return "noiseOsc";
  }

private:
  double b_noise = 19.1919191919191919191919191919191919191919;
};

class Noise808 : public Signal {
public:
  Noise808(Registry* r)
    : Signal(r) {
    
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {

    value = 0;

    for (int i=0;i<nbOscs;i++) {
      float p = seconds_since_start * 100 * frequencies[i]; // cycles since start
      p = p - (long)p; // phase
      value = value + (p < 0.5f);
    }
    value = value * 0.8f;
    seconds_since_start += seconds_per_sample; // I think if this grows too much it is not precise enough anymore, so reset it every once in a while
    if(seconds_since_start > 10) seconds_since_start = 0;
    
  }

  virtual String signame() const {
    return "noise808";
  }

private:
  float frequencies[6] = {1.0f, 1.1414f, 1.1962f, 2.1430f, 2.4961f, 2.0558f}; // * 100 Hz
  int nbOscs = 6;
  //float values[6];
  float seconds_since_start=0;
  float seconds_per_sample = 1.0f/AUDIO_SAMPLE_RATE;
};





// calculating power of 2
class Octaver : public Signal {
public:
  Octaver(Registry* r, StaticSignalDiscrete* exponent)
    : Signal(r) {
    this->exponent = exponent;
  }

  void update() {
    //value = std::pow(2, exponent->getValue());
    //value = fast_exp2f(exponent->getValue());

    switch (exponent->getValueDiscrete()) {
      case 0:
        value = 1;
        break;
      case 1:
        value = 2;
        break;
      case 2:
        value = 4;
        break;
      case 3:
        value = 8;
        break;
      case 4:
        value = 16;
        break;
      default:
        value = 1;
        break;
    }
  }

  float getValue(int channel = 0) {
    return value;
  }

  virtual String signame() const {
    return "octaver";
  }

protected:
  //Signal* exponent;
  StaticSignalDiscrete* exponent;
  //float value = 0;
};

class LFO : public Signal {
public:
  LFO(Registry* r, Signal* lfoFreq, StaticSignalDiscrete* lfoWave)
    : Signal(r) {
    freq = lfoFreq;
    wave = lfoWave;
    value = 0;
    phase = 0;
  }

  void update() {

    counter++;

    if (counter >= subsample) {
      counter = 0;

      float increment = freq->getValue() * rateInverse;
      phase = phase + increment * subsample;
      if (phase > 1) phase -= 1;
      if (phase < 0) phase += 1; // to support through zero mod

      switch (wave->getValueDiscrete()) {
        case 0:
          value = phase - 0.5f;
          break;
        case 1:
          value = (phase < 0.5f) ? phase : 1 - phase;
          value = (value * 2) - 0.5;
          break;
        default:
          value = 0;
          break;
      }
    }
  }

  virtual String signame() const {
    return "lfo";
  }

  float getValue(int channel = 0) {
    return value;
  }

  int subsample = 50;

private:
  Signal* freq;
  StaticSignalDiscrete* wave;
  //float value;
  float phase;

  int counter = 0;

};

class MultiNoise : public Signal {
public:
  MultiNoise(Registry* r, StaticSignalDiscrete *type)
    : Signal(r) {
      this->type = type;

      // TODO: not sure if this works with triangle waves
      modulator = new LFO(NULL, new StaticSignal(NULL, 233.0f), new StaticSignalDiscrete(NULL, 1));
      vca = new VCA(NULL, new StaticSignal(NULL, 4000.0f),  modulator);
      mixer = new Mixer(NULL, {vca, new StaticSignal(NULL, 200.0f)}, 1);
      carrier = new LFO(NULL, mixer, new StaticSignalDiscrete(NULL, 1)); 
      modulator->subsample=1;
      carrier->subsample=1;

      noise = new NoiseOsc(NULL);
      noise808 = new Noise808(NULL);

  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {

    //value = 0;
    if (type->getValueDiscrete() == 0) {
      noise->update();
      value = noise->value;
    } else if (type->getValueDiscrete() == 1) {
      noise808->update();
      value = noise808->value;
    } else {
    modulator->update();
    vca->update();
    mixer->update();
    carrier->update();
    value = carrier->value * 2;
    }
     
  }

  virtual String signame() const {
    return "multiNoise";
  }

private:
  StaticSignalDiscrete *type;

  NoiseOsc *noise;
  Noise808 *noise808;

  LFO *carrier;
  LFO *modulator;
  Signal *vca;
  Mixer *mixer;


};

class Env : public Signal {
public:
  Env(Registry* reg, Signal* a, Signal* d, Signal* s, Signal* r)
    : Signal(reg) {
    this->a = a;
    this->d = d;
    this->s = s;
    this->r = r;
  }

  void update() {
    switch (state) {
      case -1:
        aSecs = pow(this->convBaseA, this->a->getValue()) - 1;
        dSecs = pow(this->convBaseDR, d->getValue()) - 1;
        rSecs = pow(this->convBaseDR, r->getValue()) - 1;
        fDecay = pow(0.001f, 1.0f / (dSecs * AUDIO_SAMPLE_RATE));
        fRelease = pow(0.001f, 1.0f / (rSecs * AUDIO_SAMPLE_RATE));

        state = 0;
        break;

      case 0:
        {
          // increase by one in aSecs seconds,

          float increment = 1 / (aSecs * AUDIO_SAMPLE_RATE + 200.0f);
          //float increment = 1 / (this->a->getValue() * AUDIO_SAMPLE_RATE + 200.0f);
          value += increment;
          if (value >= 1.0) {
            value = 1.0f;
            state = 1;
          }
          break;
        }

      case 1:
        {

          //float fDecay = pow(0.001f, 1.0f / (dSecs * AUDIO_SAMPLE_RATE));
          //float fDecay = powf(0.001f, 1.0f / (d->getValue() * AUDIO_SAMPLE_RATE));
          float delta = (value - s->getValue()) * fDecay;
          value = s->getValue() + delta;
          // never actually switch officially to the sustain phase...
          break;
        }

      case 3:
        {

          //float fRelease = pow(0.001f, 1.0f / (rSecs * AUDIO_SAMPLE_RATE));
          //float fRelease = powf(0.001f, 1.0f / (r->getValue() * AUDIO_SAMPLE_RATE));
          value = value * fRelease;
          if (value <= 0.001f) {
            state = 4;
            value = 0;
          }
          break;
        }

      default:
        value = 0;  // do I need this?

    }  // switch

    value = std::min(1.0f, std::max(value, 0.0f));
  }  // update()

  void retrigger() {
    //state = 0;
    state = -1;
  }

  void release() {
    state = 3;
  }

  float getValue(int channel = 0) {
    return value;
  }

  bool isDone() {
    return (state >= 3);
  }

  virtual String signame() const {
    return "env";
  }

  

private:
  Signal *a, *d, *s, *r;
  //float value = 0;
  int state = 4;  // 0 attack, 1: decay, 2 sustain, 3: release, 4: end // -1: times not calculated yet

  const float convBaseA = 1.0180f;  // to convert the A params into seconds
  //const float convBaseDR = 1.0243f; // to convert the A params into seconds
  const float convBaseDR = 1.03f;  // to convert the DR params into seconds
  float aSecs = 1;
  float dSecs = 1;
  float rSecs = 1;
  float fDecay = 1;
  float fRelease = 1;
  
};

class Digital2Pole : public Signal {
public:
  Digital2Pole(Registry* r, Signal* in, Signal* cut, Signal* res)
    : Signal(r) {
    this->in = in;
    this->cut = cut;
    this->res = res;
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {
    float cut = this->cut->getValue();
    cut = std::min(1.0f, std::max(0.0f, cut));
    float in = this->in->getValue();
    float fb = this->res->getValue();
    fb = std::min(2.0f, std::max(0.0f, fb));
    fb = fb + fb / (1.0f - cut + eps);
    v0 = v0 + cut * (in - v0 + fb * (v0 - v1));
    v1 = v1 + cut * (v0 - v1);
    value = v1;
  }

private:
  Signal *in, *cut, *res;
  float v0 = 0;
  float v1 = 0;
  float eps = 0.01f;
  //float value = 0;
};

class DigitalHiPass : public Signal {
public:
  DigitalHiPass(Registry* r, Signal* in, Signal* cut, Signal* res)
    : Signal(r) {
    this->in = in;
    this->cut = cut;
    this->res = res;
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {
    float cut = this->cut->getValue();
    cut = std::min(1.0f, std::max(0.0f, cut));
    float in = this->in->getValue();
    float fb = this->res->getValue();
    fb = std::min(2.0f, std::max(0.0f, fb));
    fb = fb + fb / (1.0f - cut + eps);
    v0 = v0 + cut * (in - v0 + fb * (v0 - v1));
    //v0 = v0 + cut * (in - v0);
    v1 = v1 + cut * (v0 - v1);
    value = in - v1;
  }

private:
  Signal *in, *cut, *res;
  float v0 = 0;
  float v1 = 0;
  float eps = 0.01f;
  //float value = 0;
};

class Overdrive : public Signal {
public:
  Overdrive(Registry* r, Signal* in, Signal* gain)
    : Signal(r) {
    this->in = in;
    this->gain = gain;
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {
    float in = this->in->getValue();
    float gain = this->gain->getValue();
    value = fast_tanh(in * gain) * 0.5f; // * (1/gain);
  }

private:
  Signal *in, *gain;
};

class Stutter : public Signal {
public:
  Stutter(Registry* r, Signal* in, Signal* fraction, Signal* bpm)
    : Signal(r) {
    this->in = in;
    this->fraction = fraction;
    this->bpm = bpm;
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {
    float in = this->in->getValue();
    float frac = fraction->getValue();
    float bpm = this->bpm->getValue();

    if (frac < 2) { // skip
      value = in;
      return;
    }

    int loopLength = (int) (60.0f / (bpm * 4 * frac) * 44100);
    
    if (samplecounter < loopLength) samples[samplecounter]=in;
    value = samples[samplecounter % loopLength];
    samplecounter++;

  }

  void retrigger() {
    samplecounter = 0;
  }

  //int loopLength = (int) (60.0f / (bpm * 4 * frac) * 44100);

private:
  Signal *in, *fraction, *bpm;
  float samples[2800]; // slowest speed: 60 -> 16th is 248 ms, -> 10937 samples 
  int samplecounter = 0;
  //int loopLength = (int) (60.0f / (120 * 16) * 44100);
};

class Click : public Signal {
public:
  Click(Registry* r)
    : Signal(r) {
    value = 1;
  }

  float getValue(int channel = 0) {
    return value;
  }

  void update() {
    counter++;
    if (counter > 100)
      value = 0;
    
  }

  void retrigger() {
    value = 1;
    counter = 0;
  }

  private:
  int counter = 0;

};

class SimperSVF : public Signal {
public:
  SimperSVF(Registry* r, Signal* input, Signal* cutoff, Signal* reso)
    : Signal(r) {

    ic1eq = 0.0f;
    ic2eq = 0.0f;
    in = input;
    cut = cutoff;
    res = reso;
  }

  void update() {

    counter++;

    //if (counter%128==0)
    if ((counter & ((1 << 8) - 1)) == 0)  // don't update every sample (only needed for audio rate modulation)
    {

      //float cut =  std::min(0.99f, this->cut->getValue());
      //float res = this->res->getValue();
      float cut = std::min(0.99f, this->cut->value);
      float res = this->res->value;

      //float targetFreq = 0.001f * std::pow(2,9*cut);
      float targetFreq = 0.001f * fast_exp2f(9 * cut);
      //float g = std::tan(Pi * 0.5f * targetFreq);
      g = std::tan(Pi * 0.5f * targetFreq);
      //g = targetFreq;
      //float temp = Pi * 0.5f * targetFreq;
      //g = arm_sin_f32(temp)/arm_cos_f32(temp); // this is actually slower than std::tan


      //float k = 2 - 2 * res;
      k = 2 - 2 * res;

      intermed = 1.0f / (1 + g * (g + k));
    }

    //float v0 = in->getValue();
    float v0 = in->value;

    //float v1 = (ic1eq + g * (-ic2eq + v0)) / (1 + g * (g + k));
    float v1 = (ic1eq + g * (-ic2eq + v0)) * intermed;
    float v2 = ic2eq + g * v1;

    ic1eq = 2 * v1 - ic1eq;
    ic2eq = 2 * v2 - ic2eq;

    value = v2;
  }

  float getValue(int channel = 0) {
    return value;
  }

  virtual String signame() const {
    return "simper";
  }

private:
  Signal *in, *cut, *res;
  float ic1eq = 0;
  float ic2eq = 0;
  float Pi = 3.14;
  //float value = 0;

  int counter = 0;  // a test to optimise code
  float g = 0;
  float k = 0;
  float intermed = 0;
};

class FourPole : public Signal {
public:
  FourPole(Registry* r, Signal* input, Signal* cutoff, Signal* reso)
    : Signal(r) {

    ic1eq = 0.0f;
    ic2eq = 0.0f;
    ic3eq = 0.0f;
    ic4eq = 0.0f;
    in = input;
    cut = cutoff;
    res = reso;
  }

  void update() {

    counter++;


    //if (counter%128==0)
    if ((counter & ((1 << 8) - 1)) == 0)  // don't update every sample (only needed for audio rate modulation)
    {

      //float cut =  std::min(0.99f, this->cut->getValue());
      //float res = this->res->getValue();
      double cut = std::min(0.99f, this->cut->value);
      double res = this->res->value;

      //float targetFreq = 0.001f * std::pow(2,9*cut);
      double targetFreq = 0.001f * fast_exp2f(9 * cut);
      //float g = std::tan(Pi * 0.5f * targetFreq);
      //g = std::tan(Pi * 0.5f * targetFreq);
      //g = targetFreq;
      //float temp = Pi * 0.5f * targetFreq;
      //g = arm_sin_f32(temp)/arm_cos_f32(temp); // this is actually slower than std::tan


      //g = cut * 0.5f;
      //float k = 2 - 2 * res;
      k = 3 * res;  // above 3 it explodes...

      targetFreq = std::min(0.5, targetFreq);
      g = std::tan(Pi * targetFreq);  // targetfreq is at most 0.5
      //g = 2  * std::tan(Pi * 0.5f * targetFreq);

      g2 = g * g;
      g3 = g2 * g;
      g4 = g3 * g;
      intermed = 1.0f / (1 + g);
    }

    //float v0 = in->getValue();
    double v0 = in->value;

    //float v1 = (ic1eq + g * (-ic2eq + v0)) / (1 + g * (g + k));
    //float v1 = (ic1eq + g * (-ic2eq + v0)) * intermed;
    //float v2 = ic2eq + g * v1;



    double v1 = -((k * g3 + 2 * k * g2 + k * g) * ic4eq
                  + (k * g3 + k * g2) * ic3eq
                  + k * g3 * ic2eq
                  + (-g3 - 3 * g2 - 3 * g - 1) * ic1eq
                  + (-g4 - 3 * g3 - 3 * g2 - g) * v0)
                / ((k + 1) * g4 + 4 * g3 + 6 * g2 + 4 * g + 1);

    double v2 = (g * v1 + ic2eq) * intermed;
    double v3 = (g * v2 + ic3eq) * intermed;
    double v4 = (g * v3 + ic4eq) * intermed;

    //v4 = fast_tanh(v4); // this destroys all the math, but maybe helps to prevent explosion

    ic1eq = 2 * v1 - ic1eq;
    ic2eq = 2 * v2 - ic2eq;
    ic3eq = 2 * v3 - ic3eq;
    ic4eq = 2 * v4 - ic4eq;

    value = (float)v4;
  }

  float getValue(int channel = 0) {
    return value;
  }

  virtual String signame() const {
    return "4Pole";
  }

private:
  Signal *in, *cut, *res;
  double ic1eq = 0;
  double ic2eq = 0;
  double ic3eq = 0;
  double ic4eq = 0;
  double Pi = 3.14159;
  //float value = 0;

  int counter = 0;  // a test to optimise code
  double g = 0;
  double k = 0;
  double intermed = 0;
  double g2 = 0;
  double g3 = 0;
  double g4 = 0;
};

class Part {

  public:
  Part(StaticSignal *partVolume) {
  //  Part() {

    this->partVolume = partVolume;

    // lru voice logic
    for (int i = 0; i < maxNbVoices; i++) {
      lru[i] = i;
      notes[i] = -1; // a note that never receives a noteOff message, otherwise it messes up nbAvailableVoices etc.
    }

    // all voices available
    nbAvailableVoices = activeNbVoices;
    leastRecentlyReleasedVoiceId = 0;
  }

  virtual SignalPtr buildSynth(Registry* registry, SynthParameters* menu, int partId);

  void noteOn(int note, int velo);
  void noteOff(int note, int velo);

  virtual void retriggerVoice(int voiceId) {
    this->envs[voiceId]->retrigger();
    this->fenvs[voiceId]->retrigger();
  }

  float midiToFreq(int note) {
    return (440.0f * pow(2, ((note)-69) / 12.0f));
  }

  void setActiveNbVoices(int n) {
    activeNbVoices = n;
    for (int i = 0; i < maxNbVoices; i++) {
      lru[i] = i;
    }

    // all voices available
    nbAvailableVoices = activeNbVoices;
    leastRecentlyReleasedVoiceId = 0;
  }

  int getActiveNbVoices() {
    return activeNbVoices;
  }

  private:
  int activeNbVoices = 0;

  const static int maxNbVoices = 6;
  

  StaticSignal* baseFreqs[maxNbVoices];
  Env* envs[maxNbVoices];
  Env* fenvs[maxNbVoices];

  // the per voice signals that go into the last mixer
  SignalPtr signals[maxNbVoices];

  StaticSignal *partVolume; 

  // remember which note each voice is playing
  int notes[maxNbVoices];

  // for the key assignment logic, simple variant
  int nextVoice = 0;

  // lru key assignment logic
  int lru[maxNbVoices];              // remember which voice was released least recently
  int nbAvailableVoices;             // point into lru array, with wrap around, inclusive
  int leastRecentlyReleasedVoiceId;  // point to last voice that was released, with wrap around

  friend class SynthPart;
  friend class DrumPart;

};

class SynthPart :  public Part {

public:
  SynthPart(StaticSignal *partVolume): Part(partVolume) {}
  SignalPtr buildSynth(Registry* registry, SynthParameters* menu, int partId);

private:

  void createSynthVoice(int i, Registry* registry); 

  // store all parameters, so all the voices can access them:
  StaticSignal* o1detune = NULL;
  StaticSignalDiscrete* o1Oct = NULL;
  StaticSignalDiscrete* o2Oct = NULL;
  StaticSignal* o1vol = NULL;
  StaticSignal* o2vol = NULL;
  StaticSignal* o1wave = NULL;
  StaticSignal* o2wave = NULL;
  StaticSignal* o1pw = NULL;
  StaticSignal* o2pw = NULL;
  StaticSignal* subVol = NULL;
  StaticSignal* subPhase = NULL;
  StaticSignal* envA = NULL;
  StaticSignal* envD = NULL;
  StaticSignal* envS = NULL;
  StaticSignal* envR = NULL;
  StaticSignal* fenvA = NULL;
  StaticSignal* fenvD = NULL;
  StaticSignal* fenvS = NULL;
  StaticSignal* fenvR = NULL;
  StaticSignal* cutoff = NULL;
  StaticSignal* resonance = NULL;
  StaticSignal* envFilterAmount = NULL;
  StaticSignal* panSpread = NULL;

  StaticSignal* lfoFreq = NULL;
  StaticSignalDiscrete* lfoWave = NULL;
  Signal* lfo = NULL;

  StaticSignal* lfoToPitch = NULL;
  StaticSignal* lfoToCutoff = NULL;
  StaticSignal* lfoToPW1 = NULL;
  StaticSignal* lfoToPW2 = NULL;
};

class DrumPart :  public Part {

public:
  DrumPart(StaticSignal *partVolume, StaticSignal *bpm): Part(partVolume) {
  //DrumPart(StaticSignal *bpm): Part() {  
    this->bpm=bpm;
  }
  SignalPtr buildSynth(Registry* registry, SynthParameters* menu, int partId);
  virtual void retriggerVoice(int voiceId) {
    Part::retriggerVoice(voiceId);
    this->stutter->retrigger();
    this->click->retrigger();
  }

private:

  //void createSynthVoice(int i, Registry* registry); 

  // store all parameters, so all the voices can access them:
  
  StaticSignal* o1Oct = NULL;
  StaticSignal* pitchEnvDR = NULL;
  StaticSignal* ampEnvDR = NULL;
  
  StaticSignal* envPitchAmount = NULL;

  StaticSignal *sinVol = NULL;
  StaticSignal *noiseVol = NULL;

  StaticSignal* hiPassCutoff = NULL;
  StaticSignal* hiPassRes = NULL;
  StaticSignal* loPassCutoff = NULL;

  StaticSignal* overdriveGain = NULL;
  StaticSignal* stutterFraction = NULL;

  Stutter *stutter = NULL;
  Click *click = NULL;

  StaticSignal *bpm;
  
};

class SynthEngine : public AudioStream {
public:
  SynthEngine()
    : AudioStream(0, NULL) {
    // any extra initialization
  }

  void buildEngine(std::vector<SynthParameters*>, StaticSignal *bpm);
  //void buildEngine(std::vector<SynthParameters*> allParams);

  // destruction:
  // - destroy all Signals in registry
  // - destroy all paramInfos in Menu

  virtual void update(void);

  int getNbParts() {
    return nbParts;
  }

  int getNbPartTypes() {
    return nbPartTypes;
  }

  void noteOn(byte channel, byte note, byte velocity) {
    parts[channel - 1]->noteOn(note, velocity);  // TODO: proper routing and checking
  }

  void noteOff(byte channel, byte note, byte velocity) {
    parts[channel - 1]->noteOff(note, velocity);
  }

  float getAndResetMaxLevel() {
    float result = this->maxSignalLevel;
    this->maxSignalLevel = 0;
    return result;
  }

  // mark those voices as active that are needed
  void markRequiredSignals() {

    Serial.println("markRequiredSignals");
    Serial.print("number total registered signals: ");
    Serial.println(registry.nbSignals);

    registry.nbActiveSignals = 0;

    for (int s = 0; s < registry.getNbSignals(); s++) {
      int part = registry.partIds[s];
      if (parts[part]->getActiveNbVoices() > registry.voiceIds[s]) {
        registry.activeSignals[registry.nbActiveSignals] = registry.signals[s];
        registry.nbActiveSignals++;
      }
    }

    Serial.print("number signals marked as active: ");
    Serial.println(registry.nbActiveSignals);
  }

  Registry registry;  // put it here for debugging

  StaticSignal* partVolumes[NB_PARTS]; // the drum part and the synth part on the same voice will share the volume

private:

  // for each voice:
  // the main outputsignal
  //const static int nbVoices = 12;
  const static int nbParts = NB_PARTS;
  const static int nbPartTypes = NB_PART_TYPES; // type of engines (synth, drum)

  /*
  For each of the 6 logical parts we alreadz instantiate all of the possible sound engines (i.e. part types)
  The resulting signals and parts are stored in the following fields:
  currently we have 2 part types (synth and drum), the fields contain first the synth parts and then the drum parts
  When accessing the parts, depending on the selected part type we need to offset the access by nbParts...
  */
  // the per voice signals that go into the last mixer, nbParts * nbPartTypes
  SignalPtr signals[nbParts*nbPartTypes];
  Part* parts[nbParts*nbPartTypes];

  

  // totaloutput
  SignalPtr outputSignal = 0;

  //
  float maxSignalLevel = 0;
};



#endif
