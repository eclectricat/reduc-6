#include "CookieBoxPlugin.h"

#if __has_include("IPlug_include_in_plug_hdr.h")

#include "IPlug_include_in_plug_src.h"

#include <algorithm>
#include <cstring>

BEGIN_IPLUG_NAMESPACE

namespace {
constexpr int kNumParams = 0;
constexpr int kNumPresets = 1;
}

CookieBoxPlugin::CookieBoxPlugin(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPresets))
  , mUI(mApp) {
  mApp.initialize();

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](igraphics::IGraphics* pGraphics) {
    mUI.Attach(pGraphics);
  };
}

void CookieBoxPlugin::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  const int nOut = NOutChansConnected();
  for (int c = 0; c < nOut; ++c) {
    std::memset(outputs[c], 0, static_cast<size_t>(nFrames) * sizeof(sample));
  }

  mApp.processAudio(reinterpret_cast<double**>(outputs), nFrames, nOut);
}

void CookieBoxPlugin::ProcessMidiMsg(const IMidiMsg& msg) {
  mApp.processMidiMessage(msg.mStatus, msg.mData1, msg.mData2);
}

void CookieBoxPlugin::OnIdle() {
  mUI.OnIdle();
}

END_IPLUG_NAMESPACE

#endif
