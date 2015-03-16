QT       += core sql testlib network xml
TARGET = plan
TEMPLATE = app

win32:INCLUDEPATH += ../boost_1_55_0
unix:INCLUDEPATH += /Users/artem/boost_1_55_0

include(./log4qt/src/log4qt/log4qt.pri)

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
    filterstream.cpp \
    loopwrapper.cpp \
    documentsformer.cpp

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
    filterstream.h \
    loopwrapper.h \
    documentsformer.h

