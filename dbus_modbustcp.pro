#-------------------------------------------------
#
# Project created by QtCreator 2014-02-28T21:44:20
#
#-------------------------------------------------

# Add more folders to ship with the application, here
target.path = /opt/dbus_modbustcp
csv.path = /opt/dbus_modbustcp
csv.files = *.csv
INSTALLS += target csv

#DEPLOYMENTFOLDERS = attributes

machine=$$(MACHINE)
contains(machine,ccgx) {
	message($$(MACHINE))
	DEFINES += TARGET_ccgx
}

#CONFIG(release, debug|release):
DEFINES += QT_NO_DEBUG_OUTPUT

QT       += core
QT       -= gui
QT       += network
QT       += dbus

TARGET = dbus_modbustcp
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include(QsLog/QsLog.pri)

SOURCES += main.cpp \
	server.cpp \
	adu.cpp \
	pdu.cpp \
	backend.cpp \
	dbus_service.cpp \
	dbus_services.cpp \
	busitem_interface.cpp \
	busitem_cons.cpp \
	dbus.cpp \
	mappings.cpp \
	app.cpp \
	arguments.cpp

HEADERS += \
	server.h \
	adu.h \
	pdu.h \
	backend.h \
	defines.h \
	version.h \
	dbus_service.h \
	dbus_services.h \
	busitem_interface.h \
	busitem_cons.h \
	dbus.h \
	mappings.h \
	app.h \
	arguments.h

OTHER_FILES += \
	attributes.csv \
	unitid2di.csv

# suppress the mangling of va_arg has changed for gcc 4.4
QMAKE_CXXFLAGS += -Wno-psabi
