#include "CookiePlug.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

//#include "CookieBoxPlugin.h"

//#if __has_include("IPlug_include_in_plug_hdr.h")
//#include "IPlug_include_in_plug_src.h"

#include <algorithm>
#include <cstring>

BEGIN_IPLUG_NAMESPACE

namespace {
constexpr int kNumParams = 0;
constexpr int kNumPresets = 1;
}

/*CookiePlug::CookiePlug(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  SetTailSize(0);

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));
  };
#endif
}*/

CookiePlug::CookiePlug(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPresets))
  , mUI(mApp) {
  mApp.initialize();

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, igraphics::GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](igraphics::IGraphics* pGraphics) {
    mUI.Attach(pGraphics);
  };
}

#if IPLUG_DSP
void CookiePlug::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  const int nOut = NOutChansConnected();
  for (int c = 0; c < nOut; ++c) {
    std::memset(outputs[c], 0, static_cast<size_t>(nFrames) * sizeof(sample));
  }

  mApp.processAudio(reinterpret_cast<double**>(outputs), nFrames, nOut);
}


#endif

void CookiePlug::OnIdle() {
  mUI.OnIdle();
}

END_IPLUG_NAMESPACE

