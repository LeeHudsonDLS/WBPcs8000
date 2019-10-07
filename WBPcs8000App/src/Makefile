TOP=../..

include $(TOP)/configure/CONFIG

USR_CXXFLAGS += -Wall -Wextra
USR_CXXFLAGS += -Wno-unused-parameter
# -------------------------------
# Build an Diamond Support Module
# -------------------------------

LIBRARY_IOC += WBPcs8000

DBD += WBPcs8000.dbd
WBPcs8000_DBD += Pcs.dbd

WBPcs8000_SRCS += pcsController.cpp
WBPcs8000_SRCS += pcsAxis.cpp
WBPcs8000_SRCS += XmlCommandConstructor.cpp
WBPcs8000_SRCS += Sequencer.cpp

USR_INCLUDES += $(XML2_INCLUDE)
include $(TOP)/configure/RULES
