#-------------------------------------------------
#
# Project created by QtCreator 2015-01-22T15:36:43
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = TrackerServer
CONFIG   += console c++11
CONFIG   -= app_bundle

INCLUDEPATH += ./lib/TobiiSDK/include

LIBS += -L../../lib/TobiiSDK/lib/x64  # -lTobii.EyeX.Client

TEMPLATE = app


SOURCES += main.cpp \
    gaze.cpp

HEADERS += \
    gaze.h \
    TrackerServer.h
