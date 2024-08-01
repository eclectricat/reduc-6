#include "Menu.h"

class Mode;
class LiquidCrystal;
class SynthEngine;
class GlobalState;
//class Menu2;

class Mode {

  public:
  virtual void processPotValue(int potIndex, int potVal,  bool updateDisplay);
  virtual void pushButtonPressed(int buttonIndex);
  virtual void pushButtonReleased(int buttonIndex);

  virtual void fullDisplayUpdate();

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
  void setup();

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

  //std::vector<SynthParameters*> allSynthParameters;


};

class GlobalState {

  public:
  GlobalState(SynthEngine *engine, LiquidCrystal *lcd); 
  
  SynthMode synthMode;
  PartConfigMode partConfigMode;
  SynthMode *selectedMode = &synthMode;

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

  int delayedDisplayRefresh = -1;

  SynthEngine *engine;
  private:
  

};

/*class Mode {
  virtual void processPotValue(int potIndex, int potVal);
  virtual void pushButtonPressed(int buttonIndex);
  virtual void pushButtonReleased(int buttonIndex);

}*/

