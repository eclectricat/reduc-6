#include "Modes.h"
#include "SynthEngine.h"
#include <LiquidCrystal.h>


GlobalState::GlobalState(SynthEngine *engine, LiquidCrystal *lcd)
  : synthMode(lcd) {
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
  engine->noteOn(note, velocity);

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
  engine->noteOff(note, velocity);

  //digitalWrite(led1Pin, LOW);
}

void GlobalState::setup() {
  synthMode.globalState = this;
}

SynthMode::SynthMode(LiquidCrystal *lcd) {
  this->lcd = lcd;
}

void SynthMode::setup() {
  Serial.println("getting initial page");
  currentMenuPage = synthParameters.getPage(0, 0);
  Serial.println("got initial page");
}

void SynthMode::processPotValue(int potIndex, int potVal, bool updateDisplay) {
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

void SynthMode::pushButtonPressed(int buttonIndex) {

  if (selectedLane == buttonIndex) {  // already on that lane
    selectedPage = (selectedPage + 1) % synthParameters.getNbPages(selectedLane);

  } else {
    if (buttonIndex < synthParameters.getNbLanes()) {
      selectedLane = buttonIndex;
      selectedPage = 0;
    }
  }

  if (synthParameters.existPage(selectedLane, selectedPage)) {
    currentMenuPage = synthParameters.getPage(selectedLane, selectedPage);
    Serial.print("Selected param:");
    lcd->clear();
    for (unsigned int p = 0; p < currentMenuPage.size(); p++) {
      Serial.print(currentMenuPage.at(p)->getName());
      Serial.print("__");

      lcd->setCursor(4 * p, 0);
      lcd->print(currentMenuPage.at(p)->getName());
      lcd->setCursor(4 * p, 1);
      lcd->print(currentMenuPage.at(p)->printableValue());
    }
    Serial.println("");
  }

  if (buttonIndex == 7) {
    globalState->myNoteOn(1, 55, 127);
  }
}

void SynthMode::pushButtonReleased(int buttonIndex) {

  if (buttonIndex == 7) {
    globalState->myNoteOff(1, 55, 127);
  }
}
