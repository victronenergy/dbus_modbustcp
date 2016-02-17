#ifndef DBUS_SERVICES_H
#define DBUS_SERVICES_H

#include <QObject>
#include <QList>
#include <QMap>

class DBusService;
class VeQItem;

class VeQItem;

class DBusServices : public QObject
{
	Q_OBJECT
public:
	DBusServices(VeQItem *root, QObject *parent = 0);
	~DBusServices();
	void initialScan();
	DBusService *getService(QString deviceType, int deviceInstance);

signals:
	void dbusServiceFound(DBusService *service);

private slots:
	void onServiceAdded(VeQItem *item);

private:
	void addService(VeQItem *item);

	QMap<QString, DBusService *> mServicesByName;
	QMap<QString, QList<DBusService *> > mServiceByType;
	VeQItem *mRoot;
};

#endif
