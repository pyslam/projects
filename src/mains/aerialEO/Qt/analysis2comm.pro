# =========================================================================
# Last updated on 11/25/09; 5/5/10; 5/18/10
# =========================================================================

PREFIX = $(HOME)/programs/c++/svn/projects
CONFIGDIR = $$PREFIX/config
WEBDIR = $$PREFIX/src/Qt/web/
include ($$CONFIGDIR/common.pro)
RTPSDIR = $$PREFIX/src/Qt/rtps

SOURCES = analysis2comm.cc \
	  $$WEBDIR/VideoServer.cc \
	  $$WEBDIR/RTPSButtonServer.cc \
	  $$WEBDIR/SKSClient.cc \
	  $$WEBDIR/SKSDataServerInterfacer.cc \
	  $$WEBDIR/DOMParser.cc \
	  $$WEBDIR/ImageClient.cc \
	  $$WEBDIR/ImageServer.cc \
	  $$WEBDIR/WebServer.cc \
	  $$WEBDIR/WebClient.cc \
	  $$RTPSDIR/Console.cpp \
	  $$RTPSDIR/MessageWrapper.cpp \
	  $$RTPSDIR/NFOVCommand.cpp \
	  $$RTPSDIR/NFOVImageUpdate.cpp \
	  $$RTPSDIR/ROICommand.cpp \
	  $$RTPSDIR/ROIImageUpdate.cpp \
	  $$RTPSDIR/ROITrackElement.cpp \
	  $$RTPSDIR/ROITrackUpdate.cpp \
	  $$RTPSDIR/RTPSMessenger.cc \
	  $$RTPSDIR/SystemStatus.cpp \
	  $$RTPSDIR/UTMdef.cpp \
	  $$RTPSDIR/WFOVImageUpdate.cpp 

HEADERS = $$WEBDIR/VideoServer.h \
	  $$WEBDIR/RTPSButtonServer.h \
	  $$WEBDIR/SKSClient.h \
	  $$WEBDIR/SKSDataServerInterfacer.h \
	  $$WEBDIR/DOMParser.h \
	  $$WEBDIR/ImageClient.h \
	  $$WEBDIR/ImageServer.h \
	  $$WEBDIR/WebServer.h \
	  $$WEBDIR/WebClient.h \
	  $$RTPSDIR/Console.h \
	  $$RTPSDIR/MessageWrapper.h \
	  $$RTPSDIR/NFOVCommand.h \
	  $$RTPSDIR/NFOVImageUpdate.h \
	  $$RTPSDIR/ROICommand.h \
	  $$RTPSDIR/ROIImageUpdate.h \
	  $$RTPSDIR/ROITrackElement.h \
	  $$RTPSDIR/ROITrackUpdate.h \
	  $$RTPSDIR/RTPSMessenger.h \
	  $$RTPSDIR/setup.h \
	  $$RTPSDIR/SystemStatus.h \
	  $$RTPSDIR/UTMdef.h \
	  $$RTPSDIR/WFOVImageUpdate.h 

TARGET = analysis2comm

TEMPLATE_TYPE = app

QT += network thread xml
QT -= gui

CONFIG += qt 
