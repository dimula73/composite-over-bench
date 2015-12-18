#-------------------------------------------------
#
# Project created by QtCreator 2012-09-27T16:03:43
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_testavxcompositeovertest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += __STDC_LIMIT_MACROS
QMAKE_CXXFLAGS += -mavx


SOURCES += tst_testavxcompositeovertest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

QMAKE_CXXFLAGS +=  -Wnon-virtual-dtor -Wno-long-long -ansi -Wundef -Wcast-align -Wchar-subscripts -Wall -W -Wpointer-arith -Wformat-security -fno-exceptions -DQT_NO_EXCEPTIONS -fno-check-new -fno-common -Woverloaded-virtual -fno-threadsafe-statics -fvisibility=hidden -Werror=return-type -fvisibility-inlines-hidden  -Wabi -fabi-version=0 -O3


#-march=corei7-avx -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -msse2avx -mno-sse4a -mno-xop -mno-fma4   

QMAKE_CXXFLAGS += -I/home/devel/kde-install/calligra/include
LIBS += -L /home/devel/kde-install/calligra/lib/ -lVc

LIBS += -L/home/devel/cuda/lib64/ -lcudart ../cuda_code.o
