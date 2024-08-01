#include "Modes.h"
#include "SynthEngine.h"
#include <LiquidCrystal.h>


GlobalState::GlobalState(SynthEngine *engine, LiquidCrystal *lcd)
  : synthMode(lcd), partConfigMode(lcd) {
  this->engine = engine;
}

void GlobalState::myNoteOn(byte channel, byte note, byte velocity) {
  // When using MIDIx4 or MIDIx16, usbMIDI.getCable() can be used
  // to read which of the virtual MIDI cables received this message.
  Serial.print("Note On, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
  engine->noteOn(channel, note, velocity);

  //digitalWrite(led1Pin, HIGH);
}

void GlobalState::myNoteOff(byte channel, byte note, byte velocity) {
  // When using MIDIx4 or MIDIx16, usbMIDI.getCable() can be used
  // to read which of the virtual MIDI cables received this message.
  Serial.print("Note Off, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
  engine->noteOff(channel, note, velocity);

  //digitalWrite(led1Pin, LOW);
}

void GlobalState::setup() {
  synthMode.globalState = this;
  partConfigMode.globalState = this;
  for(int i=0;i<this->engine->getNbParts();i++) {
    synthMode.allSynthParameters.push_back(new SynthParameters());
    Serial.println("created synth parameters");
    partConfigMode.allSynthParameters.push_back(new SynthParameters());
  }
  this->delayedDisplayRefresh = millis() + 2000;
}

SynthMode::SynthMode(LiquidCrystal *lcd) {
  this->lcd = lcd;
}

void SynthMode::setup() {
  Serial.println("getting initial page");
  currentMenuPage = allSynthParameters[0]->getPage(0, 0);
  Serial.println("got initial page");
}

void PartConfigMode::setup() {

  // create all the synth params
  Registry *registry = &(globalState->engine->registry);

  registry->setPartAndVoiceTag(0,0); // does not really matter - this signal does not need to be updated by the synth engine
  StaticSignal *dummyS = new StaticSignal(registry, 1.0f);
  ParameterInfo *pDummy = new ParameterInfo("....", 0, 10, dummyS);

  for(int p=0;p<globalState->engine->getNbParts();p++) {
    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ pDummy, pDummy, pDummy, pDummy }, 0);
  }
  


  Serial.println("Part info mode: getting initial page");
  currentMenuPage = allSynthParameters[0]->getPage(0, 0);
  Serial.println("got initial page");
}

void Mode::processPotValue(int potIndex, int potVal, bool updateDisplay) {
  currentMenuPage.at(potIndex)->updateParameter(potVal);

  // TODO; make this more efficient, this is costly
  // only refresh if value changed, and print all 4 digits in one go

  if (updateDisplay) {
    lcd->setCursor(4 * potIndex, 1);
    lcd->print("    ");
    lcd->setCursor(4 * potIndex, 1);
    lcd->print(currentMenuPage.at(potIndex)->printableValue());
  }
}

void Mode::pushButtonPressed(int buttonIndex) {

  // update state of shift,P
  // if other button: check if part or mode switch
  // otherwise parameter page switch

  if (buttonIndex == 7) { // P
    globalState->pPressed = true;
    return;
  }

  if (buttonIndex == 6) { // shift
    globalState->shiftPressed = true;
    return;
  }

  if(globalState->shiftPressed && globalState->pPressed) { // mode switch
    Serial.println("mode switch");
    return;
  }

   if(globalState->shiftPressed) { // part switch
    Serial.println("part switch");

    globalState->selectedPart = buttonIndex; // TODO check nbParts
    this->currentMenuPage = allSynthParameters[globalState->selectedPart]->getPage(selectedLane, selectedPage);
    fullDisplayUpdate();
    lcd->setCursor(0, 0);
    lcd->print("Part ");
    lcd->setCursor(5, 0);
    lcd->print(buttonIndex+1);
    lcd->setCursor(6, 0);
    lcd->print("               ");
    this->globalState->delayedDisplayRefresh = millis() + 1000;

    return;
  }

  SynthParameters* synthParameters = allSynthParameters[globalState->selectedPart];

  if (selectedLane == buttonIndex) {  // already on that lane
    selectedPage = (selectedPage + 1) % synthParameters->getNbPages(selectedLane);

  } else {
    if (buttonIndex < synthParameters->getNbLanes()) {
      selectedLane = buttonIndex;
      selectedPage = 0;
    }
  }

  if (synthParameters->existPage(selectedLane, selectedPage)) {
    currentMenuPage = synthParameters->getPage(selectedLane, selectedPage);
    Serial.print("Selected param:");
    /*lcd->clear();
    for (unsigned int p = 0; p < currentMenuPage.size(); p++) {
      Serial.print(currentMenuPage.at(p)->getName());
      Serial.print("__");

      lcd->setCursor(4 * p, 0);
      lcd->print(currentMenuPage.at(p)->getName());
      lcd->setCursor(4 * p, 1);
      lcd->print(currentMenuPage.at(p)->printableValue());
    }*/
    fullDisplayUpdate();
    Serial.println("");
  }

  
}

void Mode::fullDisplayUpdate() {
  lcd->clear();
    for (unsigned int p = 0; p < currentMenuPage.size(); p++) {
      Serial.print(currentMenuPage.at(p)->getName());
      Serial.print("__");

      lcd->setCursor(4 * p, 0);
      lcd->print(currentMenuPage.at(p)->getName());
      lcd->setCursor(4 * p, 1);
      lcd->print(currentMenuPage.at(p)->printableValue());
    }
}

void SynthMode::pushButtonPressed(int buttonIndex) {
  Mode::pushButtonPressed(buttonIndex);

  if (buttonIndex == 7) {
    globalState->myNoteOn(1, 55, 127);
  }

}

void Mode::pushButtonReleased(int buttonIndex) {
  if (buttonIndex == 7) { // P
    globalState->pPressed = false;
    return;
  }

  if (buttonIndex == 6) { // shift
    globalState->shiftPressed = false;
    return;
  }
}

void SynthMode::pushButtonReleased(int buttonIndex) {
  Mode::pushButtonReleased(buttonIndex);
  if (buttonIndex == 7) {
    globalState->myNoteOff(1, 55, 127);
  }
}


