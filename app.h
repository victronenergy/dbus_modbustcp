#ifndef APP_H
#define APP_H

#include <QObject>
#include "server.h"
#include "backend.h"
#include "dbus_services.h"
#include "mappings.h"

class App : public QObject
{
	Q_OBJECT
public:
	App(VeQItem *dbusRoot, QObject *parent = 0);

signals:

public slots:

private:
	Server mServer;
	Backend mBackend;
	DBusServices mDBusServices;
	Mappings mMapping;
};

#endif // APP_H
