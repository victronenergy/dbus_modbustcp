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
	~DBusServices();
	void initialScan();
	int getCount() const { return mServicesByName.count(); }
	bool getConnected(const QString &serviceName);
	DBusService * getService(QString deviceType, int deviceInstance);

signals:
	void dbusServiceFound(DBusService *service);
	void dbusServiceConnected(DBusService *service);
	void dbusServiceDisconnected(DBusService *service);

private slots:
	void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

private:
	void addService(const QString &name);
	void removeService(const QString &name);
	QMap<QString, DBusService *> mServicesByName;
	QMap<QString, QMultiMap<int, DBusService *> > mServiceByType;
	QDBusConnection mDBus;
};

#endif
