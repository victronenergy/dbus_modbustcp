#ifndef DBUS_SERVICES_H
#define DBUS_SERVICES_H

#include <QObject>
#include <QList>
#include <QHash>

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

	QHash<QString, DBusService *> mServicesByName;
	QHash<QString, QList<DBusService *> > mServiceByType;
	VeQItem *mRoot;
};

#endif
