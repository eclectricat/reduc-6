
class Mode;

class GlobalState {

  public:
  Mode selectedMode;

  int selectedPart;

  void switchToMode(int mode);

  // change mode, or pass button on the active mode
  void processButtonPress();

  // the button positions are directly passed to the mode

}

class Mode {
  void processPotValue(int potIndex, int potVal);
  void pushButtonPressed(int buttonIndex);
  void pushButtonReleased(int buttonIndex);

}

class 