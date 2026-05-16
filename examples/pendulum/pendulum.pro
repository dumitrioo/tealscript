QT       -= core
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++20 -march=native
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

DEFINES += TEALSCRIPT_NO_EIGEN
DEFINES += USE_CUSTOM_MEMORY_ALLOCATION

LIBS += -lraylib

INCLUDEPATH += ../../src \
    ./bullet

SOURCES += main.cpp \
    bullet/btBulletCollisionAll.cpp \
    bullet/btBulletDynamicsAll.cpp \
    bullet/btLinearMathAll.cpp

DISTFILES += \
    ../pidreg.teal
