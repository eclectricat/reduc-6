#include "Menu.h"

class Mode;
class LiquidCrystal;
class SynthEngine;
class GlobalState;
class Sequence;
//class Menu2;

// TODO; put this to some truly global location
#define NB_PARTS 6 
#define NB_PATTERNS 4

#define MODE_BUTTON 9
#define PART_BUTTON 8


class Mode {

  public:
  virtual void processPotValue(int potIndex, int potVal,  bool updateDisplay);
  virtual bool pushButtonPressed(int buttonIndex);
  virtual bool pushButtonReleased(int buttonIndex);

  virtual void fullDisplayUpdate();

  virtual void postPartOrModeSwitch() {};

  int handleGenericPushButtonEvents(int buttonIndex);
  bool handleConfirmModeButtonPressed(int buttonIndex); // returns if pots should be locked
  void confirmationDisplayNotification();
  virtual void handleConfirmed();
  virtual void handleCancelled();

  virtual int effectivePartId(int partId); 

  void serializePart(JsonObject *jsonObject, int partId);
  void deserializePart(JsonObject *jsonObject, int partId);
  ParameterInfo* getParameterByNameAndPart(String uniqueName, int partId);

  GlobalState *globalState;
  LiquidCrystal *lcd;

  int selectedLane = 0;
  int selectedPage = 0;
  vector<ParameterInfo*> *currentMenuPage;

  ParameterInfo *parameterRequestingConfirmation = NULL;
  int lastConfDisplayUpdate = 0;

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

  void setup();
  virtual void postPartOrModeSwitch();
  virtual void handleConfirmed();

  bool handleSeqAct();

  int effectivePartId(int partId) {return partId;} // used for page switch

  void resetEngineTypeAndVoices();

  int partTypes[NB_PARTS] = {0,1,1,1,0,0}; 

  // link to parameters controlling nbVoices and engineType
  StaticSignalDiscrete* nbVoicesParams[NB_PARTS];
  StaticSignalDiscrete* engineTypeParams[NB_PARTS];

  StaticSignalDiscrete *selectedBank;
  StaticSignalDiscrete *selectedPatch;
  StaticSignalDiscrete *seqActionDest;
  StaticSignalDiscrete *seqActionOp;

  int clipBoardPattern = -1;
  int clipBoardPart = -1;
};

class MixMuteMode: public Mode {

  public:
  MixMuteMode(LiquidCrystal *lcd) {
    this->lcd = lcd;
  }
  void setup();
  virtual void postPartOrModeSwitch();
  virtual bool pushButtonPressed(int buttonIndex);
  virtual bool pushButtonReleased(int buttonIndex);
  virtual void processPotValue(int potIndex, int potVal,  bool updateDisplay);

  int armedForMuteToggle[NB_PARTS];
};

class SequencerMode: public Mode {

  public: 
  SequencerMode(LiquidCrystal *lcd, SynthEngine *engine) {
    this->lcd = lcd;

    for (int p = 0; p<NB_PARTS; p++) {
      patternLengths[p] = new StaticSignalDiscrete(&(engine->registry), 16);
    }

    bpm = new StaticSignalDiscrete(&(engine->registry), 120);
    pBPM = new ParameterInfoDiscrete("BPM", 70, 180, bpm, "BPM");
  }

  virtual void processPotValue(int potIndex, int potVal, bool updateDisplay) override;
  virtual bool pushButtonPressed(int buttonIndex) override;
  virtual bool pushButtonReleased(int buttonIndex) override;
  virtual void postPartOrModeSwitch() override { 
    this->paramLockMode = 0; // not exactly sure how it can happen that this is on
  }
 
  void setup();
  virtual void fullDisplayUpdate();
  void displayLockingParams();
  void processLockParameter(int potIndex, int value);
  void displayDoubleShiftMode();
  void processDoubleShiftParameter(int potIndex, int value);

  void maybePlay(); // internal clock
  void play(); // called directly when externally clocked
  void displayPlayStatus();
  void displayOctAndNote();
  void displayStep();


  // TBD: sometimes the syncing is about 40ms behind
  void tick() { 
    if (tickCounter == 0) {
      if (playingSync) {
        Serial.println("tick -> play");
        play();
      } else {
        Serial.println("tick (no play)"); // looks like this is not happening, at least with reaper
      }
    }
    tickCounter = (tickCounter + 1 ) % 6;
  }

  void startSync() { playHead = 0; playingSync = true; tickCounter = 0;} 
  void stopSync() { playingSync = false; cleanUpAfterStop();}
  void continueSync() {playHead = 0; playingSync = true; tickCounter = 0;}
  void cleanUpAfterStop();

  void serializeSequencerData(JsonObject *seqData, int partId);
  void deserializeSequencerData(JsonObject *seqData, int partId);

  int cursorPos = 0;
  int nbSteps = 64;

  int playHead = 0;
  double nextTriggerTime = 0;
  double interBeatMs = 125; // tempo, 16th notes when quarter is at 120 BPM

  int tickCounter = 0;
  bool playingSync = false;

  std::vector<int> lastPlayedNote; // to be able to stop notes
  std::vector<int> remainingNoteDuration; //keep note length etc

  std::vector<ParameterInfo*> doubleShiftParameters; // page and pattern switching

  // state: temporary sub-modes, pattern change, parameter lock
  bool paramLockMode = false;
  bool doubleShiftMode = false;
  int patternSelectMode = 0;

  //float valuesToReset[4 * 6] ; // TODO: don't hardcode
  ParameterInfo *parametersToReset[4 * NB_PARTS];
  StaticSignalDiscrete* patternLengths[NB_PARTS];

  StaticSignalDiscrete *sequencerActive[NB_PARTS];

  StaticSignalDiscrete* bpm;
  ParameterInfo *pBPM;

  // the double shift parameters
  StaticSignalDiscrete *page = NULL;
  StaticSignalDiscrete *selectedPattern = NULL;
  StaticSignalDiscrete *sequencerPlaying = NULL;

};

class SequencerModeGraphic : public Mode {
  public:

  SequencerModeGraphic(LiquidCrystal *lcd) {
    this->lcd = lcd;
  }

  virtual void processPotValue(int potIndex, int potVal, bool updateDisplay) override;
  virtual bool pushButtonPressed(int buttonIndex) override;
  virtual bool pushButtonReleased(int buttonIndex) override;
  virtual void postPartOrModeSwitch() override {
    armedForToggle = -1; 
    seqMode->paramLockMode = 0;
  }
  virtual void fullDisplayUpdate() override;
  void setup(SequencerMode *sMode) {seqMode = sMode;}

  SequencerMode *seqMode; // most calls will be delegated to the normal sequencer mode

  int armedForToggle = -1; // only toggle on button release, if the button press was not used for something else

};

class GlobalState {

  public:
  GlobalState(SynthEngine *engine, LiquidCrystal *lcd); 
  
  SynthMode synthMode;
  PartConfigMode partConfigMode;
  SequencerMode sequencerMode;
  SequencerModeGraphic sequencerModeGraphic;
  MixMuteMode mixMuteMode;
  Mode *selectedMode = &synthMode;


  int selectedPart=0;
  int selectedPattern=0;

  void setup();

  void myNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  void myNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);

  void serializeProgram(JsonObject *jsonObject);
  void deserializeProgram(JsonObject *jsonObject);

  //void switchToMode(int mode);
  // change mode, or pass button on the active mode
  //void processButtonPress();
  // the button positions are directly passed to the mode

  bool shiftPressed = false;
  bool pPressed = false;

  bool seqPlaying = false;

  int delayedDisplayRefresh = -1;

  //index like that: patterns[patternnumber][track]-> sequence
  std::vector<std::vector<Sequence*>> patterns = std::vector<std::vector<Sequence*>>(NB_PATTERNS, std::vector<Sequence*>(NB_PARTS, NULL)); 

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

  public:

    const int NB_STEPS = 64 ;
    // stored: octave, note, on/off, length ( velo)
    
    Sequence() { }
    //std::vector<std::vector<int> > data = std::vector<std::vector<int> >(3, std::vector<int>(NB_STEPS, 0)); // 3 x nbsteps
    std::vector<std::vector<int8_t> > data = std::vector<std::vector<int8_t> >(4, std::vector<int8_t>(NB_STEPS, 0)); // 4 x nbsteps
    std::vector<std::vector<ParameterInfo*>> lockedParameters = std::vector<std::vector<ParameterInfo*> >(4, std::vector<ParameterInfo*>(NB_STEPS, NULL)); // 4 automated params
    std::vector<std::vector<float>> lockedValues = std::vector<std::vector<float> >(4, std::vector<float>(NB_STEPS, 0)); // 4 automated params
};

