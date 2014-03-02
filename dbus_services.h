#ifndef DBUS_SERVICES_H
#define DBUS_SERVICES_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include "dbus_service.h"

class DBusServices : public QObject
{
	Q_OBJECT
public:
	DBusServices(QObject *parent = 0);
	void initialScan();
	int getCount() const { return mServicesByName.count(); }
	bool getConnected(const QString &serviceName);
	DBusService * getService(DBusService::DbusServiceType deviceType, int deviceInstance);

signals:
	void dbusServiceFound(DBusService *service);
	void dbusServiceConnected(DBusService *service);
	void dbusServiceDisconnected(DBusService *service);

private slots:
	void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

private:
	void processServiceName(QString name);
	QMap<QString, DBusService *> mServicesByName;
	QMap<DBusService::DbusServiceType, QMap<int, DBusService *> > mServiceByType;
	QDBusConnection mDBus;
};

#endif
