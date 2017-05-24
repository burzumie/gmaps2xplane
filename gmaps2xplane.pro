QT += core gui widgets network
CONFIG += qt console debug

TARGET = gmaps2xplane
TEMPLATE = app

SOURCES +=  main.cpp \
            calc_coord.cpp \
            network.cpp \
            gmap_loader.cpp \
            gmap_handler.cpp

HEADERS +=  calc_coord.h \
            network.h \
            gmap_loader.h \
            gmap_handler.h
