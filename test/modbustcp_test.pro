QMAKE_POST_LINK += cp $$PWD/../*.csv $$OUT_PWD

QT += core testlib
QT -= gui

TARGET = modbustcp_test
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

# suppress the mangling of va_arg has changed for gcc 4.4
QMAKE_CXXFLAGS += -Wno-psabi

# these warnings appear when compiling with QT4.8.3-debug. Problem appears to be
# solved in newer QT versions.
QMAKE_CXXFLAGS += -Wno-unused-local-typedefs

# gcc complains about this while compiling google test
QMAKE_CXXFLAGS += -Wno-missing-field-initializers

MOC_DIR=.moc
OBJECTS_DIR=.obj

SRCDIR = ..
EXTDIR = ../ext
TESTDIR = ../src

include($$EXTDIR/QsLog/QsLog.pri)
include($$EXTDIR/velib/src/qt/ve_qitems.pri)

INCLUDEPATH += \
    $$EXTDIR/velib/inc \
    $$SRCDIR \
    $$TESTDIR

HEADERS += \
    mapping_test.h \
    $$SRCDIR/dbus_services.h \
    $$SRCDIR/dbus_service.h \
    $$SRCDIR/diagnostics_service.h \
    $$SRCDIR/mappings.h \
    $$SRCDIR/mapping_request.h \
    $$SRCDIR/ve_qitem_init_monitor.h \
    diagnostics_service_test.h \
    mapping_test.h

SOURCES += \
    main.cpp \
    $$SRCDIR/dbus_services.cpp \
    $$SRCDIR/dbus_service.cpp \
    $$SRCDIR/diagnostics_service.cpp \
    $$SRCDIR/mappings.cpp \
    $$SRCDIR/mapping_request.cpp \
    $$SRCDIR/ve_qitem_init_monitor.cpp
