#include "Menu.h"

class Mode;
class LiquidCrystal;
class SynthEngine;
class GlobalState;
//class Menu2;


class SynthMode { //: public Mode {
  public:

  SynthMode(LiquidCrystal *lcd);
  virtual void processPotValue(int potIndex, int potVal, bool updateDisplay);
  virtual void pushButtonPressed(int buttonIndex);
  virtual void pushButtonReleased(int buttonIndex);
  void setup();

  GlobalState *globalState;
  LiquidCrystal *lcd;

  int selectedLane = 0;
  int selectedPage = 0;
  vector<ParameterInfo*> currentMenuPage;

  std::vector<SynthParameters*> allSynthParameters;

};

class GlobalState {

  public:
  GlobalState(SynthEngine *engine, LiquidCrystal *lcd); 
  
  SynthMode synthMode;
  SynthMode *selectedMode = &synthMode;

  int selectedPart=0;

  void setup();

  void myNoteOn(byte channel, byte note, byte velocity);
  void myNoteOff(byte channel, byte note, byte velocity);

  //void switchToMode(int mode);
  // change mode, or pass button on the active mode
  //void processButtonPress();
  // the button positions are directly passed to the mode

  private:
  SynthEngine *engine;

};

/*class Mode {
  virtual void processPotValue(int potIndex, int potVal);
  virtual void pushButtonPressed(int buttonIndex);
  virtual void pushButtonReleased(int buttonIndex);

}*/

