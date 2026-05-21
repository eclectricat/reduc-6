#include "Menu.h"
#include <array>
#include <cstdint>

class Mode;
class LiquidCrystal;
class SynthEngine;
class GlobalState;
class Sequence;
//class Menu2;

// TODO; put this to some truly global location
#define NB_PARTS 6 
#define NB_PATTERNS 10

#define MODE_BUTTON 9
#define PART_BUTTON 8

// ============================================
// Global note & parameter-lock pools (static, deterministic)
// 
// POOL DESIGN: Two-tier memory management with no malloc/free.
// - globalNotes[0..globalNextNoteIndex-1]: allocated notes (active or recycled via free-list)
// - globalNotes[globalNextNoteIndex..MAX-1]: unused/uninitialized slots
// - globalFreeNoteHead: linked-list head of recycled notes ready for reuse
// 
// When a note is freed, it's prepended to the free-list (O(1)).
// When allocating, we first pop from the free-list, then append fresh from unused portion.
// The lockHead field is overloaded: it points to locks when active, or to next free node when recycled.
// ============================================
struct NoteEntry {
  int8_t octave;
  int8_t note;
  int8_t on;       // 0 = off, 1 = on
  int8_t length;
  // DUAL MEANING: When note is ACTIVE (in use): lockHead points to first lock node in chain.
  // When note is FREED (in free-list): lockHead is reused as the "next" pointer in the free-list.
  // This saves 4 bytes per entry (3KB total) at the cost of semantic overlapping.
  int32_t lockHead; // active: index into globalLockNodes, -1 = none | freed: next free note, -1 = end of list
};

struct LockNode {
  ParameterInfo* param;
  float value;
  int32_t next;
};

constexpr int MAX_GLOBAL_NOTES = 2048;
static NoteEntry globalNotes[MAX_GLOBAL_NOTES];
static int32_t globalNextNoteIndex = 0;  // points to first unused slot in the pool (append position)
static int32_t globalFreeNoteHead = -1;  // head of linked-list of freed/recycled notes

constexpr int MAX_GLOBAL_LOCK_NODES = 1024;
static LockNode globalLockNodes[MAX_GLOBAL_LOCK_NODES];
static int32_t globalFreeLockNodeHead = -1;
static bool globalLockPoolInitialized = false;

/* Helper functions moved into class `Sequence` below. */


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
  ParameterInfo *parametersToReset[10 * NB_PARTS];
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

class KeyboardMode : public Mode {
  public:

  KeyboardMode(LiquidCrystal *lcd) {
    this->lcd = lcd;
  }

  //virtual void processPotValue(int potIndex, int potVal, bool updateDisplay) override;
  virtual bool pushButtonPressed(int buttonIndex) override;
  virtual bool pushButtonReleased(int buttonIndex) override;
  //virtual void fullDisplayUpdate() {};

  void setup();

  private:
  StaticSignalDiscrete *octave = NULL;
  StaticSignalDiscrete *key = NULL;
  StaticSignalDiscrete *centerNote = NULL;
  StaticSignalDiscrete *mode = NULL;

  ParameterInfoDiscrete *pOctave = NULL;
  ParameterInfoDiscrete *pKey = NULL;
  ParameterInfoDiscrete *pCenterNote = NULL;
  ParameterInfoDiscrete *pMode = NULL;
  int playingNotesPerKey[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

  int halfNotesIntervalsMajor[8] = {0,2,4,5,7,9,11,12}; 

};

class GlobalState {

  public:
  GlobalState(SynthEngine *engine, LiquidCrystal *lcd); 
  
  SynthMode synthMode;
  PartConfigMode partConfigMode;
  SequencerMode sequencerMode;
  SequencerModeGraphic sequencerModeGraphic;
  MixMuteMode mixMuteMode;
  KeyboardMode keyboardMode;
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

  // index like that: patterns[patternnumber][track] -> pointer to Sequence (nullptr if none)
  std::array<std::array<Sequence*, NB_PARTS>, NB_PATTERNS> patterns{};

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
    static const int NB_STEPS = 64;

    // steps: -1 means empty; >=0 is index into globalNotes
    std::array<int32_t, NB_STEPS> steps;

    Sequence() {
      steps.fill(-1);
    }
    
    // helper API moved here so callers read Sequence::...
    static inline void initGlobalLockPool();
    static inline int32_t allocGlobalLockNode();
    static inline void freeGlobalLockNode(int32_t nodeIndex);
    static inline int32_t allocGlobalNoteIndex();
    static inline void freeGlobalNoteIndex(int32_t noteIndex);
    static inline void clearNoteLocks(int32_t noteIndex);
    static inline int32_t allocateGlobalNote(const NoteEntry& note);
    static inline int32_t findNoteLockNode(int32_t noteIndex, ParameterInfo* param);
    static inline int noteLockCount(int32_t noteIndex);
    static inline bool addOrUpdateNoteLock(int32_t noteIndex, ParameterInfo* param, float value);
    static inline void copyNoteLocks(int32_t destNoteIndex, int32_t srcNoteIndex);
    static inline int32_t ensureNoteAtStep(Sequence* s, int step);
    static inline void clearSequence(Sequence* s);
    static inline void copySequence(Sequence* dest, Sequence* src);
};

// Implementations for Sequence helper functions
inline void Sequence::initGlobalLockPool() {
  if (globalLockPoolInitialized) return;
  globalLockPoolInitialized = true;
  globalFreeLockNodeHead = 0;
  for (int32_t i = 0; i < MAX_GLOBAL_LOCK_NODES - 1; i++) {
    globalLockNodes[i].next = i + 1;
  }
  globalLockNodes[MAX_GLOBAL_LOCK_NODES - 1].next = -1;
}

inline int32_t Sequence::allocGlobalLockNode() {
  if (!globalLockPoolInitialized) initGlobalLockPool();
  if (globalFreeLockNodeHead == -1) return -1;
  int32_t result = globalFreeLockNodeHead;
  globalFreeLockNodeHead = globalLockNodes[result].next;
  globalLockNodes[result].next = -1;
  return result;
}

inline void Sequence::freeGlobalLockNode(int32_t nodeIndex) {
  globalLockNodes[nodeIndex].next = globalFreeLockNodeHead;
  globalFreeLockNodeHead = nodeIndex;
}

inline int32_t Sequence::allocGlobalNoteIndex() {
  // Two-tier allocation: first try to reuse a freed note from the free-list,
  // then append a fresh one from the unused portion of the array.
  if (globalFreeNoteHead != -1) {
    // Reuse: pop from head of free-list (note->lockHead is the "next" pointer here)
    int32_t idx = globalFreeNoteHead;
    globalFreeNoteHead = globalNotes[idx].lockHead;
    return idx;
  }
  // Fresh: allocate from end of used portion (globalNextNoteIndex .. MAX_GLOBAL_NOTES-1)
  if (globalNextNoteIndex >= MAX_GLOBAL_NOTES) return -1;
  return globalNextNoteIndex++;
}

inline void Sequence::freeGlobalNoteIndex(int32_t noteIndex) {
  // Return note to free-list by prepending it (O(1) operation).
  // Use lockHead field as the "next" pointer since the note is no longer active.
  globalNotes[noteIndex].lockHead = globalFreeNoteHead;
  globalFreeNoteHead = noteIndex;
}

inline void Sequence::clearNoteLocks(int32_t noteIndex) {
  int32_t nodeIndex = globalNotes[noteIndex].lockHead;
  while (nodeIndex != -1) {
    int32_t next = globalLockNodes[nodeIndex].next;
    Sequence::freeGlobalLockNode(nodeIndex);
    nodeIndex = next;
  }
  globalNotes[noteIndex].lockHead = -1;
}

inline int32_t Sequence::allocateGlobalNote(const NoteEntry& note) {
  int32_t idx = Sequence::allocGlobalNoteIndex();
  if (idx < 0) return -1;
  globalNotes[idx] = note;
  return idx;
}

inline int32_t Sequence::findNoteLockNode(int32_t noteIndex, ParameterInfo* param) {
  int32_t nodeIndex = globalNotes[noteIndex].lockHead;
  while (nodeIndex != -1) {
    if (globalLockNodes[nodeIndex].param == param) return nodeIndex;
    nodeIndex = globalLockNodes[nodeIndex].next;
  }
  return -1;
}

inline int Sequence::noteLockCount(int32_t noteIndex) {
  int count = 0;
  int32_t nodeIndex = globalNotes[noteIndex].lockHead;
  while (nodeIndex != -1) {
    count++;
    nodeIndex = globalLockNodes[nodeIndex].next;
  }
  return count;
}

inline bool Sequence::addOrUpdateNoteLock(int32_t noteIndex, ParameterInfo* param, float value) {
  int32_t existing = Sequence::findNoteLockNode(noteIndex, param);
  if (existing != -1) {
    globalLockNodes[existing].value = value;
    return true;
  }
  if (Sequence::noteLockCount(noteIndex) >= 10) return false;
  int32_t nodeIndex = Sequence::allocGlobalLockNode();
  if (nodeIndex == -1) return false;
  globalLockNodes[nodeIndex].param = param;
  globalLockNodes[nodeIndex].value = value;
  globalLockNodes[nodeIndex].next = globalNotes[noteIndex].lockHead;
  globalNotes[noteIndex].lockHead = nodeIndex;
  return true;
}

inline void Sequence::copyNoteLocks(int32_t destNoteIndex, int32_t srcNoteIndex) {
  Sequence::clearNoteLocks(destNoteIndex);
  int32_t srcNode = globalNotes[srcNoteIndex].lockHead;
  int32_t lastCopied = -1;
  while (srcNode != -1) {
    if (Sequence::noteLockCount(destNoteIndex) >= 10) break;
    int32_t copyIndex = Sequence::allocGlobalLockNode();
    if (copyIndex == -1) break;
    globalLockNodes[copyIndex].param = globalLockNodes[srcNode].param;
    globalLockNodes[copyIndex].value = globalLockNodes[srcNode].value;
    globalLockNodes[copyIndex].next = -1;
    if (lastCopied == -1) {
      globalNotes[destNoteIndex].lockHead = copyIndex;
    } else {
      globalLockNodes[lastCopied].next = copyIndex;
    }
    lastCopied = copyIndex;
    srcNode = globalLockNodes[srcNode].next;
  }
}

inline int32_t Sequence::ensureNoteAtStep(Sequence* s, int step) {
  if (step < 0 || step >= Sequence::NB_STEPS) return -1;
  int32_t noteIndex = s->steps[step];
  if (noteIndex != -1) return noteIndex;
  NoteEntry note = {0, 0, 0, 1, -1};
  int32_t idx = Sequence::allocateGlobalNote(note);
  if (idx >= 0) s->steps[step] = idx;
  return idx;
}

inline void Sequence::clearSequence(Sequence* s) {
  for (int step = 0; step < Sequence::NB_STEPS; step++) {
    int32_t noteIndex = s->steps[step];
    if (noteIndex != -1) {
      Sequence::clearNoteLocks(noteIndex);
      Sequence::freeGlobalNoteIndex(noteIndex);
      s->steps[step] = -1;
    }
  }
}

inline void Sequence::copySequence(Sequence* dest, Sequence* src) {
  Sequence::clearSequence(dest);
  for (int step = 0; step < Sequence::NB_STEPS; step++) {
    int32_t srcNoteIndex = src->steps[step];
    if (srcNoteIndex == -1) continue;
    NoteEntry note = globalNotes[srcNoteIndex];
    note.lockHead = -1;
    int32_t destNoteIndex = Sequence::allocateGlobalNote(note);
    if (destNoteIndex == -1) break;
    Sequence::copyNoteLocks(destNoteIndex, srcNoteIndex);
    dest->steps[step] = destNoteIndex;
  }
}


