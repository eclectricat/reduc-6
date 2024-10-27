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
  virtual void pushButtonPressed(int buttonIndex);
  virtual void pushButtonReleased(int buttonIndex);

  virtual void fullDisplayUpdate();

  virtual void postPartOrModeSwitch() {};

  int handleGenericPushButtonEvents(int buttonIndex);

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
  virtual void pushButtonPressed(int buttonIndex) override;
  virtual void pushButtonReleased(int buttonIndex) override;
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

  //int selectedLane = 0;
  //int selectedPage = 0;
  ///vector<ParameterInfo*> currentMenuPage;
  int partTypes[6] = {1,1,0,0,0,0}; /// TODO: control via parameters

  //std::vector<SynthParameters*> allSynthParameters;


};

class SequencerMode: public Mode {

  public: 
  SequencerMode(LiquidCrystal *lcd) {
    this->lcd = lcd;
  }

  virtual void processPotValue(int potIndex, int potVal, bool updateDisplay) override;
  virtual void pushButtonPressed(int buttonIndex) override;
  //virtual void pushButtonReleased(int buttonIndex) override;
  void setup();
  virtual void fullDisplayUpdate();

  void maybePlay();
  void displayPlayStatus();
  void displayOctAndNote();
  void displayStep();

  int cursorPos = 0;
  int nbSteps = 8;

  int playHead = 0;
  int nextTriggerTime = 0;
  int interBeatMs = 250; // tempo, 8th notes when quarter is at 120 BPM

  std::vector<int> lastPlayedNote; // to be able to stop notes: todo: keep note length etc

};

class GlobalState {

  public:
  GlobalState(SynthEngine *engine, LiquidCrystal *lcd); 
  
  SynthMode synthMode;
  PartConfigMode partConfigMode;
  SequencerMode sequencerMode;
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

  SynthEngine *engine;
  private:
  

};

/*class Mode {
  virtual void processPotValue(int potIndex, int potVal);
  virtual void pushButtonPressed(int buttonIndex);
  virtual void pushButtonReleased(int buttonIndex);

}*/

class ParameterLock {
  
}


class Sequence {

  const int NB_STEPS = 16 ;
  // stored: octave, note, on/off, (length, velo)

  public:
    Sequence() {
      // TODO
    }
    //std::vector<std::vector<int> > data = std::vector<std::vector<int> >(3); // , std::vector<int>(NB_STEPS, 0));
    std::vector<std::vector<int> > data = std::vector<std::vector<int> >(3, std::vector<int>(NB_STEPS, 0));
    std:vector<>
};

