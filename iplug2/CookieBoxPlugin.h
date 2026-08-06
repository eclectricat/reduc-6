#pragma once

#include "CookieBoxUI.h"
#include "PluginApp.h"

#if __has_include("IPlug_include_in_plug_hdr.h")
#include "IPlug_include_in_plug_hdr.h"

BEGIN_IPLUG_NAMESPACE

class CookieBoxPlugin final : public Plugin {
public:
  explicit CookieBoxPlugin(const InstanceInfo& info);

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnIdle() override;

private:
  CookieBoxPluginApp mApp;
  igraphics::CookieBoxUI mUI;
};

END_IPLUG_NAMESPACE

#endif
