#include "Menu.h"

class Mode;
class LiquidCrystal;
class SynthEngine;
class GlobalState;
class Sequence;
//class Menu2;

class Mode {

  public:
  virtual void processPotValue(int potIndex, int potVal,  bool updateDisplay);
  virtual bool pushButtonPressed(int buttonIndex);
  virtual bool pushButtonReleased(int buttonIndex);

  virtual void fullDisplayUpdate();

  virtual void postPartOrModeSwitch() {};

  int handleGenericPushButtonEvents(int buttonIndex);

  int effectivePartId(int partId); 

  GlobalState *globalState;
  LiquidCrystal *lcd;

  int selectedLane = 0;
  int selectedPage = 0;
  vector<ParameterInfo*> currentMenuPage;

  std::vector<SynthParameters*> allSynthParameters;

};

class SynthMode : public Mode {
  public:

  SynthMode(LiquidCrystal *lcd);
  //virtual void processPotValue(int potIndex, int potVal, bool updateDisplay) override;
  virtual bool pushButtonPressed(int buttonIndex) override;
  virtual bool pushButtonReleased(int buttonIndex) override;
  virtual void postPartOrModeSwitch() override;
  void setup();

};

class PartConfig {
  public:
    PartConfig() {}


};

class PartConfigMode: public Mode {

  public:
  PartConfigMode(LiquidCrystal *lcd) {
    this->lcd = lcd;
  }

  //virtual void processPotValue(int potIndex, int potVal, bool updateDisplay) override;
  //virtual void pushButtonPressed(int buttonIndex) override;
  //virtual void pushButtonReleased(int buttonIndex) override;
  void setup();
  virtual void postPartOrModeSwitch();

  //int selectedLane = 0;
  //int selectedPage = 0;
  ///vector<ParameterInfo*> currentMenuPage;
  int partTypes[6] = {0,1,1,1,0,0}; /// TODO: control via parameters

  // TODO: should be nbParts * nbPartTypes * 4, and values should not be hardcoded 
  //ParameterInfo* lockParameters[4*6]; // 4 params, 6 parts, get it with (partId * 4 + paramId) //= {NULL, NULL, NULL, NULL};

  //std::vector<SynthParameters*> allSynthParameters;


};

class MixMuteMode: public Mode {

  public:
  MixMuteMode(LiquidCrystal *lcd) {
    this->lcd = lcd;
  }
  void setup();
  virtual void postPartOrModeSwitch();
  virtual bool pushButtonPressed(int buttonIndex);
};

class SequencerMode: public Mode {

  public: 
  SequencerMode(LiquidCrystal *lcd, SynthEngine *engine) {
    this->lcd = lcd;

    for (int p = 0; p<NB_PARTS; p++) {
      patternLengths[p] = new StaticSignalDiscrete(&(engine->registry), 16);
    }

    bpm = new StaticSignalDiscrete(&(engine->registry), 120);
    pBPM = new ParameterInfoDiscrete("BPM", 70, 180, bpm);
  }

  virtual void processPotValue(int potIndex, int potVal, bool updateDisplay) override;
  virtual bool pushButtonPressed(int buttonIndex) override;
  virtual bool pushButtonReleased(int buttonIndex) override;
 
  void setup();
  virtual void fullDisplayUpdate();
  void displayLockingParams();
  void processLockParameter(int potIndex, int value);

  void maybePlay();
  void displayPlayStatus();
  void displayOctAndNote();
  void displayStep();

  int cursorPos = 0;
  int nbSteps = 64;

  int playHead = 0;
  int nextTriggerTime = 0;
  //int interBeatMs = 250; // tempo, 8th notes when quarter is at 120 BPM
  int interBeatMs = 125; // tempo, 16th notes when quarter is at 120 BPM

  std::vector<int> lastPlayedNote; // to be able to stop notes: todo: keep note length etc

  // state: temporary sub-modes, pattern change, parameter lock
  int paramLockMode = 0;
  int patternSelectMode = 0;

  //float valuesToReset[4 * 6] ; // TODO: don't hardcode
  ParameterInfo *parametersToReset[4 * NB_PARTS];
  StaticSignalDiscrete* patternLengths[NB_PARTS];

  StaticSignalDiscrete* bpm;
  ParameterInfo *pBPM;

};

class GlobalState {

  public:
  GlobalState(SynthEngine *engine, LiquidCrystal *lcd); 
  
  SynthMode synthMode;
  PartConfigMode partConfigMode;
  SequencerMode sequencerMode;
  MixMuteMode mixMuteMode;
  Mode *selectedMode = &synthMode;


  int selectedPart=0;

  void setup();

  void myNoteOn(byte channel, byte note, byte velocity);
  void myNoteOff(byte channel, byte note, byte velocity);

  //void switchToMode(int mode);
  // change mode, or pass button on the active mode
  //void processButtonPress();
  // the button positions are directly passed to the mode

  bool shiftPressed = false;
  bool pPressed = false;

  bool seqPlaying = false;

  int delayedDisplayRefresh = -1;

  std::vector<Sequence*> sequences;

  StaticSignal* playingProb[NB_PARTS]; // 16 for all possible midi channels

  SynthEngine *engine;

  
  private:
  

};

/*class Mode {
  virtual void processPotValue(int potIndex, int potVal);
  virtual void pushButtonPressed(int buttonIndex);
  virtual void pushButtonReleased(int buttonIndex);

}*/


class Sequence {

  const int NB_STEPS = 64 ;
  // stored: octave, note, on/off, (length, velo)

  public:
    Sequence() {
      // TODO
    }
    //std::vector<std::vector<int> > data = std::vector<std::vector<int> >(3); // , std::vector<int>(NB_STEPS, 0));
    std::vector<std::vector<int> > data = std::vector<std::vector<int> >(3, std::vector<int>(NB_STEPS, 0)); // 3 x nbsteps
    std::vector<std::vector<ParameterInfo*>> lockedParameters = std::vector<std::vector<ParameterInfo*> >(4, std::vector<ParameterInfo*>(NB_STEPS, NULL)); // 4 automated params
    std::vector<std::vector<float>> lockedValues = std::vector<std::vector<float> >(4, std::vector<float>(NB_STEPS, 0)); // 4 automated params
};

