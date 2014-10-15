QT       += core sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = plan
TEMPLATE = app
#CONFIG += c++11
#CONFIG -= debug_and_release release
QMAKE_CXXFLAGS += -fno-elide-constructors
QMAKE_CXXFLAGS -= -pipe

INCLUDEPATH += "C:\boost_1_55_0"

SOURCES += \
    main.cpp \
    route.cpp \
    echelon.cpp \
    mydb.cpp \
    station.cpp \
    mytime.cpp \
    request.cpp \
    filtervertex.cpp \
    filteredge.cpp \
    graph.cpp \
    symbolconverter.cpp \
    section.cpp

HEADERS += \
    section.h \
    station.h \
    pvr.h \
    route.h \
    echelon.h \
    mydb.h \
    request.h \
    ps.h \
    mytime.h \
    filtervertex.h \
    filteredge.h \
    graph.h \
    symbolconverter.h
