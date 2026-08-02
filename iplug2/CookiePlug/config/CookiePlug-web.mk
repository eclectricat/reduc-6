# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/CookiePlug/config
IPLUG2_ROOT = ../../../../../../../Projekte/AudioPlugin/iPlug2

include ../../../../../../../Projekte/AudioPlugin/iPlug2/common-web.mk

SRC += $(PROJECT_ROOT)/CookiePlug.cpp

# WAM_SRC +=

# WAM_CFLAGS +=

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WAM_LDFLAGS += -O3 -s EXPORT_NAME="'ModuleFactory'" -s ASSERTIONS=0

WEB_LDFLAGS += -O3 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
