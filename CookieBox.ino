/* Simple "Hello World" example.

   After uploading this to your board, use Serial Monitor
   to view the message.  When Serial is selected from the
   Tools > USB Type menu, the correct serial port must be
   selected from the Tools > Serial Port AFTER Teensy is
   running this code.  Teensy only becomes a serial device
   while this code is running!  For non-Serial types,
   the Serial port is emulated, so no port needs to be
   selected.

   This example code is in the public domain.
*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

#include <stdlib.h>

#include <usb_audio.h>

//#include <LiquidCrystal_I2C.h>
//#include <LiquidCrystal_PCF8574.h>
#include <LiquidCrystal.h>

#include "SynthEngine.h"
#include "Menu.h"
#include "Modes.h"


// GUItool: begin automatically generated code
SynthEngine       engine;      //xy=227,175
AudioAmplifier ampL;
AudioAmplifier ampR;
AudioOutputI2S           i2s1;           //xy=494,158
AudioConnection          patchCord1(engine, 0, ampL, 0);
AudioConnection          patchCord1b(engine, 1, ampR, 0);
AudioConnection          patchCord2(ampL, 0, i2s1, 0);
AudioConnection          patchCord3(ampR, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=448,312
// GUItool: end automatically generated code

AudioOutputUSB usbAudio;
AudioConnection          patchCord4(ampL, 0, usbAudio, 0);
AudioConnection          patchCord5(ampR, 0, usbAudio, 1);

const int buttonPin = 28;

const int buttonPins[] = {28, 29, 30, 31, 32, 35, 34, 33};
Bounce* pushbuttons[8];
Bounce pushbutton = Bounce(buttonPin, 10);  // 10 ms debounce

const int potPins[] = {A13, A12, A11, A10};

const int led1Pin = 37;

//Menu menu;
//Menu2 menu2;

int selectedMenuItem = 0;
int potMoved = 0;
int potValueOnParamChange = 0;

int nbPots = 4;
int potsMoved[] = {0,0,0,0};
int potValuesOnParamChange[] = {0,0,0,0};

//int selectedLane = 0;
//int selectedPage = 0;
//vector<ParameterInfo*> currentMenuPage;

int val;
int currentPotVals[] = {0,0,0,0};


int nbLoopPasses=0;
unsigned long startMillis=0;

//JsonDocument jprofiling;
std::map<String, int> profiling;
std::map<String, int> moduleCounter;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
//const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;

const int rs = 12, en = 11, d4 = 38, d5 = 39, d6 = 40, d7 = 41;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

GlobalState globalState(&engine, &lcd);

//globalState.setup(); // intialise, so the Modes can call back into globalState


void myNoteOn(byte channel, byte note, byte velocity) {
  globalState.myNoteOn(channel, note, velocity);
}
void myNoteOff(byte channel, byte note, byte velocity) {
  globalState.myNoteOff(channel, note, velocity);
}



void lockPotentiometers(bool refreshReadingFirst) {
  for(int pt = 0; pt < nbPots; pt++) {
        
        if(refreshReadingFirst)  currentPotVals[pt] = analogRead(potPins[pt]);
        potsMoved[pt] = 0;
        potValuesOnParamChange[pt] = currentPotVals[pt];
      }
}


void setup()
{
  Serial.begin(9600); // USB is always 12 Mbit/sec
  //waveform1.frequency(500);
  //engine.frequency(500);
  globalState.setup(); // intialise, so the Modes can call back into globalState
  engine.buildEngine(globalState.synthMode.allSynthParameters, globalState.sequencerMode.bpm);
  //engine.buildEngine(globalState.synthMode.allSynthParameters);

  Serial.println("BUILDENGINE DONE");

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(100);

  Serial.println("AUDIOMEM DONE");
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.8); // caution: very loud - use oscilloscope only!
  ampL.gain(1);
  ampR.gain(1);

  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandleNoteOff(myNoteOff);
  //usbMIDI.setHandleControlChange(myControlChange);

  delay(100);

  pinMode(led1Pin, OUTPUT);
  digitalWrite(led1Pin, LOW);
  
  for(int i=0;i<8;i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
    Bounce *temp = new Bounce(buttonPins[i], 10);
    pushbuttons[i]= temp;
  }

  lcd.begin(16, 2);
  lcd.print("hello, cookie!?");

  globalState.synthMode.setup();
  globalState.partConfigMode.setup();
  globalState.sequencerMode.setup();
  globalState.mixMuteMode.setup();
  lockPotentiometers(true);


  if ( ARM_DWT_CYCCNT == ARM_DWT_CYCCNT ) {
		// Enable CPU Cycle Count
		ARM_DEMCR |= ARM_DEMCR_TRCENA;
		ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	}

  Serial.print("Initializing SD card...");
  
  // see if the card is present and can be initialized:
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Card failed, or not present");
  } else
    Serial.println("card initialized.");
}



void loop()
{
  nbLoopPasses++;

  // print some stats
  unsigned int now = millis();
  if (now > startMillis + 10000) {
  //if (false) {
    startMillis = now;

    Serial.print("loops per second:");
    Serial.println(nbLoopPasses / 10.0f);
    nbLoopPasses = 0;

    Serial.print("Max sound level: ");
    Serial.println(engine.getAndResetMaxLevel());

    Serial.print("Audio processor usage max:");
    Serial.println(AudioProcessorUsageMax());
    AudioProcessorUsageMaxReset();

    // profiling info
    /*for (int s = 0; s < engine.registry.getNbSignals(); s++) {
      Signal* as = engine.registry.getSignals()[s];
      profiling[as->signame()] = profiling[as->signame()] + as->getAndResetLastSpentTime();
      moduleCounter[as->signame()] = moduleCounter[as->signame()] + 1;
    }
    

    std::map<String, int>::iterator itr;
    for (itr = profiling.begin(); itr != profiling.end(); ++itr) {
      Serial.print(itr->first.c_str());
      Serial.print(" - ");
      Serial.print(itr->second/1000000);
      Serial.print("... nbInstances ");
      Serial.println(moduleCounter[itr->first]);
    }*/

    profiling.clear();
    moduleCounter.clear();

  }

  // do high prio stuff first

  // sequencer
  if (globalState.seqPlaying) globalState.sequencerMode.maybePlay();

   //for (int i=0;i<30;i++) {
    int received = usbMIDI.read();
    /*if(received) {
      Serial.print(usbMIDI.getType());
      Serial.print("-");
      Serial.print(usbMIDI.getData1());
      Serial.print("-");
      Serial.println(usbMIDI.getData2());
    }*/
    
  //}

  // delayed display update
  if (globalState.delayedDisplayRefresh > 0) {
    if (millis() > globalState.delayedDisplayRefresh) {
      globalState.delayedDisplayRefresh = -1;
      globalState.selectedMode->fullDisplayUpdate();
    }
  }

  
  if (nbLoopPasses%7 == 0) { // do lo prio stuff


  // check for pots moved

  for(int pt = 0; pt < nbPots; pt++) {
        currentPotVals[pt] = analogRead(potPins[pt]);
        if (potsMoved[pt] > 0) {
          globalState.selectedMode->processPotValue(pt, currentPotVals[pt], ((now / 10 % 10) == 0));

        } else {
          if (abs(currentPotVals[pt]-potValuesOnParamChange[pt]) > 30 ) {
            potsMoved[pt] = 1;
            Serial.println("potentiometer moved,  now param is unlocked");
          }
        }
  }

  // check for buttons pressed

  for (int i=0;i<8;i++) {
  if (pushbuttons[i]->update()) {
    if (pushbuttons[i]->fallingEdge()) {
      Serial.print(i);
      Serial.println(" Button press");

      // TODO: only reset this if an actual page change has happened, But it is not totally broken like that...
      // also for e.g. sequencermode, params need to be locked e.g. when step changes
      
      bool needToLock = globalState.selectedMode->pushButtonPressed(i);
      if (needToLock) lockPotentiometers(false); // typically after changing parameter pages. 

      if(i==6) {
          Serial.println("Testing sd card");

          File dataFile = SD.open("test.txt", FILE_WRITE);

          // if the file is available, write the contents of datastring to it
          if (dataFile) {
            dataFile.println("test123");
            dataFile.close();
            Serial.println("wrote file");
          } else {
            Serial.println("error opening test.txt");
          }

          Serial.println("listing files");
          File dir = SD.open("/");
          File entry = dir.openNextFile();
          while(entry) {
            Serial.println(entry.name());
            entry.close();
            entry = dir.openNextFile();
          }
          Serial.println("done listing files");
      }

    } else {
      Serial.println("Button release");
      bool needToLock = globalState.selectedMode->pushButtonReleased(i);
      if (needToLock) lockPotentiometers(false); // e.g. after editing parameterLock, we need to reset/lock the pots again 

    }
  }
  }

  } // low prio throttling

 

  delay(5);  // do not print too fast!
}

