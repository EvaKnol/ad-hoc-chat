QT += core gui widgets network

CONFIG += c++11

INCLUDEPATH += ./include

TARGET = chat
TEMPLATE = app

SOURCES += src/main.cpp\
           src/chatwindow.cpp \
           src/chatpanel.cpp \
           src/transceiver.cpp \
    src/Packet.cpp \
    src/RaftNode.cpp \
    src/Routing.cpp \
    src/Security.cpp

HEADERS += include/chatwindow.h \
           include/chatpanel.h \
           include/transceiver.h \
           include/Packet.h \
           include/RaftNode.h \
    include/Routing.h \
    include/Security.h \
    include/md5.h
