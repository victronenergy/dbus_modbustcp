# version and revision
VERSION = 0.8.0
REVISION = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --dirty --tags)

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

# Create a include file with VERSION / REVISION
version_rule.target = $$OUT_PWD/version.h
version_rule.commands = @echo \"updating file $$revtarget.target\"; \
	printf \"/* generated file (do not edit) */\\n \
	$${LITERAL_HASH}ifndef VERSION_H\\n \
	$${LITERAL_HASH}define VERSION_H\\n \
	$${LITERAL_HASH}define VERSION \\\"$${VERSION}\\\"\\n \
	$${LITERAL_HASH}define REVISION \\\"$${REVISION}\\\"\\n \
	$${LITERAL_HASH}endif\" > $$version_rule.target
version_rule.depends = FORCE
QMAKE_DISTCLEAN += $$version_rule.target

QMAKE_EXTRA_TARGETS += version_rule
PRE_TARGETDEPS += $$OUT_PWD/version.h

#CONFIG(release, debug|release):
DEFINES += QT_NO_DEBUG_OUTPUT

QT       += core
QT       -= gui
QT       += network
QT       += dbus

TARGET = dbus-modbustcp
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
	mappings.cpp \
	app.cpp \
	arguments.cpp

HEADERS += \
	server.h \
	adu.h \
	pdu.h \
	backend.h \
	dbus_service.h \
	dbus_services.h \
	busitem_interface.h \
	busitem_cons.h \
	mappings.h \
	app.h \
	arguments.h

OTHER_FILES += \
	attributes.csv \
	unitid2di.csv \
	modbustcp_tb.py

# suppress the mangling of va_arg has changed for gcc 4.4
QMAKE_CXXFLAGS += -Wno-psabi

# these warnings appear when compiling with QT4.8.3-debug. Problem appears to be
# solved in newer QT versions.
QMAKE_CXXFLAGS += -Wno-unused-local-typedefs
