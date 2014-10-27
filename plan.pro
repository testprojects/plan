QT       += core sql
TARGET = plan
TEMPLATE = app

INCLUDEPATH += "C:\boost_1_55_0"

SOURCES += \
    main.cpp \
    echelon.cpp \
    mydb.cpp \
    station.cpp \
    mytime.cpp \
    request.cpp \
    filtervertex.cpp \
    filteredge.cpp \
    graph.cpp \
    symbolconverter.cpp \
    section.cpp \
    stream.cpp \
    ps.cpp

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
    graph.h \
    symbolconverter.h \
    stream.h
