TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#USR_CXXFLAGS += -Werror
USR_CXXFLAGS += -Wall -Wextra
USR_CXXFLAGS += -Wno-unused-parameter

PROD_IOC += tests
tests_SRCS += tests.cpp
tests_SRCS += test_pcsAxis.cpp
tests_SRCS += test_XmlCommandConstructor.cpp
tests_LIBS += $(EPICS_BASE_IOC_LIBS)

ifdef BOOST_LIB
	boost_unit_test_framework_DIR=$(BOOST_LIB)
	tests_LIBS += boost_unit_test_framework
else
	tests_SYS_LIBS += boost_unit_test_framework
endif

PROD_LIBS += motor asyn WBPcs8000

CXXFLAGS += -std=c++0x -pthread

USR_INCLUDES += $(XML2_INCLUDE)

include $(TOP)/configure/RULES
