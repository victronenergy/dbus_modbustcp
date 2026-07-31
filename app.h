#ifndef APP_H
#define APP_H

#include <QDBusConnection>
#include <QObject>
#include <veutil/qt/ve_qitem.hpp>
#include "backend.h"
#include "dbus_services.h"
#include "mappings.h"

class App : public QObject
{
	Q_OBJECT
public:
	// transport is an already-constructed Modbus transport (a Server for TCP or an
	// RtuServer for RTU). It must expose modbusRequest(ADU*)/modbusReply(ADU*) and
	// is reparented to this App. This keeps App transport-agnostic so each binary
	// (dbus-modbustcp / dbus-modbusrtu) only differs in which transport it builds.
	// Pass a null pubRoot to skip exporting the com.victronenergy.modbustcp
	// diagnostics service (the RTU slave does not publish one).
	App(VeQItem *subRoot, VeQItem *pubRoot, QObject *transport, QObject *parent = 0);

private:
	QObject *mTransport;
	Backend mBackend;
	DBusServices mDBusServices;
	Mappings mMapping;
	VeQItemSettings *mSettings;
};

#endif // APP_H
