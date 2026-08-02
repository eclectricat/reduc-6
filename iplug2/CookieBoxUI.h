#pragma once

#include "PluginApp.h"

#if __has_include("IControl.h")
#include "IControl.h"
#include "IGraphics.h"

#include <memory>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IControl;
class IGraphics;

class CookieBoxUI {
public:
  explicit CookieBoxUI(CookieBoxPluginApp& app);
  ~CookieBoxUI();

  // Creates and attaches the display/knob/button controls.
  void Attach(IGraphics* pGraphics);

  // Call from your editor/plugin idle callback.
  void OnIdle();

private:
  CookieBoxPluginApp& mApp;
  IGraphics* mGraphics = nullptr;
  IControl* mDisplayControl = nullptr;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#else

// Fallback stub when iPlug2 graphics headers are not visible in this translation unit.
class CookieBoxUI {
public:
  explicit CookieBoxUI(CookieBoxPluginApp&) {}
  ~CookieBoxUI() = default;
  void Attach(void*) {}
  void OnIdle() {}
};

#endif
