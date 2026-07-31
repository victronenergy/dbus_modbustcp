# Modbus RTU serial slave binary.

TARGET = dbus-modbusrtu

QT += serialport

SOURCES += \
    main_rtu.cpp \
    rtu_server.cpp \
    rtu_adu.cpp \
    crc.cpp

HEADERS += \
    rtu_server.h \
    rtu_adu.h \
    crc.h

include(common.pri)
