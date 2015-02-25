QT       += core sql testlib network
TARGET = plan
TEMPLATE = app

win32{
    INCLUDEPATH += "C:\boost_1_55_0"
}
unix{
    INCLUDEPATH += "/Users/artem/boost_1_55_0"
}

SOURCES += \
    main.cpp \
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
    ui.cpp \
    testplan.cpp \
    server.cpp \
    graph.cpp \
    ../myClient/packet.cpp

HEADERS += \
    section.h \
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
    ui.h \
    testplan.h \
    server.h \
    graph.h \
    ../myClient/packet.h
