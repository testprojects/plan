QT       += core sql testlib network xml
CONFIG += log4qt
TARGET = plan
TEMPLATE = app

win32:INCLUDEPATH += ../boost_1_55_0
unix:INCLUDEPATH += /Users/artem/boost_1_55_0

include(./log4qt/src/log4qt.pri)

DEFINES += LOG4QT_VERSION_STR='\\"1.0.0\\"'

SOURCES += main.cpp \
    echelon.cpp \
    mydb.cpp \
    station.cpp \
    mytime.cpp \
    request.cpp \
    filtervertex.cpp \
    filteredge.cpp \
    symbolconverter.cpp \
    section.cpp \
    stream.cpp \
    ps.cpp \
    programsettings.cpp \
    pvr.cpp \
    testplan.cpp \
    server.cpp \
    graph.cpp \
    ../myClient/packet.cpp \
    documentsformer.cpp \
    planthread.cpp \
    pauser.cpp \
    sortfilterstream.cpp

HEADERS += section.h \
    station.h \
    pvr.h \
    echelon.h \
    mydb.h \
    request.h \
    ps.h \
    mytime.h \
    filtervertex.h \
    filteredge.h \
    symbolconverter.h \
    stream.h \
    programsettings.h \
    testplan.h \
    server.h \
    graph.h \
    ../myClient/packet.h \
    documentsformer.h \
    planthread.h \
    pauser.h \
    sortfilterstream.h
