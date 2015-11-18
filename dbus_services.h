#ifndef DBUS_SERVICES_H
#define DBUS_SERVICES_H

#include <QObject>
#include <QMap>

class DBusService;
class VeQItem;

class DBusServices : public QObject
{
	Q_OBJECT
public:
	DBusServices(VeQItem *root, QObject *parent = 0);
	~DBusServices();
	void initialScan();
	int getCount() const { return mServicesByName.count(); }
	bool getConnected(const QString &serviceName);
	DBusService * getService(QString deviceType, int deviceInstance);

signals:
	void dbusServiceFound(DBusService *service);

private slots:
	void onServiceAdded(VeQItem *service);
	void onDeviceInstanceFound(VeQItem *service);

private:
	void addService(VeQItem *item);
	void removeService(VeQItem *item);

	QMap<QString, DBusService *> mServicesByName;
	QMap<QString, QMultiMap<int, DBusService *> > mServiceByType;
	VeQItem *mRoot;
};

#endif
