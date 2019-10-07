TOP=../..

include $(TOP)/configure/CONFIG

# -------------------------------
# Build an Diamond Support Module
# -------------------------------

LIBRARY_IOC += WBPcs8000

DBD += WBPcs8000.dbd
WBPcs8000_DBD += Pcs.dbd

WBPcs8000_SRCS += pcsController.cpp
WBPcs8000_SRCS += pcsAxis.cpp
WBPcs8000_SRCS += XmlCommandConstructor.cpp

include $(TOP)/configure/RULES
