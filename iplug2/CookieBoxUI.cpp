#include "CookieBoxUI.h"

#if __has_include("IControl.h")

#include <algorithm>
#include <array>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <string>
#include <unordered_set>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

namespace {
static const IColor kBg(255, 255, 243, 176);
static const IColor kDisplayPanel(255, 224, 224, 224);
static const IColor kKnob(255, 255, 255, 255);
static const IColor kBlack(255, 0, 0, 0);
static const IColor kInk(255, 230, 236, 240);
static const IColor kAccent(255, 60, 170, 190);
static const std::array<IColor, 10> kButtonColors = {
  IColor(255, 239, 83, 80),
  IColor(255, 255, 152, 0),
  IColor(255, 255, 235, 59),
  IColor(255, 156, 204, 101),
  IColor(255, 76, 175, 80),
  IColor(255, 38, 198, 218),
  IColor(255, 66, 165, 245),
  IColor(255, 126, 87, 194),
  IColor(255, 176, 176, 176),
  IColor(255, 96, 96, 96)
};

static const IText kLabelText(13.f, kInk, "UI-Label", EAlign::Center, EVAlign::Middle);
static const IText kButtonLabelText(13.f, kBlack, "UI-Label", EAlign::Center, EVAlign::Middle);
static const IText kDisplayText(28.f, kBlack, "UI-Display", EAlign::Near, EVAlign::Middle);

class DisplayControl final : public IControl {
public:
  DisplayControl(const IRECT& bounds, CookieBoxPluginApp& app)
    : IControl(bounds), mApp(app) {}

  void Draw(IGraphics& g) override {
    g.FillRect(kDisplayPanel, mRECT);
    g.DrawRect(kBlack, mRECT, nullptr, 1.5f);

    const IRECT row0 = mRECT.GetReducedFromTop(4.f).GetFromTop(mRECT.H() * 0.5f - 2.f).GetPadded(-6.f);
    const IRECT row1 = mRECT.GetReducedFromBottom(4.f).GetFromBottom(mRECT.H() * 0.5f - 2.f).GetPadded(-6.f);

    g.DrawText(kDisplayText, mApp.getDisplayLine(0).c_str(), row0);
    g.DrawText(kDisplayText, mApp.getDisplayLine(1).c_str(), row1);
  }

private:
  CookieBoxPluginApp& mApp;
};

class KnobControl final : public IControl {
public:
  KnobControl(const IRECT& bounds, CookieBoxPluginApp& app, int knobIndex, const char* label)
    : IControl(bounds)
    , mApp(app)
    , mKnobIndex(knobIndex)
    , mLabel(label) {}

  void Draw(IGraphics& g) override {
    const IRECT knobRect = mRECT.GetPadded(-8.f).GetFromTop(mRECT.H() - 20.f);
    const IRECT labelRect = mRECT.GetFromBottom(18.f);

    const float r = std::min(knobRect.W(), knobRect.H()) * 0.42f;
    const float cx = knobRect.MW();
    const float cy = knobRect.MH();

    g.FillCircle(kKnob, cx, cy, r);
    g.DrawCircle(kBlack, cx, cy, r, nullptr, 1.5f);

    // Use a conventional 270-degree knob sweep from ~7:30 (min) to ~4:30 (max).
    const float startTheta = 0.75f * static_cast<float>(M_PI);
    const float endTheta = 2.25f * static_cast<float>(M_PI);
    const float theta = startTheta + static_cast<float>(mValue) * (endTheta - startTheta);
    const float ix = cx + std::cos(theta) * r * 0.72f;
    const float iy = cy + std::sin(theta) * r * 0.72f;

    g.DrawLine(kBlack, cx, cy, ix, iy, nullptr, 3.f);
    g.DrawText(kLabelText, mLabel.c_str(), labelRect);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    mDragStartY = y;
    mDragStartValue = mValue;
    mMouseDown = true;
    SetDirty(false);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override {
    if (!mMouseDown) return;

    const float delta = (mDragStartY - y) / 180.f;
    mValue = std::clamp(mDragStartValue + delta, 0.0, 1.0);
    mApp.setKnobNormalized(mKnobIndex, static_cast<float>(mValue));
    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override {
    mMouseDown = false;
    SetDirty(false);
  }

private:
  CookieBoxPluginApp& mApp;
  int mKnobIndex = 0;
  std::string mLabel;
  bool mMouseDown = false;
  float mDragStartY = 0.f;
  double mDragStartValue = 0.0;
  double mValue = 0.0;
};

class ButtonControl final : public IControl {
public:
  ButtonControl(const IRECT& bounds, CookieBoxPluginApp& app, int buttonIndex, char keyLabel, const IColor& color)
    : IControl(bounds)
    , mApp(app)
    , mButtonIndex(buttonIndex)
    , mKeyLabel(1, keyLabel)
    , mColor(color) {
    SetWantsMultiTouch(true);
  }

  void Draw(IGraphics& g) override {
    const IColor fill = mIsDown ? kKnob : mColor;

    g.FillRoundRect(fill, mRECT, 7.f);
    g.DrawRoundRect(kBlack, mRECT, 7.f, nullptr, 1.5f);
    g.DrawText(kButtonLabelText, mKeyLabel.c_str(), mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    mActiveTouchIDs.insert(mod.touchID);

    if (!mIsDown) {
      mIsDown = true;
      mApp.setButtonState(mButtonIndex, true);
      SetDirty(false);
    }
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override {
    mActiveTouchIDs.erase(mod.touchID);

    if (mIsDown && mActiveTouchIDs.empty()) {
      mIsDown = false;
      mApp.setButtonState(mButtonIndex, false);
      SetDirty(false);
    }
  }

  void OnTouchCancelled(float x, float y, const IMouseMod& mod) override {
    OnMouseUp(x, y, mod);
  }

  void OnMouseOut() override {
    // Keep touch presses active when another finger leaves bounds.
    if (mIsDown && mActiveTouchIDs.empty()) {
      mIsDown = false;
      mApp.setButtonState(mButtonIndex, false);
      SetDirty(false);
    }
  }

private:
  CookieBoxPluginApp& mApp;
  int mButtonIndex = 0;
  std::string mKeyLabel;
  IColor mColor;
  bool mIsDown = false;
  std::unordered_set<ITouchID> mActiveTouchIDs;
};

}

CookieBoxUI::CookieBoxUI(CookieBoxPluginApp& app)
  : mApp(app) {}

CookieBoxUI::~CookieBoxUI() = default;

void CookieBoxUI::Attach(IGraphics* pGraphics) {
  if (!pGraphics) return;
  mGraphics = pGraphics;
  mGraphics->EnableMultiTouch(true);

  const IRECT bounds = mGraphics->GetBounds();
  mGraphics->AttachPanelBackground(kBg);

  // Try bundled fonts first, then reliable system families as fallback.
  bool loadedLabelFont = mGraphics->LoadFont("UI-Label", "Roboto-Regular.ttf");
  if (!loadedLabelFont) loadedLabelFont = mGraphics->LoadFont("UI-Label", "Helvetica", ETextStyle::Normal);
  if (!loadedLabelFont) loadedLabelFont = mGraphics->LoadFont("UI-Label", "Arial", ETextStyle::Normal);

  bool loadedDisplayFont = mGraphics->LoadFont("UI-Display", "Menlo", ETextStyle::Normal);
  if (!loadedDisplayFont) loadedDisplayFont = mGraphics->LoadFont("UI-Display", "Courier New", ETextStyle::Normal);
  if (!loadedDisplayFont) loadedDisplayFont = mGraphics->LoadFont("UI-Display", "Courier", ETextStyle::Normal);
  if (!loadedDisplayFont) loadedDisplayFont = mGraphics->LoadFont("UI-Display", "Roboto-Regular.ttf");
  if (!loadedDisplayFont) mGraphics->LoadFont("UI-Display", "Arial", ETextStyle::Normal);

  mGraphics->SetKeyHandlerFunc([this](const IKeyPress& key, bool isUp) {
    const char c = key.utf8[0];
    if (c == '\0') return false;

    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    std::fprintf(stderr, "[CookieBoxUI] key %c %s\n", upper, isUp ? "up" : "down");
    mApp.setKeyState(upper, !isUp);
    return true;
  });

  const float outerPad = std::max(8.f, std::min(bounds.W(), bounds.H()) * 0.02f);
  const IRECT content = bounds.GetPadded(-outerPad);

  // Stable landscape-friendly proportions that also behave on square/tall views.
  const IRECT topArea = content.GetFromTop(content.H() * 0.24f);
  const IRECT displayRect = topArea.GetMidHPadded(topArea.W() * 0.25f).GetReducedFromTop(16.f);

  auto* display = new DisplayControl(displayRect, mApp);
  mDisplayControl = display;
  mGraphics->AttachControl(display);

  const IRECT knobBand = content.GetFromTop(content.H() * 0.70f).GetFromBottom(content.H() * 0.30f).GetPadded(-2.f);
  const char* knobLabels[4] = {"K1", "K2", "K3", "K4"};
  const float knobW = knobBand.W() / 4.f;
  for (int i = 0; i < 4; ++i) {
    const IRECT r = IRECT(knobBand.L + i * knobW, knobBand.T, knobBand.L + (i + 1) * knobW, knobBand.B).GetPadded(-4.f);
    mGraphics->AttachControl(new KnobControl(r, mApp, i, knobLabels[i]));
  }

  const IRECT buttonRow = content.GetFromBottom(content.H() * 0.22f).GetPadded(-2.f);
  const std::array<char, 10> keys = {'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P'};
  const float buttonW = buttonRow.W() / static_cast<float>(keys.size());
  for (int i = 0; i < static_cast<int>(keys.size()); ++i) {
    const IRECT r = IRECT(buttonRow.L + i * buttonW, buttonRow.T, buttonRow.L + (i + 1) * buttonW, buttonRow.B).GetPadded(-4.f);
    mGraphics->AttachControl(new ButtonControl(r, mApp, i, keys[i], kButtonColors[i]));
  }
}

void CookieBoxUI::OnIdle() {
  mApp.tickUI();
  if (mDisplayControl) {
    mDisplayControl->SetDirty(false);
  }
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif
