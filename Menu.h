#ifndef MENU_H
#define MENU_H

#include <vector>
#include <string>

#include "SynthEngine.h"

using namespace std;

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
      this->param->setValue(targetValue);
    }

    String getName() {return name;}
    
    // we only really have 3 digits, so map the value back to a 0..100 scale
    virtual int printableValue() {return (int)( 100* (param->getValue() - this->min) / (this->max - this->min) );}

    float getValue() {return param->getValue();}

  protected:
    String name;
    float min;
    float max;
    StaticSignal* param;
};

class ParameterInfoDiscrete: public ParameterInfo {
  
  public:
  ParameterInfoDiscrete(String name, float min, float max, StaticSignal* param):ParameterInfo(name, min, max, param) {
  }

  int printableValue() override {
    return (int) param->getValue();
  }
};

/**
* datastructure to keep all the info about the menu, how to navigate
* e.g. parameter names, ranges, and StaticSignals to store their current value
*/
class Menu {

  public:
    void addItem(ParameterInfo* newParam) {
      items[nbItems++]=newParam;
      // TODO check we don't exceed limits
    }

    ParameterInfo* getItem(int index) {
      return items[index];
    }

    int getNbItems() {return nbItems;}

  private:
    const static int currentCapacity = 100;
    int nbItems = 0;

    ParameterInfo* items[currentCapacity];

};

class Menu2 {

  public:
    Menu2() {
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