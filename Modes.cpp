#include "Modes.h"
#include "SynthEngine.h"
#include <LiquidCrystal.h>
#include <ArduinoJson.h>
#include <SD.h>

int freeram() {
    return (char*)&_heap_end - __brkval;
}


GlobalState::GlobalState(SynthEngine *engine, LiquidCrystal *lcd)
  : synthMode(lcd), partConfigMode(lcd), sequencerMode(lcd, engine), mixMuteMode(lcd), sequencerModeGraphic(lcd) {
  this->engine = engine;

  for (int p = 0; p < NB_PARTS; p++) {
    this->playingProb[p] = new StaticSignal(NULL, 1);
  }
}

void GlobalState::myNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  // When using MIDIx4 or MIDIx16, usbMIDI.getCable() can be used
  // to read which of the virtual MIDI cables received this message.
  //channel = channel + 6 * this->partConfigMode.partTypes[channel-1]; // TODO: hardcoded 6

  float dice = ((float)rand()) / ((float)RAND_MAX);

  if (dice > playingProb[channel - 1]->value) {
    return;
  }

  channel = synthMode.effectivePartId(channel - 1) + 1;
  Serial.print("Note On, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
  engine->noteOn(channel, note, velocity);
  //digitalWrite(led1Pin, HIGH);
}

void GlobalState::myNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  // When using MIDIx4 or MIDIx16, usbMIDI.getCable() can be used
  // to read which of the virtual MIDI cables received this message.
  channel = channel + 6 * this->partConfigMode.partTypes[channel - 1];  //
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
  sequencerMode.globalState = this;
  sequencerModeGraphic.globalState = this;
  mixMuteMode.globalState = this;
  for (int i = 0; i < this->engine->getNbParts(); i++) {

    for (int type = 0; type < this->engine->getNbPartTypes(); type++)
      synthMode.allSynthParameters.push_back(new SynthParameters());  // one for every effective part

    Serial.println("created synth parameters");
    partConfigMode.allSynthParameters.push_back(new SynthParameters());  // one for every logical part
    //sequences.push_back(new Sequence());
  }

  Serial.print("patterns: ");
  
  Serial.println(patterns.size());
  Serial.println(patterns[0].size());
  for (int pat=0; pat<NB_PATTERNS; pat++) {
    //patterns.push_back(std::vector<Sequence*>());
    for (int track=0; track<this->engine->getNbParts(); track++) {
      //patterns[pat].push_back(new Sequence());
      //patterns[pat][track] = new Sequence();
      Serial.println("create sequence:");
      Serial.println(pat);
      Serial.println(track);
      Serial.println(freeram());
      Serial.println(patterns[pat][track]==NULL);
      patterns[pat][track] = new Sequence();
      
    }
  }

  this->delayedDisplayRefresh = millis() + 2000;
}

SynthMode::SynthMode(LiquidCrystal *lcd) {
  this->lcd = lcd;
}

void SynthMode::setup() {
  Serial.println("getting initial page");
  currentMenuPage = allSynthParameters[effectivePartId(0)]->getPage(0, 0);
  Serial.println("got initial page");
}

void PartConfigMode::setup() {

  // create all the synth params
  Registry *registry = &(globalState->engine->registry);

  registry->setPartAndVoiceTag(0, 0);  // does not really matter - this signal does not need to be updated by the synth engine
  //StaticSignal *dummyS = new StaticSignal(registry, 0);
  //ParameterInfo *pDummy = new ParameterInfo("....", 0, 10, dummyS, "dummyS");
  ParameterInfo *pDummy = new DummyParameterInfo(" ");
  selectedBank = new StaticSignalDiscrete(NULL, 0);
  selectedPatch = new StaticSignalDiscrete(NULL, 0);

  /*ParameterInfo *pSave = new ParameterInfoDiscrete("SAV", 0, 9, dummyS, "SAV");
  ParameterInfo *pLoad = new ParameterInfoDiscrete("LOD", 0, 9, dummyS, "LOD");
  ParameterInfo *pSaveGlobal = new ParameterInfoDiscrete("GSV", 0, 9, dummyS, "GSV");
  ParameterInfo *pLoadGlobal = new ParameterInfoDiscrete("GLD", 0, 9, dummyS, "GLD");
  ParameterInfo *pQuestion = new ParameterInfo(" ", 0, 10, dummyS, "?");*/

  ParameterInfo *pSave = new DummyParameterInfo("SAV");
  ParameterInfo *pLoad = new DummyParameterInfo("LOD");
  ParameterInfo *pSaveGlobal = new DummyParameterInfo("GSV");
  ParameterInfo *pLoadGlobal = new DummyParameterInfo("GLD");
  ParameterInfo *pQuestion = new DummyParameterInfo(" ");



  ParameterInfo *pBankSave = new ParameterInfoDiscreteConfirmation("BNK", 0, 10, selectedBank, "BNKSave", &(this->parameterRequestingConfirmation));
  ParameterInfo *pPatchSave = new ParameterInfoDiscreteConfirmation("PTC", 0, 10, selectedPatch, "PTCSave", &(this->parameterRequestingConfirmation));
  ParameterInfo *pBankLoad = new ParameterInfoDiscreteConfirmation("BNK", 0, 10, selectedBank, "BNKLoad", &(this->parameterRequestingConfirmation));
  ParameterInfo *pPatchLoad = new ParameterInfoDiscreteConfirmation("PTC", 0, 10, selectedPatch, "PTCLoad", &(this->parameterRequestingConfirmation));

  ParameterInfo *pGlobalBankSave = new ParameterInfoDiscreteConfirmation("BNK", 0, 10, selectedBank, "GlobalBNKSave", &(this->parameterRequestingConfirmation));
  ParameterInfo *pGlobalProgSave = new ParameterInfoDiscreteConfirmation("PRG", 0, 10, selectedPatch, "GlobalPRGSave", &(this->parameterRequestingConfirmation));
  ParameterInfo *pGlobalBankLoad = new ParameterInfoDiscreteConfirmation("BNK", 0, 10, selectedBank, "GlobalBNKLoad", &(this->parameterRequestingConfirmation));
  ParameterInfo *pGlobalProgLoad = new ParameterInfoDiscreteConfirmation("PRG", 0, 10, selectedPatch, "GlobalPRGLoad", &(this->parameterRequestingConfirmation));


  for (int p = 0; p < globalState->engine->getNbParts(); p++) {

    ParameterInfo *pPatternLength = new ParameterInfoDiscrete("len", 1, 64, globalState->sequencerMode.patternLengths[p], "patternLen");
    StaticSignalDiscrete *engineType = new StaticSignalDiscrete(NULL, 0);
    StaticSignalDiscrete *nbVoices = new StaticSignalDiscrete(NULL, (p<4 ? 1: 0));

    this->engineTypeParams[p] = engineType;
    this->nbVoicesParams[p] = nbVoices;

    ParameterInfo *pEngineType = new ParameterInfoDiscreteConfirmation("ENG", 0, 1, engineType, "engineType", &(this->parameterRequestingConfirmation));
    ParameterInfo *pNbVoices = new ParameterInfoDiscreteConfirmation("VOC", 0, 6, nbVoices, "nbVoices", &(this->parameterRequestingConfirmation));

    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ globalState->sequencerMode.pBPM, pPatternLength, pDummy, pDummy }, 0);
    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ pEngineType, pNbVoices, pDummy, pDummy }, 1);

    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ pLoad, pBankLoad, pPatchLoad, pQuestion }, 4);
    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ pSave, pBankSave, pPatchSave, pQuestion }, 5);

    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ pLoadGlobal, pGlobalBankLoad, pGlobalProgLoad, pQuestion }, 4);
    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ pSaveGlobal, pGlobalBankSave, pGlobalProgSave, pQuestion }, 5);

  }

  Serial.println("Part info mode: getting initial page");
  currentMenuPage = allSynthParameters[0]->getPage(0, 0);
  Serial.println("got initial page");
}

void MixMuteMode::setup() {

  // create all the synth params
  Registry *registry = &(globalState->engine->registry);

  registry->setPartAndVoiceTag(0, 0);  // does not really matter - this signal does not need to be updated by the synth engine
  //StaticSignal *dummyS = new StaticSignal(registry, 1.0f);
  //ParameterInfo *pDummy = new ParameterInfo("....", 0, 10, dummyS, "dummyS");
  ParameterInfo *pDummy = new DummyParameterInfo(" ");

  allSynthParameters.push_back(new SynthParameters());  // we only need 1 SynthParameters, not one per part (the different lanes are the different part params)

  for (int p = 0; p < globalState->engine->getNbParts(); p++) {

    //char[] name = "Vx  ";
    //sprintf(name[1], "%d", p);
    ParameterInfo *pPatternVolume = new ParameterInfo("V" + String(p), 0, 1, globalState->engine->partVolumes[p], "partVolume"+String(p));
    ParameterInfo *pPlayingProb = new ParameterInfo("prb", 0, 1, globalState->playingProb[p], "playProb"+String(p));
    ParameterInfo *pMute = new ParameterInfoDiscrete("On ",0,1, globalState->sequencerMode.sequencerActive[p], "UnMuted"+String(p) );
    allSynthParameters[0]->addPage(vector<ParameterInfo *>{ pPatternVolume, pMute, pDummy, pPlayingProb }, p);
    armedForMuteToggle[p] = 0;
  }

  Serial.println("MixMuteMode info mode: getting initial page");
  currentMenuPage = allSynthParameters[0]->getPage(0, 0);
  Serial.println("got initial page");
}

int Mode::effectivePartId(int partId) {  // map from logical part id 0-5 to effective partId for synthEngine
  return partId + globalState->engine->getNbParts() * globalState->partConfigMode.partTypes[partId];
}

// if buttons are moved -> do not mute/unmute on next button release
void MixMuteMode::processPotValue(int potIndex, int potVal, bool updateDisplay) {
  armedForMuteToggle[selectedLane] = 0; // the currently selected part is stored in the selectedLane field in this mode
  Mode::processPotValue(potIndex, potVal, updateDisplay);
}

bool MixMuteMode::pushButtonReleased(int buttonIndex) {

  Mode::pushButtonReleased(buttonIndex);

  if (buttonIndex >= NB_PARTS) return false; 
  if (armedForMuteToggle[buttonIndex]) {
    int oldValue = globalState->sequencerMode.sequencerActive[buttonIndex]->getValueDiscrete();
    globalState->sequencerMode.sequencerActive[buttonIndex]->setValue(!oldValue);
    armedForMuteToggle[buttonIndex] = 0;

    int paramPos = 1; // position of this parameter on display
    lcd->setCursor(4 * paramPos, 1);
    lcd->print("    ");
    lcd->setCursor(4 * paramPos, 1);

    char buffer[] = "____";
    currentMenuPage->at(paramPos)->renderPrintableValue(buffer);
    lcd->print(buffer);
    //lcd->print(currentMenuPage->at(paramPos)->printableValue());
  }
  return true;
}

void Mode::processPotValue(int potIndex, int potVal, bool updateDisplay) {
  currentMenuPage->at(potIndex)->updateParameter(potVal);

  // TODO; make this more efficient, this is costly
  // only refresh if value changed, and print all 4 digits in one go

  if (updateDisplay) {
    //lcd->setCursor(4 * potIndex, 1);
    //lcd->print("    ");
    //lcd->setCursor(4 * potIndex, 1);
    //lcd->print(currentMenuPage->at(potIndex)->printableValue());

    lcd->setCursor(4 * potIndex, 1);
    char buffer[] = "____";
    currentMenuPage->at(potIndex)->renderPrintableValue(buffer);
    lcd->print(buffer);
  }
}

// returned 'consumed': 0 not consumed, 1 registered shift keys, 2, actually did something
int Mode::handleGenericPushButtonEvents(int buttonIndex) {

  if (buttonIndex == 7) {  // P
    globalState->pPressed = true;
    return 1;
  }

  if (buttonIndex == 6) {  // shift
    globalState->shiftPressed = true;
    return 1;
  }

  if (globalState->pPressed) {  // mode switch
    Serial.println("mode switch");

    if ((buttonIndex == 0) && (globalState->selectedMode != &(globalState->sequencerMode))) {
      globalState->selectedMode = &(globalState->sequencerMode);
      lcd->setCursor(0, 0);
      lcd->print("  SEQUENCER         ");
      globalState->selectedMode->postPartOrModeSwitch();
    } else if (buttonIndex == 0) { // state is already sequencer
      globalState->selectedMode = &(globalState->sequencerModeGraphic);
      lcd->setCursor(0, 0);
      lcd->print("  SEQ GRAPHIC    ");
      globalState->selectedMode->postPartOrModeSwitch();
    } else if (buttonIndex == 3) {
      globalState->selectedMode = &(globalState->partConfigMode);
      lcd->setCursor(0, 0);
      lcd->print("  PART CONFIG      ");
      globalState->selectedMode->postPartOrModeSwitch();
    } else if (buttonIndex == 1) {
      globalState->selectedMode = &(globalState->synthMode);
      lcd->setCursor(0, 0);
      lcd->print("  SYNTH         ");
      globalState->selectedMode->postPartOrModeSwitch();
    } else if (buttonIndex == 2) {
      globalState->selectedMode = &(globalState->mixMuteMode);
      lcd->setCursor(0, 0);
      lcd->print("  MIX/MUTE      ");
      globalState->selectedMode->postPartOrModeSwitch();
    }

    this->globalState->delayedDisplayRefresh = millis() + 1000;

    return 2; 
  }

  if (globalState->shiftPressed) {  // part switch
    Serial.println("part switch");

    globalState->selectedPart = buttonIndex;  // TODO check nbParts

    postPartOrModeSwitch();  //this->currentMenuPage = allSynthParameters[globalState->selectedPart]->getPage(selectedLane, selectedPage);
    //fullDisplayUpdate();
    lcd->setCursor(0, 0);
    lcd->print("Part ");
    lcd->setCursor(5, 0);
    lcd->print(buttonIndex + 1);
    lcd->setCursor(6, 0);
    lcd->print("               ");
    this->globalState->delayedDisplayRefresh = millis() + 1000;

    return 2;
  }

  return 0;
}

bool Mode::pushButtonPressed(int buttonIndex) {

  if (parameterRequestingConfirmation != NULL) {
    return handleConfirmModeButtonPressed(buttonIndex);
  }

  // update state of shift,P
  // if other button: check if part or mode switch
  // otherwise parameter page switch

  int eventConsumed = handleGenericPushButtonEvents(buttonIndex);
  if (eventConsumed) return true;

  //int partId = globalState->selectedPart + 6 * globalState->partConfigMode.partTypes[globalState->selectedPart];

  int partId = effectivePartId(globalState->selectedPart); // TODO: this only makes sense for the synthMode !?
  SynthParameters *synthParameters = allSynthParameters[partId];

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
    return true;
  }

  return false;
}

void Mode::confirmationDisplayNotification() {
   int now = millis()/500; // in half seconds
  if (now != this->lastConfDisplayUpdate) {
    lcd->setCursor(4 * 3, 0);
    if (2*(now / 2) == now) {
      lcd->print("    ");
    } else {
      lcd->print("???");
    }
  }
}

void Mode::handleConfirmed() {
    parameterRequestingConfirmation = NULL;
    Serial.println("Confirmed");
    lcd->setCursor(4 * 3, 0);
    lcd->print("proc");
  }
void Mode::handleCancelled() {
    parameterRequestingConfirmation = NULL;
    Serial.println("Cancelled");
    lcd->setCursor(4 * 3, 0);
    lcd->print("    ");
  }

// in conform mode, we only look at buttons 6 and 7
bool Mode::handleConfirmModeButtonPressed(int buttonIndex) {

  if (buttonIndex == 6) {// cancel
    handleCancelled();
    return true;
  } else if (buttonIndex == 7) { // confirm
    handleConfirmed();
    return true;
  }
  return false;
}



bool MixMuteMode::pushButtonPressed(int buttonIndex) {

  // when doing parameter switch, respect the fact that we have only one synthparameter here

  int eventConsumed = handleGenericPushButtonEvents(buttonIndex);
  if (eventConsumed) return true;

  if (buttonIndex < NB_PARTS) armedForMuteToggle[buttonIndex] = 1;

  //int partId = globalState->selectedPart + 6 * globalState->partConfigMode.partTypes[globalState->selectedPart];
  SynthParameters *synthParameters = allSynthParameters[0];

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

  return true;
}

// confirmed, can be either load or save (either patch or global/program)
void PartConfigMode::handleConfirmed() {
  // find out WHAT was confirmed
  // we can look at the lane, or at the parameter that was asking for confirmation
  String paramName = this->parameterRequestingConfirmation->getUniqueName();
  Mode::handleConfirmed();

  //if ((buttonIndex == 5)&&(selectedLaneBefore == 5)) {  // save current state
  if((paramName == "PTCSave") || (paramName == "BNKSave")) {

    JsonDocument doc;
    //doc["PartParameters"] = JsonObject();
    JsonObject obj = doc["PartParameters"].to<JsonObject>();

    int effPartId = globalState->synthMode.effectivePartId(globalState->selectedPart);
    globalState->synthMode.serializePart(&obj, effPartId);
    

    char output[2560];
    int nbBytes = serializeJson(doc, output);

    String filename = String("Patch_")+this->selectedBank->getValueDiscrete()+"_"+this->selectedPatch->getValueDiscrete()+".json";
    Serial.println(filename);

    SD.remove(filename.c_str()); // don't append to existing file

    File dataFile = SD.open(filename.c_str(), FILE_WRITE);

    Serial.print(output);

    Serial.print("number bytes: ");
    Serial.println(nbBytes);

    // if the file is available, write the contents of datastring to it
    if (dataFile) {
      dataFile.println(output);

      dataFile.close();
      Serial.println("wrote file");
      lcd->setCursor(4 * 3, 0);
      lcd->print("w-ok");
    } else {
      Serial.println("error opening file for write");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    }
  }

  //if ((buttonIndex == 4)&&(selectedLaneBefore == 4)) {  // load saved state
  if((paramName == "PTCLoad") || (paramName == "BNKLoad")) {
    Serial.println("listing files");
    File dir = SD.open("/");
    File entry = dir.openNextFile();
    while (entry) {
      Serial.println(entry.name());
      entry.close();
      entry = dir.openNextFile();
    }
    Serial.println("done listing files");

    JsonDocument doc;
    //JsonObject obj = doc["PartParameters"].to<JsonObject>();
    String filename = String("Patch_")+this->selectedBank->getValueDiscrete()+"_"+this->selectedPatch->getValueDiscrete()+".json";
    Serial.println(filename);

    File dataFile = SD.open(filename.c_str());
    if (dataFile) {
      Serial.println("reading patch file:");

      deserializeJson(doc, dataFile);

      //Serial.println("read file:");
      //Serial.println("Deserialize, this is what we got");
      //serializeJson(doc["PartParameters"], Serial);

      JsonObject obj = doc["PartParameters"]; //.to<JsonObject>();
      //Serial.println(doc["PartParameters"].to<JsonObject>());
      int effPartId = globalState->synthMode.effectivePartId(globalState->selectedPart);
      globalState->synthMode.deserializePart(&obj, effPartId);
      // close the file:
      dataFile.close();

      Serial.println(freeram());

      lcd->setCursor(4 * 3, 0);
      lcd->print("l-ok");

    } else {
      Serial.println("error opening - patch does not exist");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    }
  }

  if((paramName == "GlobalBNKSave") || (paramName == "GlobalPRGSave")) {
    Serial.println("save global prg");

    JsonDocument doc;
    JsonObject obj = doc["Program"].to<JsonObject>();

    //globalState->synthMode.serializeSynthPart(&obj, globalState->selectedPart);
    globalState->serializeProgram(&obj);

    char output[25600];
    int nbBytes = serializeJson(doc, output);

    String filename = String("Program_")+this->selectedBank->getValueDiscrete()+"_"+this->selectedPatch->getValueDiscrete()+".json";
    Serial.println(filename);

    SD.remove(filename.c_str()); // don't append to existing file

    File dataFile = SD.open(filename.c_str(), FILE_WRITE);

    Serial.print(output);

    Serial.print("number bytes: ");
    Serial.println(nbBytes);

    // if the file is available, write the contents of datastring to it
    if (dataFile) {
      dataFile.println(output);

      dataFile.close();
      Serial.println("wrote file");
      lcd->setCursor(4 * 3, 0);
      lcd->print("w-ok");
    } else {
      Serial.println("error opening file for write");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    }
  }

  if((paramName == "GlobalBNKLoad") || (paramName == "GlobalPRGLoad")) {
    Serial.println("load global prg");
    JsonDocument doc;
    //JsonObject obj = doc["PartParameters"].to<JsonObject>();
    String filename = String("Program_")+this->selectedBank->getValueDiscrete()+"_"+this->selectedPatch->getValueDiscrete()+".json";
    Serial.println(filename);

    Serial.print("RAM before and after:");
    Serial.println(freeram());

    File dataFile = SD.open(filename.c_str());
    if (dataFile) {
      Serial.println("reading patch file:");

      deserializeJson(doc, dataFile);

      JsonObject obj = doc["Program"];
      //globalState->synthMode.deserializeSynthPart(&obj, globalState->selectedPart);
      globalState->deserializeProgram(&obj);
      // close the file:
      dataFile.close();

      Serial.println(freeram());
      
      lcd->setCursor(4 * 3, 0);
      lcd->print("l-ok");

    } else {
      Serial.println("error opening - patch does not exist");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    }
    
  }

  if((paramName == "nbVoices") || (paramName == "engineType")) {
    resetEngineTypeAndVoices();
    lcd->setCursor(4 * 3, 0);
    lcd->print("done");
  }

}

void PartConfigMode::resetEngineTypeAndVoices() {
  Serial.println("resetting all engine Types and nbVoices");

  // first disable all voices
  for (int p=0;p<NB_PARTS;p++) {
    int partId = globalState->synthMode.effectivePartId(p);
    Serial.print("effectivePartId ");
    Serial.println(partId);
    globalState->engine->getPart(partId)->setActiveNbVoices(0);
  }

  globalState->engine->markRequiredSignals();

  // set all synth engines
  for (int p=0;p<NB_PARTS;p++) {
    partTypes[p] = engineTypeParams[p]->getValueDiscrete();
  }

  // set the voices
  // total count can not be larger than 6
  // engine type 1 can only have 1 voice
  int maxVoiceAssign = 6;
  int nbVoicesAssigned = 0;
  for (int p=0;p<NB_PARTS;p++) {
    int partId = globalState->synthMode.effectivePartId(p);
    int requestedVoices = nbVoicesParams[p]->getValueDiscrete();
    Serial.print("effectivePartId ");
    Serial.println(partId);
    Serial.print("Engine ");
    Serial.print(partTypes[p]);
    Serial.print("/ nbVoices ");

    if ((partTypes[p] == 1) && (requestedVoices > 1)) {
      requestedVoices = 1;
      nbVoicesParams[p]->setValue(1);
    }

    if(nbVoicesAssigned +  requestedVoices <= maxVoiceAssign) { // can give it all the requested voices
      globalState->engine->getPart(partId)->setActiveNbVoices(requestedVoices);
      nbVoicesAssigned += requestedVoices;
      //Serial.println(requestedVoices);
    } else { // give as many voices as we have left
      globalState->engine->getPart(partId)->setActiveNbVoices(maxVoiceAssign - nbVoicesAssigned);
      nbVoicesParams[p]->setValue(maxVoiceAssign - nbVoicesAssigned); // reflect in parameter what is the actual voice number
      //Serial.println(maxVoiceAssign - nbVoicesAssigned);
      nbVoicesAssigned = maxVoiceAssign;
    }
    Serial.println(globalState->engine->getPart(partId)->getActiveNbVoices());
  }

  globalState->engine->markRequiredSignals();

}



void Mode::fullDisplayUpdate() {
  lcd->clear();
  for (unsigned int p = 0; p < currentMenuPage->size(); p++) {
    Serial.print(currentMenuPage->at(p)->getName());
    Serial.print("__");

    lcd->setCursor(4 * p, 0);
    lcd->print(currentMenuPage->at(p)->getName());
    lcd->setCursor(4 * p, 1);
    char buffer[] = "____";
    currentMenuPage->at(p)->renderPrintableValue(buffer);
    lcd->print(buffer);
    //lcd->print(currentMenuPage->at(p)->printableValue());
  }
}

bool SynthMode::pushButtonPressed(int buttonIndex) {
  Mode::pushButtonPressed(buttonIndex);

  if ((buttonIndex == 5) && (!globalState->shiftPressed)) {
    globalState->myNoteOn(globalState->selectedPart + 1, 36, 127);
  }

  return true;
}

bool Mode::pushButtonReleased(int buttonIndex) {
  if (buttonIndex == 7) {  // P
    globalState->pPressed = false;
    return false;
  }

  if (buttonIndex == 6) {  // shift
    globalState->shiftPressed = false;
    return false;
  }

  return false;
}

bool SynthMode::pushButtonReleased(int buttonIndex) {
  Mode::pushButtonReleased(buttonIndex);
  if (buttonIndex == 5) {
    globalState->myNoteOff(globalState->selectedPart + 1, 36, 127);
  }
  return false;
}

void SynthMode::postPartOrModeSwitch() {
  //int partId = globalState->selectedPart + 6 * globalState->partConfigMode.partTypes[globalState->selectedPart]; // TODO: hardcoded
  int partId = effectivePartId(globalState->selectedPart);
  this->currentMenuPage = allSynthParameters[partId]->getPage(selectedLane, selectedPage);
}

void PartConfigMode::postPartOrModeSwitch() {
  int partId = globalState->selectedPart;
  this->currentMenuPage = allSynthParameters[partId]->getPage(selectedLane, selectedPage);
}

void MixMuteMode::postPartOrModeSwitch() {
  int partId = globalState->selectedPart;
  this->currentMenuPage = allSynthParameters[0]->getPage(partId, 0);
}


void SequencerMode::setup() {
  for (int i = 0; i < globalState->engine->getNbParts(); i++) {
    lastPlayedNote.push_back(-1);
    sequencerActive[i] = new StaticSignalDiscrete(NULL, 1);
  }

  page = new StaticSignalDiscrete(NULL, 0);
  ParameterInfoDiscrete *pPage = new ParameterInfoDiscrete("PAG ", 0, 7, page, "SequencerPage");
  
  ParameterInfo *pDummy = new DummyParameterInfo("  ");
  this->doubleShiftParameters.push_back(pPage);
  this->doubleShiftParameters.push_back(pDummy);
  this->doubleShiftParameters.push_back(pDummy);
  this->doubleShiftParameters.push_back(pDummy);
}

void SequencerMode::processPotValue(int potIndex, int potVal, bool updateDisplay) {

  if (this->paramLockMode) return processLockParameter(potIndex, potVal);
  if (this->doubleShiftMode) return processDoubleShiftParameter(potIndex, potVal);


  if (potIndex == 0) {  // octave
    int oct = std::lround(1 + (potVal / 1024.0) * 4) - 2;
    //Sequence *s = this->globalState->sequences[this->globalState->selectedPart];
    Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
    s->data[0][cursorPos] = oct;
    displayOctAndNote();
  }
  if (potIndex == 1) {  // note
    int note = std::lround(0 + (potVal / 1024.0) * 12);
    //Sequence *s = this->globalState->sequences[this->globalState->selectedPart];
    Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
    s->data[1][cursorPos] = note;
    displayOctAndNote();
  }
}

void SequencerModeGraphic::processPotValue(int potIndex, int potVal, bool updateDisplay) {
  seqMode->processPotValue(potIndex, potVal, updateDisplay);
  this->armedForToggle = -1; // when pLocks were modified, don't toggle step on release
}

void SequencerMode::processLockParameter(int potIndex, int potVal) {
  // potVal is 0-3, referring to selected parameter or last used parameters
  // initial implementatino, just take the parameters that are currently active in synth mode
  int effectivePartId = this->effectivePartId(globalState->selectedPart);

  std::vector<ParameterInfo*> *pis = this->globalState->synthMode.allSynthParameters[effectivePartId]->getPage(this->globalState->synthMode.selectedLane, this->globalState->synthMode.selectedPage);
  ParameterInfo *lockParam = (*pis)[potIndex];

  // display locked value
  lcd->setCursor(4 * potIndex, 1);
  char buffer[] = "    ";
  lockParam->renderPrintableValueFromPotValue(buffer, potVal);
  lcd->print(buffer);
  //lcd->print(lockParam->printableValueFromPotValue(potVal));


  // store parameter value in sequence
  int selectedPart = this->globalState->selectedPart;
  //Sequence *s = this->globalState->sequences[selectedPart];
  Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];

  for (int lockPosition = 0; lockPosition < 4; lockPosition++) {
    if ((s->lockedParameters[lockPosition][cursorPos] == NULL) || (s->lockedParameters[lockPosition][cursorPos] == lockParam)) {
      s->lockedParameters[lockPosition][cursorPos] = lockParam;
      s->lockedValues[lockPosition][cursorPos] = potVal;
      Serial.print("locking parameter ");
      Serial.print(lockParam->getName());
      Serial.print(" in position ");
      Serial.println(lockPosition);
      return;
    }
  }
  Serial.println("all locking slots occupied");
}

void SequencerMode::processDoubleShiftParameter(int potIndex, int potVal) {
  ParameterInfo *param = this->doubleShiftParameters[potIndex];

  param->updateParameter(potVal);

  // display locked value
  lcd->setCursor(4 * potIndex, 1);
  lcd->print("   ");
  lcd->setCursor(4 * potIndex, 1);
  char buffer[] = "____";
  param->renderPrintableValue(buffer);
  lcd->print(buffer);
  //lcd->print(param->printableValue());

}


bool SequencerMode::pushButtonPressed(int buttonIndex) {

  int consumed = handleGenericPushButtonEvents(buttonIndex);
  if (consumed == 2) return true;

  if (consumed == 1) { // one of the shift buttons was pressed
    if (globalState->shiftPressed && globalState->pPressed) { // now both shift buttons are pressed
      this->doubleShiftMode = true;
      // put the actual values into the parameters
      int page = cursorPos / 8;
      this->page->setValue(page);
      displayDoubleShiftMode();
      return true;
    }
  }

  int patternLength = patternLengths[this->globalState->selectedPart]->getValueDiscrete();

  if (buttonIndex == 0) {
    cursorPos = cursorPos - 1;
    if (cursorPos < 0) cursorPos += patternLength;
    displayStep();
    displayOctAndNote();
  }

  if (buttonIndex == 1) {
    cursorPos = cursorPos + 1;
    if (cursorPos >= patternLength) cursorPos -= patternLength;
    displayStep();
    displayOctAndNote();
  }

  if (buttonIndex == 2) {
    //Sequence *s = this->globalState->sequences[this->globalState->selectedPart];
    Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
    int currentValue = s->data[2][cursorPos];
    int newValue = 1 - currentValue;

    Serial.print("new value at position:");
    Serial.print(cursorPos);
    Serial.print(":");
    Serial.println(newValue);
    s->data[2][cursorPos] = newValue;

    displayOctAndNote();
  }

  if (buttonIndex == 3) {
    cursorPos = cursorPos + 8;
    if (cursorPos >= patternLength) cursorPos -= patternLength;
    displayStep();
    displayOctAndNote();
  }

  if (buttonIndex == 4) {  // parameter lock 
    this->paramLockMode = 1;
    displayLockingParams();
  }

  if (buttonIndex == 5) {  // play/stop
    this->globalState->seqPlaying = !this->globalState->seqPlaying;
    if (this->globalState->seqPlaying) {
      this->playHead = 0;
      this->nextTriggerTime = millis();  // 0; // means: in the next call a step 0 is going to be played
    } else {                             // switch off current note
      // of all parts
      for (int i = 0; i < globalState->engine->getNbParts(); i++) {
        globalState->myNoteOff(i + 1, lastPlayedNote[i], 0);  // TODO: mapping from seq part to midi channel
        for (int lockPosition = 0; lockPosition < 4; lockPosition++) {
          if (this->parametersToReset[i * 4 + lockPosition] != NULL) {
            this->parametersToReset[i * 4 + lockPosition]->unlock();
            this->parametersToReset[i * 4 + lockPosition] = NULL;
          }
        }
      }
    }

    displayPlayStatus();
  }

  return true;  // TODO: do we really need to lock pots after all buttons here?
}

bool SequencerModeGraphic::pushButtonPressed(int buttonIndex) {

  int consumed = handleGenericPushButtonEvents(buttonIndex);
  if (consumed == 2) return true;

  if (consumed == 1) { // one of the shift buttons was pressed
    if (globalState->shiftPressed && globalState->pPressed) { // now both shift buttons are pressed
      seqMode->doubleShiftMode = true;
      seqMode->paramLockMode = false;
      
      // put the actual values into the parameters
      int page = seqMode->cursorPos / 8;
      seqMode->page->setValue(page);
      seqMode->displayDoubleShiftMode();
      return true;
    }
  }

  // every button press goes into pLock mode
  seqMode->paramLockMode = 1;
  seqMode->displayLockingParams();

  // arm to toggle this step on button release
  armedForToggle = buttonIndex;

  int page = seqMode->cursorPos / 8; 

  // move cursor to step
  seqMode->cursorPos = page * 8 + buttonIndex; 
  // toggle this step
  /*
  Sequence *s = this->globalState->sequences[this->globalState->selectedPart];
  int currentValue = s->data[2][seqMode->cursorPos];
  int newValue = 1 - currentValue;

  Serial.print("new value at position:");
  Serial.print(seqMode->cursorPos);
  Serial.print(":");
  Serial.println(newValue);
  s->data[2][seqMode->cursorPos] = newValue;
  */

  //seqMode->displayStep();
  //seqMode->displayOctAndNote();

  // every button is a step change, and goes into pLock mode -> lock all pots
  return true;

}


bool SequencerMode::pushButtonReleased(int buttonIndex) {
  //Serial.print("pushButtonReleased seq mode:");
  bool needToLock = Mode::pushButtonReleased(buttonIndex);
  //Serial.println(buttonIndex);
  if (buttonIndex == 4) {
    this->paramLockMode = 0;

    fullDisplayUpdate();
    needToLock = true;
  }

  if (this->doubleShiftMode) {
    if ((buttonIndex == 6) || (buttonIndex == 7)) { // we are no longer in double shift mode
      doubleShiftMode = false;
      int stepInPage = cursorPos % 8;
      cursorPos = page->getValueDiscrete() * 8 + stepInPage;
      fullDisplayUpdate();
      needToLock = true;
    }
  }
  return needToLock;
}

bool SequencerModeGraphic::pushButtonReleased(int buttonIndex) {
  
  bool needToLock = Mode::pushButtonReleased(buttonIndex);

  if (seqMode->doubleShiftMode) {
    if ((buttonIndex == 6) || (buttonIndex == 7)) { // we are no longer in double shift mode
      seqMode->doubleShiftMode = false;
      seqMode->paramLockMode = false;
      armedForToggle = -1;
      int stepInPage = seqMode->cursorPos % 8;
      seqMode->cursorPos = seqMode->page->getValueDiscrete() * 8 + stepInPage;
      fullDisplayUpdate();
      needToLock = true;
    }
  }
  

  if (armedForToggle == buttonIndex) {
    //Sequence *s = this->globalState->sequences[this->globalState->selectedPart];
    Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
    int currentValue = s->data[2][seqMode->cursorPos];
    int newValue = 1 - currentValue;

    Serial.print("new value at position:");
    Serial.print(seqMode->cursorPos);
    Serial.print(":");
    Serial.println(newValue);
    s->data[2][seqMode->cursorPos] = newValue;

    
  }
  
  //seqMode->displayStep();
  //seqMode->displayOctAndNote();
  if(seqMode->paramLockMode) {
    needToLock = true; // returning from pLock mode
    seqMode->paramLockMode = 0;
    this->fullDisplayUpdate();
  }

  // if we e.g. just switched into this mode, don't do anything on button release. displaz will be refreshed by delayed update.

  return needToLock;
}

void SequencerMode::fullDisplayUpdate() {
  lcd->clear();
  lcd->setCursor(0, 0);

  // Part
  lcd->print(String("P") + (this->globalState->selectedPart + 1));

  displayStep();

  displayOctAndNote();
  displayPlayStatus();
}

void SequencerModeGraphic::fullDisplayUpdate() {
  seqMode->fullDisplayUpdate();
}

void SequencerMode::displayLockingParams() {
  lcd->clear();
  //lcd->setCursor(0, 0);
  //lcd->print("Locking parameters");

  //lcd->clear();


  for (unsigned int p = 0; p < 4; p++) {
    int effectivePartId = this->effectivePartId(globalState->selectedPart);
    //ParameterInfo *pinfo = this->globalState->partConfigMode.lockParameters[partId * 4 + p];
    std::vector<ParameterInfo*> *pis = this->globalState->synthMode.allSynthParameters[effectivePartId]->getPage(this->globalState->synthMode.selectedLane, this->globalState->synthMode.selectedPage);
    ParameterInfo *lockParam = (*pis)[p];

    Serial.print(lockParam->getName());
    Serial.print("__");

    lcd->setCursor(4 * p, 0);
    lcd->print(lockParam->getName());
    lcd->setCursor(4 * p, 1);
    char buffer[] = "____";
    lockParam->renderPrintableValue(buffer);
    lcd->print(buffer);
    //lcd->print(lockParam->printableValue());  // TODO: if there is already a parameter lock, print this value instead
  }
}

void SequencerMode::displayDoubleShiftMode() {
  lcd->clear();
  for (unsigned int p = 0; p < 4; p++) {
    ParameterInfo *param = doubleShiftParameters[p];

    Serial.print(param->getName());
    Serial.print("__");

    lcd->setCursor(4 * p, 0);
    lcd->print(param->getName());
    lcd->setCursor(4 * p, 1);
    
    //lcd->print(param->printableValue()); 
    char buffer[] = "    ";
    param->renderPrintableValue(buffer);
    lcd->print(buffer); 
  }
}

/*void SequencerModeGraphic::displayLockingParams() {
  seqMode->displayLockingParams();
}*/

void SequencerMode::displayStep() {
  // Step
  lcd->setCursor(3, 0);  // X,Y
  lcd->print("S  ");
  lcd->setCursor(4, 0);
  lcd->print(this->cursorPos);

  String stepVisu = String("");
  int page = cursorPos / 8;
  for (int step=0;step<8;step++) {
    String thisChar = (page*8+step == cursorPos) ? String("|") : String(" ");
    stepVisu = stepVisu + thisChar;
  }
  lcd->setCursor(6,1);
  lcd->print(stepVisu);
}

void SequencerMode::displayOctAndNote() {
  //Sequence *s = this->globalState->sequences[this->globalState->selectedPart];
  Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
  // oct and note
  lcd->setCursor(0, 1);
  lcd->print(String("O") + String(s->data[0][cursorPos]) + String(" N") + String(s->data[1][cursorPos]) + String(" "));
  lcd->setCursor(7, 1);
  //lcd->print(s->data[2][cursorPos] ? "+" : "-");

  String stepVisu = String("");
  int page = cursorPos / 8;
  for (int step=0;step<8;step++) {
    String thisChar = s->data[2][page*8+step] ? "+" : "_";
    stepVisu = stepVisu + thisChar;
  }
  lcd->setCursor(6,0);
  lcd->print(stepVisu);
}

void SequencerMode::displayPlayStatus() {
  // play status
  lcd->setCursor(15, 0);
  lcd->print(globalState->seqPlaying ? "P" : "-");
  lcd->setCursor(14, 1);
  lcd->print("__");
  lcd->setCursor(14, 1);
  lcd->print(this->playHead % nbSteps);
}


void SequencerMode::maybePlay() {
  if (millis() > nextTriggerTime) {
    nextTriggerTime = nextTriggerTime + interBeatMs;
    Serial.print("playing step ");
    Serial.println(playHead);

    for (int part = 0; part < NB_PARTS; part++) {  // TODO: play all parts

      if (sequencerActive[part]->getValueDiscrete() == 0) {
        if (lastPlayedNote[part] > -1) globalState->myNoteOff(part + 1, lastPlayedNote[part], 0);
        lastPlayedNote[part] = -1;
        continue;
      }

      //Sequence *s = this->globalState->sequences[this->globalState->selectedPart]; // for now only play part that is being edited

      int wrappedPlayHead = playHead % patternLengths[part]->getValueDiscrete();

      //Sequence *s = this->globalState->sequences[part];
      Sequence *s = this->globalState->patterns[globalState->selectedPattern][part];
      if (s->data[2][wrappedPlayHead]) {
        // stop previous note
        //globalState->myNoteOff(globalState->selectedPart+1, lastPlayedNote, 0);

        if (lastPlayedNote[part] > -1) globalState->myNoteOff(part + 1, lastPlayedNote[part], 0);
        int newNote = 36 + s->data[0][wrappedPlayHead] * 12 + s->data[1][wrappedPlayHead];
        lastPlayedNote[part] = newNote;
        // play
        //globalState->myNoteOn(globalState->selectedPart+1, newNote, 127);

        // automate locked parameter and/or reset original parameter values

        for (int lockPosition = 0; lockPosition < 4; lockPosition++) {

          if (this->parametersToReset[part * 4 + lockPosition] != NULL) {  // for the first note in the sequence after a restart, we will call unlock here, even though it is unlocked
            this->parametersToReset[part * 4 + lockPosition]->unlock();
          }

          this->parametersToReset[part * 4 + lockPosition] = s->lockedParameters[lockPosition][wrappedPlayHead];  // also copy NULLs from lockedParameters
          if (s->lockedParameters[lockPosition][wrappedPlayHead] != NULL) {

            //this->valuesToReset[part*4 + lockPosition] = s->lockedParameters[lockPosition][playHead]->getValue();
            //s->lockedParameters[lockPosition][playHead]->updateParameter(s->lockedValues[lockPosition][playHead]);
            s->lockedParameters[lockPosition][wrappedPlayHead]->lock(s->lockedValues[lockPosition][wrappedPlayHead]);
          }
        }
        globalState->myNoteOn(part + 1, newNote, 127);
        Serial.println(newNote);
      }
    }

    if ((playHead % 4 == 0) && (this->globalState->selectedMode == this)) {
      displayPlayStatus();
    }
    playHead = (playHead + 1);  // % nbSteps;

    // recalculate tempo: TODO, can we do that more rarely?
    interBeatMs = 60000 / (bpm->value * 4);
  }
}

void Mode::serializePart(JsonObject *jsonObject, int partId) {

  
  SynthParameters *params = this->allSynthParameters[partId];
  int nbLanes = params->getNbLanes();
  for (int lane = 0; lane < nbLanes; lane++) {

    int nbPages = params->getNbPages(lane);
    for (int page = 0; page < nbPages; page++) {
      std::vector<ParameterInfo *> *pOnPage = params->getPage(lane, page);
      for (int elementId = 0; elementId < pOnPage->size(); elementId++) {
        ParameterInfo *pinfo = (*pOnPage)[elementId];
        (*jsonObject)[pinfo->getUniqueName()] = pinfo->getValue();
        Serial.print("Serializing ");
        Serial.print(pinfo->getName());
        Serial.println(pinfo->getValue());

      }
    }
  }
}



void Mode::deserializePart(JsonObject *jsonObject,  int partId) {
  
  
  SynthParameters *params = this->allSynthParameters[partId];
  int nbLanes = params->getNbLanes();
  for (int lane = 0; lane < nbLanes; lane++) {

    int nbPages = params->getNbPages(lane);
    for (int page = 0; page < nbPages; page++) {
      std::vector<ParameterInfo *> *pOnPage = params->getPage(lane, page);
      for (int elementId = 0; elementId < pOnPage->size(); elementId++) {
        ParameterInfo *pinfo = (*pOnPage)[elementId];
        //(*jsonObject)[pinfo->getUniqueName()] = pinfo->getValue();

        // TODO: check (doc["value"].is<int>())
        if((*jsonObject)[pinfo->getUniqueName()].is<float>()) {
          float extractedValue = (*jsonObject)[pinfo->getUniqueName()];
          pinfo->setValue(extractedValue);
        }
        //Serial.print("Deserialized ");
        //Serial.print(pinfo->getName());
        //Serial.println(extractedValue);
      }
    }
  }
}

ParameterInfo* Mode::getParameterByNameAndPart(String uniqueName, int partId) {

  SynthParameters *params = this->allSynthParameters[partId];
  int nbLanes = params->getNbLanes();
  for (int lane = 0; lane < nbLanes; lane++) {

    int nbPages = params->getNbPages(lane);
    for (int page = 0; page < nbPages; page++) {
      std::vector<ParameterInfo *> *pOnPage = params->getPage(lane, page);
      for (int elementId = 0; elementId < pOnPage->size(); elementId++) {
        ParameterInfo *pinfo = (*pOnPage)[elementId];
        if (pinfo->getUniqueName() == uniqueName) {
          return pinfo;
        }
      }
    }
  }
  return NULL;
}

void GlobalState::serializeProgram(JsonObject *prg) {

  // JsonObject  add<JsonObject>() const;   // adds a new empty object
  // *** synth parameters
  JsonArray patchList = (*prg)["PartParameters"].to<JsonArray>();

  for (int i=0;i<NB_PARTS;i++) {
    int effPartId = synthMode.effectivePartId(i);
    JsonObject partParameters = patchList.add<JsonObject>();
    synthMode.serializePart(&partParameters, effPartId);
  }

  // *** partConfig
  JsonArray configList = (*prg)["PartConfig"].to<JsonArray>();

  for (int i=0;i<NB_PARTS;i++) {
    JsonObject partConfigParameters = configList.add<JsonObject>();
    partConfigMode.serializePart(&partConfigParameters, i);
  }

  // *** sequencer
  JsonArray seqdata = (*prg)["SequencerData"].to<JsonArray>();

  for (int i=0;i<NB_PARTS;i++) {
    JsonObject partSeqData = seqdata.add<JsonObject>();
    sequencerMode.serializeSequencerData(&partSeqData, i);
  }

}

void GlobalState::deserializeProgram(JsonObject *prg) {
 

  // *** partConfig
  JsonArray configList = (*prg)["PartConfig"]; //.to<JsonArray>();

  for (int i=0;i<NB_PARTS;i++) {
    JsonObject partConfigParameters = configList[i];
    partConfigMode.deserializePart(&partConfigParameters, i);
  }

  // based on part config, select the correct sound engines and voice numbers
  // needs to happen before loading synth params
  this->partConfigMode.resetEngineTypeAndVoices(); 

  // *** synth parameters
  JsonArray patchList = (*prg)["PartParameters"];//.to<JsonArray>();

  for (int i=0;i<NB_PARTS;i++) {
    int effPartId = synthMode.effectivePartId(i);

    JsonObject partParameters = patchList[i]; //.to<JsonObject>(); // this breaks it
    synthMode.deserializePart(&partParameters, effPartId);
  }


  // *** sequencer
  JsonArray seqdata = (*prg)["SequencerData"]; 

  for (int i=0;i<NB_PARTS;i++) {
    JsonObject partSeqData = seqdata[i]; 
    sequencerMode.deserializeSequencerData(&partSeqData, i);
  }

}

void SequencerMode::serializeSequencerData(JsonObject *seqData, int partId) {
  Serial.println("serializeSequencerData");

  JsonArray patterns = (*seqData)["Patterns"].to<JsonArray>();
  //Sequence *s = this->globalState->sequences[partId];
  
  // TODO: Loop, to add multiple patterns
  Sequence *s = this->globalState->patterns[globalState->selectedPattern][partId];

  JsonArray pattern = patterns.add<JsonArray>();
  for(int step = 0; step<s->NB_STEPS; step++) {
    JsonObject stepData = pattern.add<JsonObject>();
    stepData["octave"] = s->data[0][step];
    stepData["note"] = s->data[1][step];
    stepData["on/off"] = s->data[2][step];

    JsonArray lockedParams = stepData["pLocks"].to<JsonArray>();
    int pIndex = 0;
    while((pIndex < 4) && (s->lockedParameters[pIndex][step]!=NULL)) {
      JsonObject onePlock = lockedParams.add<JsonObject>();
      onePlock["name"] = s->lockedParameters[pIndex][step]->getUniqueName();
      //onePlock["value"] = s->lockedParameters[pIndex][step]->getValue();
      onePlock["value"] = s->lockedValues[pIndex][step];
      pIndex++;
    }

  }

}

void SequencerMode::deserializeSequencerData(JsonObject *seqData, int partId) {
  Serial.println("deserializeSequencerData");

  JsonArray patterns = (*seqData)["Patterns"]; 

  //Sequence *s = this->globalState->sequences[partId];
  
  // TODO: Loop, to add multiple patterns
  Sequence *s = this->globalState->patterns[globalState->selectedPattern][partId];

  JsonArray pattern = patterns[0]; 
  for(int step = 0; step<pattern.size(); step++) {
    JsonObject stepData = pattern[step]; 
    s->data[0][step] = stepData["octave"];
    s->data[1][step] = stepData["note"];
    s->data[2][step] = stepData["on/off"];

    // first delete existing pLocks
    for(int pIndex = 0; pIndex < s->lockedParameters.size(); pIndex++) {
      s->lockedParameters[pIndex][step] = NULL;
    }
    
    if (stepData["pLocks"].is<JsonArray>()) {
      JsonArray pLocks = stepData["pLocks"];
      for(int pIndex = 0; pIndex < pLocks.size(); pIndex++) {
        String uniqueName = pLocks[pIndex]["name"];
        Serial.print("Searching param ");
        Serial.print(uniqueName);
        Serial.print(" is null? ");
        float value = pLocks[pIndex]["value"];
        // TODO mapping from sequencer Part ID to synth Part ID via midi channel settings, for now 1:1
        int synthPartId = this->effectivePartId(partId);
        ParameterInfo *paramToLock = globalState->synthMode.getParameterByNameAndPart(uniqueName, synthPartId);
        Serial.println(paramToLock == NULL);
        if(paramToLock != NULL) {
          Serial.println(value);
          s->lockedParameters[pIndex][step] = paramToLock;
          s->lockedValues[pIndex][step] = value;
        }
      }
    }

  }

}
