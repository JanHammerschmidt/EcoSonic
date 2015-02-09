#-------------------------------------------------
#
# Project created by QtCreator 2014-04-01T18:21:30
#
#-------------------------------------------------

QT       += core gui
QT      += svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport multimedia concurrent network

TARGET = car_simulator
TEMPLATE = app

#QMAKE_LFLAGS += -F/System/Library/Frameworks/Kernel.framework/Versions/A/Headers/IOKit

LIBS += -framework IOKit
LIBS += -framework CoreFoundation
LIBS += -L/Users/jhammers/boost_1_57_0/stage/lib -lboost_thread -lboost_system
LIBS += -L$$PWD/lib/quazip/release -lquazip -lz

DEPENDPATH += . \
    ./include
INCLUDEPATH += . \
    ./include \
    ./lib/oscpack_1_1_0 \
    ./lib/HID \
    ./lib/eyetribe/include \
    ./lib/quazip/quazip \
    /Users/jhammers/boost_1_57_0

CONFIG += c++11 precompile_header
PRECOMPILED_HEADER = stdafx.h

SOURCES += main.cpp\
        mainwindow.cpp \
    engine.cpp \
    lib/qcustomplot/qcustomplot.cpp \
    lib/oscpack_1_1_0/osc/OscOutboundPacketStream.cpp \
    lib/oscpack_1_1_0/osc/OscPrintReceivedElements.cpp \
    lib/oscpack_1_1_0/osc/OscReceivedElements.cpp \
    lib/oscpack_1_1_0/osc/OscTypes.cpp \
    lib/oscpack_1_1_0/ip/IpEndpointName.cpp \
    lib/oscpack_1_1_0/ip/posix/NetworkingUtils.cpp \
    lib/oscpack_1_1_0/ip/posix/UdpSocket.cpp \
    qtrackeditor.cpp \
    lib/HID/HID.cpp \
    qcarviz.cpp \
    car.cpp \
    hudwindow.cpp \
    qhudwidget.cpp \
    hud.cpp

HEADERS  += mainwindow.h \
    engine.h \
    lib/qcustomplot/qcustomplot.h \
    lib/oscpack_1_1_0/osc/MessageMappingOscPacketListener.h \
    lib/oscpack_1_1_0/osc/OscException.h \
    lib/oscpack_1_1_0/osc/OscHostEndianness.h \
    lib/oscpack_1_1_0/osc/OscOutboundPacketStream.h \
    lib/oscpack_1_1_0/osc/OscPacketListener.h \
    lib/oscpack_1_1_0/osc/OscPrintReceivedElements.h \
    lib/oscpack_1_1_0/osc/OscReceivedElements.h \
    lib/oscpack_1_1_0/osc/OscTypes.h \
    lib/oscpack_1_1_0/ip/IpEndpointName.h \
    lib/oscpack_1_1_0/ip/NetworkingUtils.h \
    lib/oscpack_1_1_0/ip/PacketListener.h \
    lib/oscpack_1_1_0/ip/TimerListener.h \
    lib/oscpack_1_1_0/ip/UdpSocket.h \
    consumption_map.h \
    torque_map.h \
    gearbox.h \
    car.h \
    resistances.h \
    qcarviz.h \
    qtrackeditor.h \
    track.h \
    lib/HID/HID.h \
    OSCSender.h \
    hud.h \
    speed_observer.h \
    misc.h \
    KeyboardInput.h \
    logging.h \
    wingman_input.h \
    hudwindow.h \
    qhudwidget.h \
    stdafx.h

FORMS    += mainwindow.ui \
    hudwindow.ui
