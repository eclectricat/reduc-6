#pragma once

#include "IPlug_include_in_plug_hdr.h"


#include "CookieBoxUI.h"
#include "PluginApp.h"

const int kNumPresets = 1;

/*enum EParams
{
  kGain = 0,
  kNumParams
};*/

//using namespace iplug;
//using namespace igraphics;

BEGIN_IPLUG_NAMESPACE

class CookiePlug final : public Plugin
{
public:
  CookiePlug(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnIdle() override;
#endif

private:
  CookieBoxPluginApp mApp;
  igraphics::CookieBoxUI mUI;
};

END_IPLUG_NAMESPACE

