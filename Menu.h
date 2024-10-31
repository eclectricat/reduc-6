#ifndef MENU_H
#define MENU_H

#include <vector>
#include <string>

#include "SynthEngine.h"

using namespace std;

// there are 3 types of values
// raw potentiometer values (0-1024)
// actual internal float values (range defined by min and max)
// printable value (0-100) for display in lcd where we have only 2 characters
//
// locking logic
// normal mode: value is updated directly
// locking: set value to lock value, store normal value in storedValue
// while locked: any parameter updates (from pots) go to stored Value
// unlocking: set value to stored Value

class ParameterInfo {

  public: 
    ParameterInfo(String name, float min, float max, StaticSignal* param)  {
      this->name = name;
      this->min = min;
      this->max = max;
      this->param = param;
    }

    // update the parameter given the current pot value
    // scale and convert to flow
    void updateParameter(int potValue) {
      // 1024 is the max pot value
      float targetValue = this->min + (potValue/1024.0) * (this->max - this->min);

      if(!locked) this->param->setValue(targetValue);
      else this->storedValue = targetValue;
    }

    void lock(int potValue) {
      locked = true;
      storedValue = param->getValue();
      float targetValue = this->min + (potValue/1024.0) * (this->max - this->min);
      param->setValue(targetValue);
      Serial.print("lock");
      Serial.println(this->getName());
    }

    void unlock() {
      locked = false;
      param->setValue(storedValue);
      Serial.print("unlock");
      Serial.println(this->getName());
    }

    String getName() {return name;}
    
    // we only really have 3 digits, so map the value back to a 0..100 scale
    virtual int printableValue() {return (int)( 100* (param->getValue() - this->min) / (this->max - this->min) );}

    virtual int printableValueFromPotValue(int potValue) {
      return 100 * (potValue/1024.0);
    }

    float getValue() {return param->getValue();}
    //void setValue(float newValue) {param->setValue(newValue);} // directly set the parameter value (used internally e.g. for parameter automation)

  protected:
    String name;
    float min;
    float max;
    StaticSignal* param;

    float storedValue;
    bool locked = false;
};

class ParameterInfoDiscrete: public ParameterInfo {
  
  public:
  ParameterInfoDiscrete(String name, float min, float max, StaticSignal* param):ParameterInfo(name, min, max, param) {
  }

  int printableValue() override {
    return (int) param->getValue();
  }

  virtual int printableValueFromPotValue(int potValue) {
      return (int) (this->min + (potValue/1024.0) * (this->max - this->min));
  }
};

/**
* datastructure to keep all the info about the menu, how to navigate
* e.g. parameter names, ranges, and StaticSignals to store their current value
*/

class SynthParameters {

  public:
    SynthParameters() {
      for(int i=0;i<nbLanes; i++) {
        vector<vector<ParameterInfo*>> newLane;
        lanes.push_back(newLane);
      }
    }

    void addPage(vector<ParameterInfo*> newParams, unsigned int laneId) {
      if (laneId < lanes.size()) {
        lanes.at(laneId).push_back(newParams);
        Serial.print("adding params to lane ");
        Serial.println(laneId);
      } else {
        Serial.println("Can't add parameters");
      }
    }

    vector<ParameterInfo*> getPage(int laneId, int pageId) {
      if (existPage(laneId, pageId)) {
        return lanes.at(laneId).at(pageId);
      } else {
        Serial.println("Page does not exist");
        return getPage(0,0);
      }
    }

    int getNbLanes() {
      return lanes.size();
    }

    int getNbPages(int laneId) {
      return lanes.at(laneId).size();
    }

    bool existPage(unsigned int laneId, unsigned int pageId) {
      return (laneId < lanes.size()) && (pageId < lanes.at(laneId).size());
    }
    
  private:
    const static int nbLanes = 6;
    vector<vector<vector<ParameterInfo*>>> lanes;
};

#endif