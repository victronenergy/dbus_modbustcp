#ifndef APP_H
#define APP_H

#include <QDBusConnection>
#include <QObject>
#include <veutil/qt/ve_qitem.hpp>
#include "server.h"
#include "backend.h"
#include "dbus_services.h"
#include "mappings.h"

class App : public QObject
{
	Q_OBJECT
public:
	App(VeQItem *subRoot, VeQItem *pubRoot, int tcpPort, QObject *parent = 0);

private:
	Server mServer;
	Backend mBackend;
	DBusServices mDBusServices;
	Mappings mMapping;
	VeQItemSettings *mSettings;
};

#endif // APP_H
