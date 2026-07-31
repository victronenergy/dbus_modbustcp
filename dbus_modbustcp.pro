# Modbus TCP server binary.

TARGET = dbus-modbustcp

SOURCES += \
    main.cpp \
    server.cpp \
    tcp_adu.cpp \
    connection.cpp

HEADERS += \
    server.h \
    tcp_adu.h \
    connection.h

include(common.pri)
