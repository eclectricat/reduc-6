#include "Modes.h"
#include "SynthEngine.h"
#include <LiquidCrystal.h>
#include <ArduinoJson.h>
#include <SD.h>

static const char *kCookieBoxSaveDir = "/CookieBoxV2";
static const char *kPatchPrefix = "Patch_";
static const char *kProgramPrefix = "Program_";
static const char *kSaveExtension = ".cb2";
static const size_t kLineJsonDocSize = 512;

static bool ensureSaveDirectory() {
  if (SD.exists(kCookieBoxSaveDir)) return true;
  return SD.mkdir(kCookieBoxSaveDir);
}

static bool writeJsonLine(File &dataFile, JsonDocument &doc) {
  if (!dataFile) return false;
  size_t bytes = serializeJson(doc, dataFile);
  dataFile.write('\n');
  return bytes > 0;
}

static String patchSlotFilename(int bank, int patch) {
  return String(kCookieBoxSaveDir) + "/" + String(kPatchPrefix) + bank + "_" + patch + String(kSaveExtension);
}

static String programSlotFilename(int bank, int patch) {
  return String(kCookieBoxSaveDir) + "/" + String(kProgramPrefix) + bank + "_" + patch + String(kSaveExtension);
}

static bool patchSlotTaken(int bank, int patch) {
  return SD.exists(patchSlotFilename(bank, patch).c_str());
}

static bool programSlotTaken(int bank, int patch) {
  return SD.exists(programSlotFilename(bank, patch).c_str());
}

static bool slotCacheInitialized = false;
static bool patchSlotCache[100] = {false};
static bool programSlotCache[100] = {false};

static void refreshSaveSlotCache() {
  for (int i = 0; i < 100; i++) {
    patchSlotCache[i] = false;
    programSlotCache[i] = false;
  }

  if (!ensureSaveDirectory()) {
    slotCacheInitialized = true;
    return;
  }

  File dir = SD.open(kCookieBoxSaveDir);
  if (!dir) {
    slotCacheInitialized = true;
    return;
  }

  File entry = dir.openNextFile();
  while (entry) {
    String name = entry.name();
    if (name.startsWith(kPatchPrefix) && name.endsWith(kSaveExtension)) {
      String core = name.substring(strlen(kPatchPrefix), name.length() - strlen(kSaveExtension));
      int sep = core.indexOf('_');
      if (sep > 0) {
        int bank = core.substring(0, sep).toInt();
        int patch = core.substring(sep + 1).toInt();
        if (bank >= 0 && bank < 10 && patch >= 0 && patch < 10) {
          patchSlotCache[bank * 10 + patch] = true;
        }
      }
    } else if (name.startsWith(kProgramPrefix) && name.endsWith(kSaveExtension)) {
      String core = name.substring(strlen(kProgramPrefix), name.length() - strlen(kSaveExtension));
      int sep = core.indexOf('_');
      if (sep > 0) {
        int bank = core.substring(0, sep).toInt();
        int patch = core.substring(sep + 1).toInt();
        if (bank >= 0 && bank < 10 && patch >= 0 && patch < 10) {
          programSlotCache[bank * 10 + patch] = true;
        }
      }
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  slotCacheInitialized = true;
}

static bool patchSlotCached(int bank, int patch) {
  if (!slotCacheInitialized) refreshSaveSlotCache();
  if (bank < 0 || bank >= 10 || patch < 0 || patch >= 10) return false;
  return patchSlotCache[bank * 10 + patch];
}

static bool programSlotCached(int bank, int patch) {
  if (!slotCacheInitialized) refreshSaveSlotCache();
  if (bank < 0 || bank >= 10 || patch < 0 || patch >= 10) return false;
  return programSlotCache[bank * 10 + patch];
}

static void writeParameterLines(File &dataFile, Mode *mode, int partId, const char *kind) {
  SynthParameters *params = mode->allSynthParameters[partId];
  int nbLanes = params->getNbLanes();
  for (int lane = 0; lane < nbLanes; lane++) {
    int nbPages = params->getNbPages(lane);
    for (int page = 0; page < nbPages; page++) {
      std::vector<ParameterInfo *> *pOnPage = params->getPage(lane, page);
      for (int elementId = 0; elementId < pOnPage->size(); elementId++) {
        ParameterInfo *pinfo = (*pOnPage)[elementId];
        StaticJsonDocument<kLineJsonDocSize> lineDoc;
        lineDoc["kind"] = kind;
        lineDoc["part"] = partId;
        lineDoc["name"] = pinfo->getUniqueName();
        lineDoc["value"] = pinfo->getValue();
        writeJsonLine(dataFile, lineDoc);
      }
    }
  }
}

static void writePatchParameterLines(File &dataFile, Mode *mode, int partId) {
  SynthParameters *params = mode->allSynthParameters[partId];
  int nbLanes = params->getNbLanes();
  for (int lane = 0; lane < nbLanes; lane++) {
    int nbPages = params->getNbPages(lane);
    for (int page = 0; page < nbPages; page++) {
      std::vector<ParameterInfo *> *pOnPage = params->getPage(lane, page);
      for (int elementId = 0; elementId < pOnPage->size(); elementId++) {
        ParameterInfo *pinfo = (*pOnPage)[elementId];
        StaticJsonDocument<kLineJsonDocSize> lineDoc;
        lineDoc["kind"] = "patch-param";
        lineDoc["name"] = pinfo->getUniqueName();
        lineDoc["value"] = pinfo->getValue();
        writeJsonLine(dataFile, lineDoc);
      }
    }
  }
}

static void writeSequencerNoteLine(File &dataFile, GlobalState *globalState, int partId, int patternId, int step, const NoteEntry &note) {
  StaticJsonDocument<kLineJsonDocSize> lineDoc;
  lineDoc["kind"] = "program-sequencer-note";
  lineDoc["part"] = partId;
  lineDoc["pattern"] = patternId;
  lineDoc["step"] = step;
  lineDoc["octave"] = note.octave;
  lineDoc["note"] = note.note;
  lineDoc["on"] = note.on;
  lineDoc["length"] = note.length;
  JsonArray locks = lineDoc["locks"].to<JsonArray>();
  int32_t lockNode = note.lockHead;
  while (lockNode != -1) {
    JsonObject oneLock = locks.add<JsonObject>();
    oneLock["name"] = globalLockNodes[lockNode].param->getUniqueName();
    oneLock["value"] = globalLockNodes[lockNode].value;
    lockNode = globalLockNodes[lockNode].next;
  }
  writeJsonLine(dataFile, lineDoc);
}

static void writeProgramFile(File &dataFile, GlobalState *globalState) {
  StaticJsonDocument<kLineJsonDocSize> headerDoc;
  headerDoc["kind"] = "program-header";
  headerDoc["format"] = "CookieBoxV2";
  headerDoc["version"] = 1;
  writeJsonLine(dataFile, headerDoc);

  for (int i = 0; i < NB_PARTS; i++) {
    writeParameterLines(dataFile, &globalState->partConfigMode, i, "program-part-config");
  }

  for (int i = 0; i < NB_PARTS; i++) {
    writeParameterLines(dataFile, &globalState->synthMode, i, "program-part-param");
  }

  for (int patternId = 0; patternId < NB_PATTERNS; patternId++) {
    for (int partId = 0; partId < NB_PARTS; partId++) {
      Sequence *s = globalState->patterns[patternId][partId];
      for (int step = 0; step < Sequence::NB_STEPS; step++) {
        int32_t noteIndex = s->steps[step];
        if (noteIndex == -1) continue;
        const NoteEntry &note = globalNotes[noteIndex];
        writeSequencerNoteLine(dataFile, globalState, partId, patternId, step, note);
      }
    }
  }
}

static void clearAllSequences(GlobalState *globalState) {
  for (int patternId = 0; patternId < NB_PATTERNS; patternId++) {
    for (int partId = 0; partId < NB_PARTS; partId++) {
      Sequence *s = globalState->patterns[patternId][partId];
      if (s) {
        Sequence::clearSequence(s);
      }
    }
  }
}

static void loadProgramNewFormat(File &dataFile, GlobalState *globalState) {
  clearAllSequences(globalState);
  globalState->partConfigMode.resetEngineTypeAndVoices();

  char lineBuffer[512];
  while (dataFile.available()) {
    size_t len = dataFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
    lineBuffer[len] = '\0';
    if (len == 0) continue;

    StaticJsonDocument<kLineJsonDocSize> lineDoc;
    DeserializationError err = deserializeJson(lineDoc, lineBuffer);
    if (err) continue;

    const char *kind = lineDoc["kind"];
    if (!kind) continue;

    if (strcmp(kind, "program-part-config") == 0) {
      int partId = lineDoc["part"];
      String name = lineDoc["name"].as<String>();
      float value = lineDoc["value"];
      ParameterInfo *param = globalState->partConfigMode.getParameterByNameAndPart(name, partId);
      if (param) param->setValue(value);
      continue;
    }

    if (strcmp(kind, "program-part-param") == 0) {
      int partId = lineDoc["part"];
      String name = lineDoc["name"].as<String>();
      float value = lineDoc["value"];
      int effPartId = globalState->synthMode.effectivePartId(partId);
      ParameterInfo *param = globalState->synthMode.getParameterByNameAndPart(name, effPartId);
      if (param) param->setValue(value);
      continue;
    }

    if (strcmp(kind, "program-sequencer-note") == 0) {
      int partId = lineDoc["part"];
      int patternId = lineDoc["pattern"];
      int step = lineDoc["step"];
      if (partId < 0 || partId >= NB_PARTS) continue;
      if (patternId < 0 || patternId >= NB_PATTERNS) continue;
      if (step < 0 || step >= Sequence::NB_STEPS) continue;

      Sequence *s = globalState->patterns[patternId][partId];
      if (!s) continue;
      int32_t existingNoteIndex = s->steps[step];
      if (existingNoteIndex != -1) {
        Sequence::clearNoteLocks(existingNoteIndex);
        Sequence::freeGlobalNoteIndex(existingNoteIndex);
        s->steps[step] = -1;
      }

      NoteEntry note;
      note.octave = lineDoc["octave"];
      note.note = lineDoc["note"];
      note.on = lineDoc["on"];
      note.length = lineDoc["length"];
      note.lockHead = -1;

      int32_t noteIndex = Sequence::allocateGlobalNote(note);
      if (noteIndex == -1) continue;
      s->steps[step] = noteIndex;

      if (lineDoc["locks"].is<JsonArray>()) {
        JsonArray locks = lineDoc["locks"].as<JsonArray>();
        int effPartId = globalState->synthMode.effectivePartId(partId);
        for (JsonObject lockEntry : locks) {
          String lockName = lockEntry["name"].as<String>();
          float lockValue = lockEntry["value"];
          ParameterInfo *param = globalState->synthMode.getParameterByNameAndPart(lockName, effPartId);
          if (param) {
            Sequence::addOrUpdateNoteLock(noteIndex, param, lockValue);
          }
        }
      }
      continue;
    }
  }
}

static void loadPatchNewFormat(File &dataFile, GlobalState *globalState, int selectedPart) {
  int effPartId = globalState->synthMode.effectivePartId(selectedPart);
  char lineBuffer[512];
  while (dataFile.available()) {
    size_t len = dataFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
    lineBuffer[len] = '\0';
    if (len == 0) continue;

    StaticJsonDocument<kLineJsonDocSize> lineDoc;
    DeserializationError err = deserializeJson(lineDoc, lineBuffer);
    if (err) continue;

    const char *kind = lineDoc["kind"];
    if (!kind) continue;
    if (strcmp(kind, "patch-param") != 0) continue;

    String name = lineDoc["name"].as<String>();
    float value = lineDoc["value"];
    ParameterInfo *param = globalState->synthMode.getParameterByNameAndPart(name, effPartId);
    if (param) param->setValue(value);
  }
}

int freeram() {
  return (char *)&_heap_end - __brkval;
}


GlobalState::GlobalState(SynthEngine *engine, LiquidCrystal *lcd)
  : synthMode(lcd), partConfigMode(lcd), sequencerMode(lcd, engine), mixMuteMode(lcd), sequencerModeGraphic(lcd), keyboardMode(lcd) {
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
  keyboardMode.globalState = this;
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
  for (int pat = 0; pat < NB_PATTERNS; pat++) {
    //patterns.push_back(std::vector<Sequence*>());
    for (int track = 0; track < this->engine->getNbParts(); track++) {
      //patterns[pat].push_back(new Sequence());
      //patterns[pat][track] = new Sequence();
      Serial.println("create sequence:");
      Serial.println(pat);
      Serial.println(track);
      Serial.println(freeram());
      Serial.println(patterns[pat][track] == NULL);
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

  seqActionDest = new StaticSignalDiscrete(NULL, 0);
  seqActionOp = new StaticSignalDiscrete(NULL, 0);
  std::vector<String> destinations = {"seq", "ptn"};
  std::vector<String> actions = {"clr", "cop", "pas"};

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

  ParameterInfo *pSeqAction = new DummyParameterInfo("ACT");
  ParameterInfo *pSeqActionDest = new ParameterInfoDiscreteConfirmation("DST", 0, destinations.size()-1, seqActionDest, "actDst", &(this->parameterRequestingConfirmation), destinations);
  ParameterInfo *pSeqActionOp = new ParameterInfoDiscreteConfirmation("OP", 0, actions.size()-1, seqActionOp, "actOp", &(this->parameterRequestingConfirmation), actions);


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
    StaticSignalDiscrete *nbVoices = new StaticSignalDiscrete(NULL, (p < 4 ? 1 : 0));

    this->engineTypeParams[p] = engineType;
    this->nbVoicesParams[p] = nbVoices;

    ParameterInfo *pEngineType = new ParameterInfoDiscreteConfirmation("ENG", 0, 1, engineType, "engineType", &(this->parameterRequestingConfirmation));
    ParameterInfo *pNbVoices = new ParameterInfoDiscreteConfirmation("VOC", 0, 6, nbVoices, "nbVoices", &(this->parameterRequestingConfirmation));

    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ globalState->sequencerMode.pBPM, pPatternLength, pDummy, pDummy }, 0);
    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ pEngineType, pNbVoices, pDummy, pDummy }, 1);

    allSynthParameters[p]->addPage(vector<ParameterInfo *>{ pSeqAction, pSeqActionDest, pSeqActionOp, pDummy }, 2);

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
    ParameterInfo *pPatternVolume = new ParameterInfo("V" + String(p), 0, 1, globalState->engine->partVolumes[p], "partVolume" + String(p));
    ParameterInfo *pPlayingProb = new ParameterInfo("prb", 0, 1, globalState->playingProb[p], "playProb" + String(p));
    ParameterInfo *pMute = new ParameterInfoDiscrete("On ", 0, 1, globalState->sequencerMode.sequencerActive[p], "UnMuted" + String(p));
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
  armedForMuteToggle[selectedLane] = 0;  // the currently selected part is stored in the selectedLane field in this mode
  Mode::processPotValue(potIndex, potVal, updateDisplay);
}

bool MixMuteMode::pushButtonReleased(int buttonIndex) {

  Mode::pushButtonReleased(buttonIndex);

  if (buttonIndex >= NB_PARTS) return false;
  if (armedForMuteToggle[buttonIndex]) {
    int oldValue = globalState->sequencerMode.sequencerActive[buttonIndex]->getValueDiscrete();
    globalState->sequencerMode.sequencerActive[buttonIndex]->setValue(!oldValue);
    armedForMuteToggle[buttonIndex] = 0;

    int paramPos = 1;  // position of this parameter on display
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

  if (buttonIndex == MODE_BUTTON) {  // P
    globalState->pPressed = true;
    return 1;
  }

  if (buttonIndex == PART_BUTTON) {  // shift
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
    } else if (buttonIndex == 0) {  // state is already sequencer
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
    } else if (buttonIndex == 7) {
      globalState->selectedMode = &(globalState->keyboardMode);
      lcd->setCursor(0, 0);
      lcd->print("  KEYBOARD      ");
      //globalState->selectedMode->postPartOrModeSwitch();
    }

    this->globalState->delayedDisplayRefresh = millis() + 1000;

    return 2;
  }

  if ((globalState->shiftPressed) && (buttonIndex < NB_PARTS)) {  // part switch
    Serial.println("part switch");

    globalState->selectedPart = buttonIndex;  

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

  int partId = effectivePartId(globalState->selectedPart);  // TODO: this only makes sense for the synthMode !?
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
  int now = millis() / 500;  // in half seconds
  if (now != this->lastConfDisplayUpdate) {
    lcd->setCursor(4 * 3, 0);
    if (2 * (now / 2) == now) {
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

  if (buttonIndex == PART_BUTTON) {  // cancel
    handleCancelled();
    return true;
  } else if (buttonIndex == MODE_BUTTON) {  // confirm
    handleConfirmed();
    return true;
  }
  return false;
}



bool MixMuteMode::pushButtonPressed(int buttonIndex) {

  // when doing parameter switch, respect the fact that we have only one synthparameter here
  int eventConsumed = handleGenericPushButtonEvents(buttonIndex);
  if (eventConsumed) return true;

  if (buttonIndex >= NB_PARTS) return false;

  armedForMuteToggle[buttonIndex] = 1;

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
  if ((paramName == "PTCSave") || (paramName == "BNKSave")) {
    if (!ensureSaveDirectory()) {
      Serial.println("failed to create save dir");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
      return;
    }

    int effPartId = globalState->synthMode.effectivePartId(globalState->selectedPart);
    String filename = String(kCookieBoxSaveDir) + "/" + String(kPatchPrefix) + this->selectedBank->getValueDiscrete() + "_" + this->selectedPatch->getValueDiscrete() + String(kSaveExtension);
    Serial.println(filename);

    SD.remove(filename.c_str());  // don't append to existing file
    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if (dataFile) {
      writePatchParameterLines(dataFile, &globalState->synthMode, effPartId);
      dataFile.close();
      if (slotCacheInitialized) {
        int bank = this->selectedBank->getValueDiscrete();
        int patch = this->selectedPatch->getValueDiscrete();
        if (bank >= 0 && bank < 10 && patch >= 0 && patch < 10) {
          patchSlotCache[bank * 10 + patch] = true;
        }
      }
      Serial.println("wrote file");
      lcd->setCursor(4 * 3, 0);
      lcd->print("w-ok");
    } else {
      Serial.println("error opening file for write");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    }

    /* Legacy JSON patch save for reference:
    JsonDocument doc;
    JsonObject obj = doc["PartParameters"].to<JsonObject>();
    int effPartId = globalState->synthMode.effectivePartId(globalState->selectedPart);
    globalState->synthMode.serializePart(&obj, effPartId);
    char output[2560];
    int nbBytes = serializeJson(doc, output);
    String filename = String("Patch_") + this->selectedBank->getValueDiscrete() + "_" + this->selectedPatch->getValueDiscrete() + ".json";
    SD.remove(filename.c_str());
    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if (dataFile) {
      dataFile.println(output);
      dataFile.close();
    }
    */
  }

  if ((paramName == "PTCLoad") || (paramName == "BNKLoad")) {
    String filename = String(kCookieBoxSaveDir) + "/" + String(kPatchPrefix) + this->selectedBank->getValueDiscrete() + "_" + this->selectedPatch->getValueDiscrete() + String(kSaveExtension);
    Serial.println(filename);

    File dataFile = SD.open(filename.c_str());
    if (dataFile) {
      Serial.println("reading patch file:");
      loadPatchNewFormat(dataFile, globalState, globalState->selectedPart);
      dataFile.close();
      lcd->setCursor(4 * 3, 0);
      lcd->print("l-ok");
    } else {
      Serial.println("error opening - patch does not exist");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    }

    /* Legacy JSON patch load for reference:
    JsonDocument doc;
    String filename = String("Patch_") + this->selectedBank->getValueDiscrete() + "_" + this->selectedPatch->getValueDiscrete() + ".json";
    File dataFile = SD.open(filename.c_str());
    if (dataFile) {
      deserializeJson(doc, dataFile);
      JsonObject obj = doc["PartParameters"];
      int effPartId = globalState->synthMode.effectivePartId(globalState->selectedPart);
      globalState->synthMode.deserializePart(&obj, effPartId);
      dataFile.close();
    }
    */
  }

  if ((paramName == "GlobalBNKSave") || (paramName == "GlobalPRGSave")) {
    Serial.println("save global prg");

    if (!ensureSaveDirectory()) {
      Serial.println("failed to create save dir");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
      return;
    }

    String filename = String(kCookieBoxSaveDir) + "/" + String(kProgramPrefix) + this->selectedBank->getValueDiscrete() + "_" + this->selectedPatch->getValueDiscrete() + String(kSaveExtension);
    Serial.println(filename);

    SD.remove(filename.c_str());  // don't append to existing file
    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if (dataFile) {
      writeProgramFile(dataFile, globalState);
      dataFile.close();
      if (slotCacheInitialized) {
        int bank = this->selectedBank->getValueDiscrete();
        int patch = this->selectedPatch->getValueDiscrete();
        if (bank >= 0 && bank < 10 && patch >= 0 && patch < 10) {
          programSlotCache[bank * 10 + patch] = true;
        }
      }
      Serial.println("wrote file");
      lcd->setCursor(4 * 3, 0);
      lcd->print("w-ok");
    } else {
      Serial.println("error opening file for write");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    }

    /* Legacy JSON program save for reference:
    JsonDocument doc;
    JsonObject obj = doc["Program"].to<JsonObject>();
    globalState->serializeProgram(&obj);
    char output[100000];
    int nbBytes = serializeJson(doc, output);
    String filename = String("Program_") + this->selectedBank->getValueDiscrete() + "_" + this->selectedPatch->getValueDiscrete() + ".json";
    SD.remove(filename.c_str());
    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if (dataFile) {
      dataFile.println(output);
      dataFile.close();
    }
    */
  }

  if ((paramName == "GlobalBNKLoad") || (paramName == "GlobalPRGLoad")) {
    Serial.println("load global prg");
    String filename = String(kCookieBoxSaveDir) + "/" + String(kProgramPrefix) + this->selectedBank->getValueDiscrete() + "_" + this->selectedPatch->getValueDiscrete() + String(kSaveExtension);
    Serial.println(filename);

    Serial.print("RAM before:");
    Serial.println(freeram());

    File dataFile = SD.open(filename.c_str());
    if (dataFile) {
      Serial.println("reading program file:");
      loadProgramNewFormat(dataFile, globalState);
      dataFile.close();

      Serial.print("RAM after:");
      Serial.println(freeram());

      lcd->setCursor(4 * 3, 0);
      lcd->print("l-ok");
    } else {
      Serial.println("error opening - program does not exist");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    }

    /* Legacy JSON program load for reference:
    JsonDocument doc;
    String filename = String("Program_") + this->selectedBank->getValueDiscrete() + "_" + this->selectedPatch->getValueDiscrete() + ".json";
    File dataFile = SD.open(filename.c_str());
    if (dataFile) {
      deserializeJson(doc, dataFile);
      JsonObject obj = doc["Program"];
      globalState->deserializeProgram(&obj);
      dataFile.close();
    }
    */
  }

  if ((paramName == "nbVoices") || (paramName == "engineType")) {
    resetEngineTypeAndVoices();
    lcd->setCursor(4 * 3, 0);
    lcd->print("done");
  }

  if ((paramName == "actDst") || (paramName == "actOp")) {
    bool success = handleSeqAct();
    if (!success) {
      Serial.println("error executing action");
      lcd->setCursor(4 * 3, 0);
      lcd->print("err ");
    } else {
      lcd->setCursor(4 * 3, 0);
      lcd->print("done");
    }
  }

}

bool PartConfigMode::handleSeqAct() {

  // handle copying
  // copying pattern or sequence is the same, only at paste time we distinguish
  if (this->seqActionOp->getValueDiscrete() == 1) {// copy
    this->clipBoardPattern=globalState->selectedPattern;
    this->clipBoardPart=globalState->selectedPart; // 
    return true;
  }

  // which parts are we processing
  int minPart=0;
  int maxPart=NB_PARTS-1;
  if(seqActionDest->getValueDiscrete() == 0) { // only current sequence, i.e. one part of the pattern
    minPart = globalState->selectedPart;
    maxPart = minPart;
  }

  // process parts
  for (int part = minPart; part<=maxPart; part++) {
    Sequence *s = this->globalState->patterns[globalState->selectedPattern][part];

    if (this->seqActionOp->getValueDiscrete() == 0) { // clr
      Sequence::clearSequence(s);
    }

    if (this->seqActionOp->getValueDiscrete() == 2) { // paste
      if ((clipBoardPart < 0) || (clipBoardPattern < 0)) return false;
      int partToCopyFrom = (seqActionDest->getValueDiscrete() == 0) ? clipBoardPart : part;
      Sequence *source = this->globalState->patterns[clipBoardPattern][partToCopyFrom];
      Sequence::copySequence(s, source);
    }
    
  }

  return true;
}

void PartConfigMode::resetEngineTypeAndVoices() {
  Serial.println("resetting all engine Types and nbVoices");

  // first disable all voices
  for (int p = 0; p < NB_PARTS; p++) {
    int partId = globalState->synthMode.effectivePartId(p);
    Serial.print("effectivePartId ");
    Serial.println(partId);
    globalState->engine->getPart(partId)->setActiveNbVoices(0);
  }

  globalState->engine->markRequiredSignals();

  // set all synth engines
  for (int p = 0; p < NB_PARTS; p++) {
    partTypes[p] = engineTypeParams[p]->getValueDiscrete();
  }

  // set the voices
  // total count can not be larger than 6
  // engine type 1 can only have 1 voice
  int maxVoiceAssign = 6;
  int nbVoicesAssigned = 0;
  for (int p = 0; p < NB_PARTS; p++) {
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

    if (nbVoicesAssigned + requestedVoices <= maxVoiceAssign) {  // can give it all the requested voices
      globalState->engine->getPart(partId)->setActiveNbVoices(requestedVoices);
      nbVoicesAssigned += requestedVoices;
      //Serial.println(requestedVoices);
    } else {  // give as many voices as we have left
      globalState->engine->getPart(partId)->setActiveNbVoices(maxVoiceAssign - nbVoicesAssigned);
      nbVoicesParams[p]->setValue(maxVoiceAssign - nbVoicesAssigned);  // reflect in parameter what is the actual voice number
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

  if ((buttonIndex == 7) && (!globalState->shiftPressed) && (!globalState->pPressed)) {
    globalState->myNoteOn(globalState->selectedPart + 1, 36, 127);
  }

  return true;
}

bool Mode::pushButtonReleased(int buttonIndex) {
  if (buttonIndex == MODE_BUTTON) {  // P
    globalState->pPressed = false;
    return false;
  }

  if (buttonIndex == PART_BUTTON) {  // shift
    globalState->shiftPressed = false;
    return false;
  }

  return false;
}

bool SynthMode::pushButtonReleased(int buttonIndex) {
  Mode::pushButtonReleased(buttonIndex);
  if (buttonIndex == 7) {
    globalState->myNoteOff(globalState->selectedPart + 1, 36, 127);
  }
  return false;
}

void SynthMode::postPartOrModeSwitch() {
  int partId = effectivePartId(globalState->selectedPart);
  this->currentMenuPage = allSynthParameters[partId]->getPage(selectedLane, selectedPage);
}

void PartConfigMode::postPartOrModeSwitch() {
  int partId = globalState->selectedPart;
  this->currentMenuPage = allSynthParameters[partId]->getPage(selectedLane, selectedPage);
}

void PartConfigMode::fullDisplayUpdate() {
  Mode::fullDisplayUpdate();

  this->updateFilExistenceFeedback();
}

void PartConfigMode::processPotValue(int potIndex, int potVal, bool updateDisplay) {
  Mode::processPotValue(potIndex, potVal, updateDisplay);

  this->updateFilExistenceFeedback();
}

void PartConfigMode::updateFilExistenceFeedback() {

  if ((selectedLane != 4) && (selectedLane != 5)) return;
  if (!currentMenuPage) return;
  if (currentMenuPage->size() < 3) return;

  if (!slotCacheInitialized) refreshSaveSlotCache();

  String actionName = currentMenuPage->at(0)->getName();
  bool isGlobal = (actionName == "GLD") || (actionName == "GSV");
  int bank = selectedBank->getValueDiscrete();
  int patch = selectedPatch->getValueDiscrete();
  bool slotTaken = isGlobal ? programSlotCached(bank, patch) : patchSlotCached(bank, patch);

  char buffer[] = "____";
  currentMenuPage->at(2)->renderPrintableValue(buffer);
  buffer[3] = slotTaken ? '*' : '.';
  lcd->setCursor(4 * 2, 1);
  lcd->print(buffer);
}

void MixMuteMode::postPartOrModeSwitch() {
  int partId = globalState->selectedPart;
  this->currentMenuPage = allSynthParameters[0]->getPage(partId, 0);
}


void SequencerMode::setup() {
  for (int i = 0; i < globalState->engine->getNbParts(); i++) {
    lastPlayedNote.push_back(-1);
    remainingNoteDuration.push_back(0);
    sequencerActive[i] = new StaticSignalDiscrete(NULL, 1);
  }

  page = new StaticSignalDiscrete(NULL, 0);
  ParameterInfoDiscrete *pPage = new ParameterInfoDiscrete("PAG ", 0, 7, page, "SequencerPage");
  selectedPattern = new StaticSignalDiscrete(NULL, 0);
  ParameterInfoDiscrete *pSelectedPattern = new ParameterInfoDiscrete("PAT ", 0, NB_PATTERNS-1, selectedPattern, "SelectedPattern");
  sequencerPlaying = new StaticSignalDiscrete(NULL, 0);
  ParameterInfoDiscrete *pSequencerPlaying = new ParameterInfoDiscrete("PLY ", 0, 1, sequencerPlaying, "SequencerPlaying");

  ParameterInfo *pDummy = new DummyParameterInfo("  ");
  this->doubleShiftParameters.push_back(pPage);
  this->doubleShiftParameters.push_back(pDummy);
  this->doubleShiftParameters.push_back(pSelectedPattern);
  this->doubleShiftParameters.push_back(pSequencerPlaying);
}

void SequencerMode::processPotValue(int potIndex, int potVal, bool updateDisplay) {

  if (this->paramLockMode) return processLockParameter(potIndex, potVal);
  if (this->doubleShiftMode) return processDoubleShiftParameter(potIndex, potVal);


  Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
  int32_t noteIndex = Sequence::ensureNoteAtStep(s, cursorPos);

  if (noteIndex != -1) {
    if (potIndex == 0) {  // octave
      int oct = std::lround(1 + (potVal / 1024.0) * 4) - 2;
      globalNotes[noteIndex].octave = (int8_t)oct;
      displayOctAndNote();
    }
    if (potIndex == 1) {  // note
      int note = std::lround((potVal / 1024.0) * 12);
      globalNotes[noteIndex].note = (int8_t)note;
      displayOctAndNote();
    }
    if (potIndex == 2) {  // length
      int length = (potVal < 300) ? std::lround(8 * (potVal / 300.0)) : 8 + std::lround((s->NB_STEPS - 8) * ((potVal - 300) / (1024.0 - 300.0)));
      globalNotes[noteIndex].length = (int8_t)length;
      displayOctAndNote();
    }
  }

}

void SequencerModeGraphic::processPotValue(int potIndex, int potVal, bool updateDisplay) {
  seqMode->processPotValue(potIndex, potVal, updateDisplay);
  this->armedForToggle = -1;  // when pLocks were modified, don't toggle step on release
}

void SequencerMode::processLockParameter(int potIndex, int potVal) {
  // potVal is 0-3, referring to selected parameter or last used parameters
  // initial implementatino, just take the parameters that are currently active in synth mode
  int effectivePartId = this->effectivePartId(globalState->selectedPart);

  std::vector<ParameterInfo *> *pis = this->globalState->synthMode.allSynthParameters[effectivePartId]->getPage(this->globalState->synthMode.selectedLane, this->globalState->synthMode.selectedPage);
  ParameterInfo *lockParam = (*pis)[potIndex];

  // display locked value
  lcd->setCursor(4 * potIndex, 1);
  char buffer[] = "    ";
  lockParam->renderPrintableValueFromPotValue(buffer, potVal);
  lcd->print(buffer);

  int selectedPart = this->globalState->selectedPart;
  Sequence *s = this->globalState->patterns[globalState->selectedPattern][selectedPart];
  int32_t noteIndex = Sequence::ensureNoteAtStep(s, cursorPos);
  if (noteIndex == -1) {
    Serial.println("failed to allocate note for lock");
    return;
  }

  if (!Sequence::addOrUpdateNoteLock(noteIndex, lockParam, potVal)) {
    Serial.println("all locking slots occupied");
  } else {
    Serial.print("locking parameter ");
    Serial.print(lockParam->getName());
    Serial.print(" on note index ");
    Serial.println(noteIndex);
  }
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

  // if in plock mode, pressing 'no'/6/part button deletes all plocks of the current step
  if (this->paramLockMode == 1) {
    if (buttonIndex == MODE_BUTTON) {
      Serial.println("deleting plocks");
      Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
      int32_t noteIndex = s->steps[cursorPos];
      if (noteIndex != -1) {
        Sequence::clearNoteLocks(noteIndex);
      }
    }
  }

  int consumed = handleGenericPushButtonEvents(buttonIndex);
  if (consumed == 2) return true;

  if (consumed == 1) {                                         // one of the shift buttons was pressed
    if (globalState->shiftPressed && globalState->pPressed) {  // now both shift buttons are pressed
      this->doubleShiftMode = true;
      // put the actual values into the parameters
      int page = cursorPos / 8;
      this->page->setValue(page);
      this->sequencerPlaying->setValue(globalState->seqPlaying ? 1 : 0);
      this->selectedPattern->setValue(globalState->selectedPattern);
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
    Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
    int32_t noteIndex = s->steps[cursorPos];
    int currentValue = 0;
    if (noteIndex != -1) currentValue = globalNotes[noteIndex].on;
    int newValue = 1 - currentValue;

    if (noteIndex == -1 && newValue == 1) {
      noteIndex = Sequence::ensureNoteAtStep(s, cursorPos);
    }
    if (noteIndex != -1) {
      globalNotes[noteIndex].on = (int8_t)newValue;
    }

    Serial.print("new value at position:");
    Serial.print(cursorPos);
    Serial.print(":");
    Serial.println(newValue);
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

  if (buttonIndex == 7) {  // play/stop
    this->globalState->seqPlaying = !this->globalState->seqPlaying;
    if (this->globalState->seqPlaying) {
      this->playHead = 0;
      this->nextTriggerTime = millis();  // 0; // means: in the next call a step 0 is going to be played
    } else {                             // switch off current note
      // of all parts
      cleanUpAfterStop();
    }

    displayPlayStatus();
  }

  return true;  // TODO: do we really need to lock pots after all buttons here?
}

void SequencerMode::cleanUpAfterStop() {
  for (int i = 0; i < globalState->engine->getNbParts(); i++) {
        globalState->myNoteOff(i + 1, lastPlayedNote[i], 0);  // TODO: mapping from seq part to midi channel
        for (int lockPosition = 0; lockPosition < 10; lockPosition++) {
          if (this->parametersToReset[i * 10 + lockPosition] != NULL) {
            this->parametersToReset[i * 10 + lockPosition]->unlock();
            this->parametersToReset[i * 10 + lockPosition] = NULL;
          }
        }
    }
}

bool SequencerModeGraphic::pushButtonPressed(int buttonIndex) {

  int consumed = handleGenericPushButtonEvents(buttonIndex);
  if (consumed == 2) return true;

  if (consumed == 1) {                                         // one of the shift buttons was pressed
    if (globalState->shiftPressed && globalState->pPressed) {  // now both shift buttons are pressed
      seqMode->doubleShiftMode = true;
      seqMode->paramLockMode = false;

      // put the actual values into the parameters
      int page = seqMode->cursorPos / 8;
      seqMode->page->setValue(page);
      seqMode->sequencerPlaying->setValue(globalState->seqPlaying ? 1 : 0);
      seqMode->selectedPattern->setValue(globalState->selectedPattern);

      seqMode->displayDoubleShiftMode();
      return true;
    }
  }

  // from here on only process the 8 step buttons
  if (buttonIndex >= 8) return false;

  // every step button press goes into pLock mode
  seqMode->paramLockMode = 1;
  seqMode->displayLockingParams();

  // arm to toggle this step on button release
  armedForToggle = buttonIndex;

  int page = seqMode->cursorPos / 8;

  // move cursor to step
  seqMode->cursorPos = page * 8 + buttonIndex;

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
    if ((buttonIndex == MODE_BUTTON) || (buttonIndex == PART_BUTTON)) {  // we are no longer in double shift mode
      doubleShiftMode = false;
      int stepInPage = cursorPos % 8;
      cursorPos = page->getValueDiscrete() * 8 + stepInPage;
      if(!globalState->seqPlaying && sequencerPlaying->getValueDiscrete() == 1) {
        this->nextTriggerTime = millis(); // avoid that all missed steps are played now
      }
      globalState->seqPlaying = sequencerPlaying->getValueDiscrete() == 1;
      globalState->selectedPattern = selectedPattern->getValueDiscrete();

      fullDisplayUpdate();
      needToLock = true;
    }
  }
  return needToLock;
}

bool SequencerModeGraphic::pushButtonReleased(int buttonIndex) {

  bool needToLock = Mode::pushButtonReleased(buttonIndex);

  if (seqMode->doubleShiftMode) {
    if ((buttonIndex == PART_BUTTON) || (buttonIndex == MODE_BUTTON)) {  // we are no longer in double shift mode
      seqMode->doubleShiftMode = false;
      seqMode->paramLockMode = false;
      armedForToggle = -1;
      int stepInPage = seqMode->cursorPos % 8;
      seqMode->cursorPos = seqMode->page->getValueDiscrete() * 8 + stepInPage;
      if(!globalState->seqPlaying && seqMode->sequencerPlaying->getValueDiscrete() == 1) {
        seqMode->nextTriggerTime = millis(); // avoid that all missed steps are played now
      }
      globalState->seqPlaying = seqMode->sequencerPlaying->getValueDiscrete() == 1;
      globalState->selectedPattern = seqMode->selectedPattern->getValueDiscrete();

      fullDisplayUpdate();
      needToLock = true;
    }
  }


  if (armedForToggle == buttonIndex) {
    Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
    int32_t noteIndex = s->steps[seqMode->cursorPos];
    int currentValue = 0;
    if (noteIndex != -1) currentValue = globalNotes[noteIndex].on;
    int newValue = 1 - currentValue;

    if (noteIndex == -1 && newValue == 1) {
      noteIndex = Sequence::ensureNoteAtStep(s, seqMode->cursorPos);
    }
    if (noteIndex != -1) {
      globalNotes[noteIndex].on = (int8_t)newValue;
    }

    Serial.print("new value at position:");
    Serial.print(seqMode->cursorPos);
    Serial.print(":");
    Serial.println(newValue);
  }

  //seqMode->displayStep();
  //seqMode->displayOctAndNote();
  if (seqMode->paramLockMode) {
    needToLock = true;  // returning from pLock mode
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

  for (unsigned int p = 0; p < 4; p++) {
    int effectivePartId = this->effectivePartId(globalState->selectedPart);
    //ParameterInfo *pinfo = this->globalState->partConfigMode.lockParameters[partId * 4 + p];
    std::vector<ParameterInfo *> *pis = this->globalState->synthMode.allSynthParameters[effectivePartId]->getPage(this->globalState->synthMode.selectedLane, this->globalState->synthMode.selectedPage);
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
  for (int step = 0; step < 8; step++) {
    String thisChar = (page * 8 + step == cursorPos) ? String("|") : String(" ");
    stepVisu = stepVisu + thisChar;
  }
  lcd->setCursor(8, 1);
  lcd->print(stepVisu);
}

void SequencerMode::displayOctAndNote() {

  Sequence *s = this->globalState->patterns[globalState->selectedPattern][globalState->selectedPart];
  int32_t noteIndex = s->steps[cursorPos];
  int8_t octave = 0;
  int8_t note = 0;
  int8_t length = 0;
  int8_t on = 0;
  if (noteIndex != -1) {
    octave = globalNotes[noteIndex].octave;
    note = globalNotes[noteIndex].note;
    length = globalNotes[noteIndex].length;
    on = globalNotes[noteIndex].on;
  }

  lcd->setCursor(0, 1);
  lcd->print(String("O") + String(octave) + String("N") + String(note) + String("L") + String(length) + String(" "));
  lcd->setCursor(7, 1);

  String stepVisu = String("");
  int page = cursorPos / 8;
  for (int step = 0; step < 8; step++) {
    int32_t stepIndex = s->steps[page * 8 + step];
    String thisChar = (stepIndex != -1 && globalNotes[stepIndex].on) ? "+" : "_";
    stepVisu = stepVisu + thisChar;
  }
  lcd->setCursor(8, 0);
  lcd->print(stepVisu);
}

void SequencerMode::displayPlayStatus() {
  // play status
  lcd->setCursor(6, 0);
  lcd->print(globalState->seqPlaying ? "P" : "-");
  //lcd->setCursor(14, 1);
  //lcd->print("__");
  //lcd->setCursor(14, 1);
  //lcd->print(this->playHead % nbSteps);
}


void SequencerMode::maybePlay() {
  if (millis() > nextTriggerTime) {
    nextTriggerTime = nextTriggerTime + interBeatMs;
    play();
    // recalculate tempo: TODO, can we do that more rarely?
    interBeatMs = 60000 / (bpm->value * 4);
  }
}

void SequencerMode::play() {

    Serial.print("playing step ");
    Serial.println(playHead);

    for (int part = 0; part < NB_PARTS; part++) {  // TODO: play all parts

      if (sequencerActive[part]->getValueDiscrete() == 0) {
        if (lastPlayedNote[part] > -1) globalState->myNoteOff(part + 1, lastPlayedNote[part], 0);
        lastPlayedNote[part] = -1;
        continue;
      }

      int wrappedPlayHead = playHead % patternLengths[part]->getValueDiscrete();

      // check if a note needs to be stopped due to note length
      remainingNoteDuration[part]--;
      if (remainingNoteDuration[part] == 0) { // a note just ended
        if (lastPlayedNote[part] > -1) globalState->myNoteOff(part + 1, lastPlayedNote[part], 0);
        lastPlayedNote[part] = -1;
      }

      Sequence *s = this->globalState->patterns[globalState->selectedPattern][part];
      int32_t noteIndex = s->steps[wrappedPlayHead];
      if (noteIndex != -1 && globalNotes[noteIndex].on) { // we have a new note to play
 
        // stop previous note if it is still playing
        if (lastPlayedNote[part] > -1) globalState->myNoteOff(part + 1, lastPlayedNote[part], 0);
        int newNote = 36 + globalNotes[noteIndex].octave * 12 + globalNotes[noteIndex].note;
        lastPlayedNote[part] = newNote;
        remainingNoteDuration[part] = globalNotes[noteIndex].length;

        // clear previously applied locks for this part
        for (int lockPosition = 0; lockPosition < 10; lockPosition++) {
          if (this->parametersToReset[part * 10 + lockPosition] != NULL) {
            this->parametersToReset[part * 10 + lockPosition]->unlock();
            this->parametersToReset[part * 10 + lockPosition] = NULL;
          }
        }

        int32_t lockNode = globalNotes[noteIndex].lockHead;
        int lockPosition = 0;
        while (lockNode != -1 && lockPosition < 10) {
          ParameterInfo* param = globalLockNodes[lockNode].param;
          float value = globalLockNodes[lockNode].value;
          if (param != NULL) {
            param->lock(value);
            this->parametersToReset[part * 10 + lockPosition] = param;
          }
          lockPosition++;
          lockNode = globalLockNodes[lockNode].next;
        }

        // finally play new note
        globalState->myNoteOn(part + 1, newNote, 127);
        Serial.println(newNote);
      }
    }

    if ((playHead % 4 == 0) && (this->globalState->selectedMode == this)) {
      displayPlayStatus();
    }
    playHead = (playHead + 1);  // % nbSteps;

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

void Mode::deserializePart(JsonObject *jsonObject, int partId) {


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
        if ((*jsonObject)[pinfo->getUniqueName()].is<float>()) {
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

ParameterInfo *Mode::getParameterByNameAndPart(String uniqueName, int partId) {

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

  for (int i = 0; i < NB_PARTS; i++) {
    int effPartId = synthMode.effectivePartId(i);
    JsonObject partParameters = patchList.add<JsonObject>();
    synthMode.serializePart(&partParameters, effPartId);
  }

  // *** partConfig
  JsonArray configList = (*prg)["PartConfig"].to<JsonArray>();

  for (int i = 0; i < NB_PARTS; i++) {
    JsonObject partConfigParameters = configList.add<JsonObject>();
    partConfigMode.serializePart(&partConfigParameters, i);
  }

  // *** sequencer
  JsonArray seqdata = (*prg)["SequencerData"].to<JsonArray>();

  for (int i = 0; i < NB_PARTS; i++) {
    JsonObject partSeqData = seqdata.add<JsonObject>();
    sequencerMode.serializeSequencerData(&partSeqData, i);
  }
}

void GlobalState::deserializeProgram(JsonObject *prg) {

  // *** partConfig
  JsonArray configList = (*prg)["PartConfig"];  //.to<JsonArray>();

  for (int i = 0; i < NB_PARTS; i++) {
    JsonObject partConfigParameters = configList[i];
    partConfigMode.deserializePart(&partConfigParameters, i);
  }

  // based on part config, select the correct sound engines and voice numbers
  // needs to happen before loading synth params
  this->partConfigMode.resetEngineTypeAndVoices();

  // *** synth parameters
  JsonArray patchList = (*prg)["PartParameters"];  //.to<JsonArray>();

  for (int i = 0; i < NB_PARTS; i++) {
    int effPartId = synthMode.effectivePartId(i);

    JsonObject partParameters = patchList[i];  //.to<JsonObject>(); // this breaks it
    synthMode.deserializePart(&partParameters, effPartId);
  }

  // *** sequencer
  JsonArray seqdata = (*prg)["SequencerData"];

  for (int i = 0; i < NB_PARTS; i++) {
    JsonObject partSeqData = seqdata[i];
    sequencerMode.deserializeSequencerData(&partSeqData, i);
  }
}

void SequencerMode::serializeSequencerData(JsonObject *seqData, int partId) {
  Serial.println("serializeSequencerData");

  JsonArray patterns = (*seqData)["Patterns"].to<JsonArray>();

  for (int patternId = 0; patternId < NB_PATTERNS; patternId++) {
    Sequence *s = this->globalState->patterns[patternId][partId];
    JsonArray pattern = patterns.add<JsonArray>();
    for (int step = 0; step < s->NB_STEPS; step++) {
      JsonObject stepData = pattern.add<JsonObject>();
      int32_t noteIndex = s->steps[step];
      if (noteIndex == -1) {
        stepData["o"] = -1;
        stepData["n"] = 0;
        stepData["o/o"] = 0;
        stepData["l"] = 0;
      } else {
        const NoteEntry &note = globalNotes[noteIndex];
        stepData["o"] = note.octave;
        stepData["n"] = note.note;
        stepData["o/o"] = note.on;
        stepData["l"] = note.length;

        JsonArray lockedParams = stepData["pLocks"].to<JsonArray>();
        int32_t lockNode = note.lockHead;
        while (lockNode != -1) {
          JsonObject onePlock = lockedParams.add<JsonObject>();
          onePlock["name"] = globalLockNodes[lockNode].param->getUniqueName();
          onePlock["value"] = globalLockNodes[lockNode].value;
          lockNode = globalLockNodes[lockNode].next;
        }
      }
    }
  }
}

void SequencerMode::deserializeSequencerData(JsonObject *seqData, int partId) {
  Serial.println("deserializeSequencerData");

  JsonArray patterns = (*seqData)["Patterns"];

  for (int patternId = 0; ((patternId < patterns.size())&&(patternId < NB_PATTERNS)); patternId++) {
    Sequence *s = this->globalState->patterns[patternId][partId];
    JsonArray pattern = patterns[patternId];
    for (int step = 0; step < pattern.size(); step++) {
      JsonObject stepData = pattern[step];
      bool active = true;
      int8_t octave = 0;
      int8_t noteValue = 0;
      int8_t onValue = 1;
      int8_t length = 1;

      if (stepData.containsKey("octave")) {
        active = stepData["on/off"];
        octave = stepData["octave"];
        noteValue = stepData["note"];
        onValue = stepData.containsKey("on/off") ? stepData["on/off"] : 1;
        length = stepData.containsKey("l") ? stepData["l"] : 1;
      } else {
        octave = stepData["o"];
        noteValue = stepData["n"];
        onValue = stepData["o/o"];
        length = stepData["l"];
        active = octave >= 0;
      }

      int32_t existingNoteIndex = s->steps[step];
      if (existingNoteIndex != -1) {
        Sequence::clearNoteLocks(existingNoteIndex);
        Sequence::freeGlobalNoteIndex(existingNoteIndex);
        s->steps[step] = -1;
      }

      if (!active) {
        s->steps[step] = -1;
        continue;
      }

      NoteEntry newNote = {octave, noteValue, onValue, length, -1};
      int32_t noteIndex = Sequence::allocateGlobalNote(newNote);
      if (noteIndex == -1) {
        s->steps[step] = -1;
        continue;
      }
      s->steps[step] = noteIndex;

      if (stepData["pLocks"].is<JsonArray>()) {
        JsonArray pLocks = stepData["pLocks"];
        for (int pIndex = 0; pIndex < pLocks.size(); pIndex++) {
          String uniqueName = pLocks[pIndex]["name"];
          float value = pLocks[pIndex]["value"];
          int synthPartId = this->effectivePartId(partId);
          ParameterInfo *paramToLock = globalState->synthMode.getParameterByNameAndPart(uniqueName, synthPartId);
          if (paramToLock != NULL) {
            Sequence::addOrUpdateNoteLock(noteIndex, paramToLock, value);
          }
        }
      }
    }
  }
}



void KeyboardMode::setup() {

  octave = new StaticSignalDiscrete(NULL, 0);
  key = new StaticSignalDiscrete(NULL, 0);
  centerNote = new StaticSignalDiscrete(NULL, 0);
  mode = new StaticSignalDiscrete(NULL, 0);

  // ParameterInfoDiscrete(String name, float min, float max, StaticSignal* param, String uniqueName, const std::vector<String>& strings=std::vector<String>() )

  ParameterInfoDiscrete *pOctave = new ParameterInfoDiscrete("OCT", 0, 4, octave, "ko");

  // Key means Tonart here, not the keyboard key
  std::vector<String> keys = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};
  ParameterInfoDiscrete *pKey = new ParameterInfoDiscrete("KEY", 0, 11, key, "key", keys);
  ParameterInfoDiscrete *pCenterNote = new ParameterInfoDiscrete("CTR", 0, 7, centerNote, "ck");

  std::vector<String> modes = {"ply", "rec", "dub"};
  ParameterInfoDiscrete *pMode = new ParameterInfoDiscrete("MOD", 0, 3, mode, "md", modes);

  allSynthParameters.push_back(new SynthParameters());
  allSynthParameters[0]->addPage(vector<ParameterInfo *>{ pOctave, pKey, pCenterNote, pMode}, 0);
  this->currentMenuPage = allSynthParameters[0]->getPage(0, 0);

}

bool KeyboardMode::pushButtonPressed(int buttonIndex) {

  Serial.print("keyboard button pressed");

  // when doing parameter switch, respect the fact that we have only one synthparameter here
  int eventConsumed = handleGenericPushButtonEvents(buttonIndex);
  if (eventConsumed) return true;

  if ((globalState->pPressed) || (globalState->shiftPressed)) return false;

  // in every mode: play the note
  int part = globalState->selectedPart;
  // in seq play it looks like that: int newNote = 36 + s->data[0][wrappedPlayHead] * 12 + s->data[1][wrappedPlayHead];
  int noteIndexInScale = (buttonIndex + centerNote->getValueDiscrete()) % 7;
  int additionalOctave = 12 * ((buttonIndex + centerNote->getValueDiscrete()) / 7);
  int noteToPlay = 36 + octave->getValueDiscrete() * 12 + key->getValueDiscrete() + this->halfNotesIntervalsMajor[noteIndexInScale] + additionalOctave;
  this->playingNotesPerKey[buttonIndex] = noteToPlay;

  // todo: if the sequencer is currently playing/holding a note, stop the note so we can hear the button we are pressing (assuming monophonic synth)

  globalState->myNoteOn(part+1, noteToPlay, 127); // TODO: mapping from seq-track to midi channel

  // depending on the rec mode: record the note

  // sequencer playing and REC: 
  //     record the note in the closest position of the sequencer
  //     TODO: if it is the first note, delete the entire sequence
  //     (remember where in the sequence the recording started)
  //     (if we did record one entire sequence, stop recording)
  // sequencer playing and DUB:
  //     the same but don't  delete the entire sequence in the beginning
  // sequencer not playing, and REC or DUB
  //     record the note in the current position of the cursor
  //     when releasing the button, advance the sequencer by one


  if (mode->getValueDiscrete() > 0) {
    // playhead is always pointing to the next note to be played
    Sequence *s = this->globalState->patterns[globalState->selectedPattern][part];

    int positionToRecord = globalState->sequencerMode.cursorPos; // that is the position in case the seq is not playing


    if (globalState->seqPlaying) {// TODO, also when synced to clock we are in this mode
      int wrappedPlayHead = globalState->sequencerMode.playHead % globalState->sequencerMode.patternLengths[part]->getValueDiscrete();
      positionToRecord = wrappedPlayHead;
      int currentTime = millis();
      int timeToNextTrig = globalState->sequencerMode.nextTriggerTime - currentTime;

      if (timeToNextTrig > 0.5 * globalState->sequencerMode.interBeatMs) {
        positionToRecord = positionToRecord - 1;
      }
    }

    int32_t noteIndex = Sequence::ensureNoteAtStep(s, positionToRecord);
    if (noteIndex != -1) {
      globalNotes[noteIndex].on = 1;
      globalNotes[noteIndex].octave = (int8_t)((octave->getValueDiscrete() + additionalOctave) / 12);
      globalNotes[noteIndex].note = (int8_t)(key->getValueDiscrete() + this->halfNotesIntervalsMajor[noteIndexInScale]);
      Serial.print("Recorded at position ");
      Serial.println(positionToRecord);
    }

  }

  return false;
}

bool KeyboardMode::pushButtonReleased(int buttonIndex) {

  bool consumed = Mode::pushButtonReleased(buttonIndex);
  if (consumed) return false;

  if (buttonIndex > 7) return false;

  if (playingNotesPerKey[buttonIndex] >=0) {
    globalState->myNoteOff(globalState->selectedPart+1, playingNotesPerKey[buttonIndex], 0);
  }

  if (mode->getValueDiscrete() > 0) {
    if (!globalState->seqPlaying) { // go to next step
      int part = globalState->selectedPart;
      globalState->sequencerMode.cursorPos = (globalState->sequencerMode.cursorPos + 1) % globalState->sequencerMode.patternLengths[part]->getValueDiscrete();
    } else {
      // TODO: store note length
    }
  }

  

  return true;
}
