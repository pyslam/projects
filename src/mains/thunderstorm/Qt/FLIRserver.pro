# =========================================================================
# Last updated on 10/20/11; 10/24/11
# =========================================================================

PREFIX = $(HOME)/programs/c++/svn/projects
CONFIGDIR = $$PREFIX/config
WEBDIR = $$PREFIX/src/Qt/web/
include ($$CONFIGDIR/common.pro)

SOURCES = flirservertest.cc \
	  $$WEBDIR/FLIRServer.cc 

HEADERS = $$WEBDIR/FLIRServer.h 


TARGET = flirservertest

TEMPLATE_TYPE = app

QT += network thread xml
QT -= gui

CONFIG += qt 
