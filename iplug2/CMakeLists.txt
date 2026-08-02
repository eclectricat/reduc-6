# CMake scaffold for building CookieBox with iPlug2 headers available.
# This does not embed the iPlug2 repository as a CMake subproject.

cmake_minimum_required(VERSION 3.15)
project(CookieBoxPlugin LANGUAGES CXX OBJC OBJCXX)

# Optional explicit path override: -DIPLUG2_DIR=/path/to/iPlug2
set(IPLUG2_DIR "" CACHE PATH "Path to iPlug2 root")

if(NOT IPLUG2_DIR)
  set(_iplug_candidates
    "${CMAKE_CURRENT_SOURCE_DIR}/../../iPlug2"
    "$ENV{HOME}/Projekte/AudioPlugin/iPlug2"
    "$ENV{HOME}/Documents/iPlug2"
  )

  foreach(_cand IN LISTS _iplug_candidates)
    if(EXISTS "${_cand}/IPlug/IPlug_include_in_plug_hdr.h")
      set(IPLUG2_DIR "${_cand}" CACHE PATH "Path to iPlug2 root" FORCE)
      break()
    endif()
  endforeach()
endif()

if(NOT IPLUG2_DIR)
  message(FATAL_ERROR "iPlug2 not found. Set IPLUG2_DIR to your iPlug2 checkout.")
endif()

get_filename_component(_src_real "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)
get_filename_component(_iplug_real "${IPLUG2_DIR}" REALPATH)

if(_src_real STREQUAL _iplug_real)
  set(_fixed FALSE)
  set(_repair_candidates
    "${CMAKE_CURRENT_SOURCE_DIR}/../../iPlug2"
    "$ENV{HOME}/Projekte/AudioPlugin/iPlug2"
    "$ENV{HOME}/Documents/iPlug2"
  )

  foreach(_cand IN LISTS _repair_candidates)
    if(EXISTS "${_cand}/IPlug/IPlug_include_in_plug_hdr.h")
      get_filename_component(_cand_real "${_cand}" REALPATH)
      if(NOT _cand_real STREQUAL _src_real)
        set(IPLUG2_DIR "${_cand}" CACHE PATH "Path to iPlug2 root" FORCE)
        set(_iplug_real "${_cand_real}")
        set(_fixed TRUE)
        break()
      endif()
    endif()
  endforeach()

  if(NOT _fixed)
    message(FATAL_ERROR
      "IPLUG2_DIR resolves to this project directory (${IPLUG2_DIR}). "
      "Point IPLUG2_DIR to the external iPlug2 SDK root.")
  endif()
endif()

if(NOT EXISTS "${IPLUG2_DIR}/IPlug/IPlug_include_in_plug_hdr.h")
  message(FATAL_ERROR
    "Invalid IPLUG2_DIR: ${IPLUG2_DIR}. Expected IPlug/IPlug_include_in_plug_hdr.h.")
endif()

option(COOKIEBOX_BOOTSTRAP_SAMPLE "Build iPlug sample skeleton before CookieBox integration" ON)

set(IPLUG_RUNTIME_SOURCES
  ${IPLUG2_DIR}/IPlug/IPlugAPIBase.cpp
  ${IPLUG2_DIR}/IPlug/IPlugParameter.cpp
  ${IPLUG2_DIR}/IPlug/IPlugPluginBase.cpp
  ${IPLUG2_DIR}/IPlug/IPlugProcessor.cpp
  ${IPLUG2_DIR}/IPlug/IPlugTimer.cpp
  ${IPLUG2_DIR}/IPlug/IPlugPaths.mm
  ${IPLUG2_DIR}/IPlug/APP/IPlugAPP.cpp
  ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_dialog.cpp
  ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_host.cpp
  ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_main.cpp
  ${IPLUG2_DIR}/Dependencies/IPlug/RTAudio/RtAudio.cpp
  ${IPLUG2_DIR}/Dependencies/IPlug/RTMidi/RtMidi.cpp
  ${IPLUG2_DIR}/IGraphics/IGraphics.cpp
  ${IPLUG2_DIR}/IGraphics/IGraphicsEditorDelegate.cpp
  ${IPLUG2_DIR}/IGraphics/IControl.cpp
  ${IPLUG2_DIR}/IGraphics/Controls/IControls.cpp
  ${IPLUG2_DIR}/IGraphics/Controls/ITextEntryControl.cpp
  ${IPLUG2_DIR}/IGraphics/Controls/IPopupMenuControl.cpp
  ${IPLUG2_DIR}/IGraphics/Drawing/IGraphicsNanoVG_src.m
  ${IPLUG2_DIR}/IGraphics/Platforms/IGraphicsMac.mm
  ${IPLUG2_DIR}/IGraphics/Platforms/IGraphicsMac_view.mm
  ${IPLUG2_DIR}/IGraphics/Platforms/IGraphicsCoreText.mm
  ${IPLUG2_DIR}/WDL/swell/swell.cpp
  ${IPLUG2_DIR}/WDL/swell/swell-modstub.mm
  ${IPLUG2_DIR}/WDL/swell/swell-dlg.mm
  ${IPLUG2_DIR}/WDL/swell/swell-gdi.mm
  ${IPLUG2_DIR}/WDL/swell/swell-ini.cpp
  ${IPLUG2_DIR}/WDL/swell/swell-kb.mm
  ${IPLUG2_DIR}/WDL/swell/swell-menu.mm
  ${IPLUG2_DIR}/WDL/swell/swell-misc.mm
  ${IPLUG2_DIR}/WDL/swell/swell-miscdlg.mm
  ${IPLUG2_DIR}/WDL/swell/swell-wnd.mm
)

set(COOKIEBOX_SOURCES
  CookieBoxPlugin.cpp
  CookieBoxPlugin.h
  PluginApp.cpp
  PluginApp.h
  CookieBoxUI.cpp
  CookieBoxUI.h
  IPlugDisplay.cpp
  IPlugDisplay.h
  SystemAdapter.cpp
  compat/DesktopArduinoShims.cpp
  ../Modes.cpp
  ../SynthEngine.cpp
)

set(SAMPLE_ROOT "${IPLUG2_DIR}/Examples/IPlugEffect")
set(SAMPLE_SOURCES
  ${SAMPLE_ROOT}/IPlugEffect.cpp
)

if(COOKIEBOX_BOOTSTRAP_SAMPLE)
  message(STATUS "COOKIEBOX_BOOTSTRAP_SAMPLE=ON: building IPlugEffect sample skeleton")
  set(PROJECT_SOURCES ${SAMPLE_SOURCES})
  set(PROJECT_INCLUDE_DIRS
    ${SAMPLE_ROOT}
    ${SAMPLE_ROOT}/resources
  )
  set(PROJECT_DEFINES
    OBJC_PREFIX=vIPlugEffect
    SWELL_APP_PREFIX=Swell_vIPlugEffect
  )
else()
  message(STATUS "COOKIEBOX_BOOTSTRAP_SAMPLE=OFF: building CookieBox integration target")
  set(PROJECT_SOURCES ${COOKIEBOX_SOURCES})
  set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../.pio/libdeps/teensy41/ArduinoJson/src
  )
  set(PROJECT_DEFINES
    OBJC_PREFIX=vCookieBox
    SWELL_APP_PREFIX=Swell_vCookieBox
  )
endif()

add_library(CookieBoxPlugin MODULE
  ${PROJECT_SOURCES}
  ${IPLUG_RUNTIME_SOURCES}
)

set_source_files_properties(
  ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_main.cpp
  PROPERTIES LANGUAGE OBJCXX
)

set_source_files_properties(
  ${IPLUG2_DIR}/IGraphics/Drawing/IGraphicsNanoVG_src.m
  PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
)

target_include_directories(CookieBoxPlugin PRIVATE
  ${PROJECT_INCLUDE_DIRS}
  ${IPLUG2_DIR}
  ${IPLUG2_DIR}/IPlug
  ${IPLUG2_DIR}/IPlug/APP
  ${IPLUG2_DIR}/IPlug/Extras
  ${IPLUG2_DIR}/Dependencies/IPlug/RTAudio
  ${IPLUG2_DIR}/Dependencies/IPlug/RTMidi
  ${IPLUG2_DIR}/IGraphics
  ${IPLUG2_DIR}/IGraphics/Drawing
  ${IPLUG2_DIR}/IGraphics/Controls
  ${IPLUG2_DIR}/IGraphics/Platforms
  ${IPLUG2_DIR}/WDL
  ${IPLUG2_DIR}/WDL/swell
  ${IPLUG2_DIR}/Dependencies/IGraphics/NanoVG/src
  ${IPLUG2_DIR}/Dependencies/IGraphics/NanoSVG/src
  ${IPLUG2_DIR}/Dependencies/IGraphics/MetalNanoVG/src
  ${IPLUG2_DIR}/Dependencies/IGraphics/STB
)

target_compile_features(CookieBoxPlugin PRIVATE cxx_std_17)
target_compile_definitions(CookieBoxPlugin PRIVATE
  APP_API=1
  IPLUG_EDITOR=1
  IPLUG_DSP=1
  IPLUG_SWELL=0
  WDL_NO_DEFINE_MINMAX=1
  IGRAPHICS_NANOVG=1
  IGRAPHICS_METAL=1
  ${PROJECT_DEFINES}
)

if(APPLE)
  find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
  find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
  find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
  find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
  find_library(CORETEXT_FRAMEWORK CoreText REQUIRED)
  find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
  find_library(CARBON_FRAMEWORK Carbon REQUIRED)
  find_library(METAL_FRAMEWORK Metal REQUIRED)
  find_library(IOKIT_FRAMEWORK IOKit REQUIRED)

  target_link_libraries(CookieBoxPlugin PRIVATE
    ${COCOA_FRAMEWORK}
    ${APPKIT_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK}
    ${COREGRAPHICS_FRAMEWORK}
    ${CORETEXT_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
    ${CARBON_FRAMEWORK}
    ${METAL_FRAMEWORK}
    ${IOKIT_FRAMEWORK}
  )
endif()

# Xcode generator example:
# mkdir build && cd build
# cmake -G Xcode -DIPLUG2_DIR=/path/to/iPlug2 ..
