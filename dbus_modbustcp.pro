# version
VERSION = 1.0.55

# Add more folders to ship with the application here
unix {
    bindir = $$(bindir)
    DESTDIR = $$(DESTDIR)
    isEmpty(bindir) {
        bindir = /usr/local/bin
    }
    INSTALLS += target csv
    target.path = $${DESTDIR}$${bindir}
    csv.path = $${DESTDIR}$${bindir}
    csv.files = *.csv
}

#CONFIG(release, debug|release):
DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += VERSION=\\\"$${VERSION}\\\"

QT += core dbus network xml
QT -= gui

TARGET = dbus-modbustcp
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

MOC_DIR=.moc
OBJECTS_DIR=.obj

include(ext/QsLog/QsLog.pri)
include(ext/veutil/veutil.pri)

INCLUDEPATH += \
    ext/QsLog \
    ext/velib/inc

SOURCES += main.cpp \
    server.cpp \
    adu.cpp \
    pdu.cpp \
    backend.cpp \
    dbus_service.cpp \
    dbus_services.cpp \
    mappings.cpp \
    app.cpp \
    arguments.cpp \
    backend_request.cpp \
    mapping_request.cpp \
    diagnostics_service.cpp \
    ve_qitem_init_monitor.cpp \
    connection.cpp

HEADERS += \
    server.h \
    adu.h \
    pdu.h \
    backend.h \
    dbus_service.h \
    dbus_services.h \
    mappings.h \
    app.h \
    arguments.h \
    velib/velib_config_app.h \
    nostorage_qitem_producer.h \
    backend_request.h \
    mapping_request.h \
    diagnostics_service.h \
    ve_qitem_init_monitor.h \
    connection.h

OTHER_FILES += \
    attributes.csv \
    unitid2di.csv \
    modbustcp_tb.py

*g++* {
# suppress the mangling of va_arg has changed for gcc 4.4
QMAKE_CXXFLAGS += -Wno-psabi

# these warnings appear when compiling with QT4.8.3-debug. Problem appears to be
# solved in newer QT versions.
QMAKE_CXXFLAGS += -Wno-unused-local-typedefs
}

*clang* {
QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-local-typedef
}
