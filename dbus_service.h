#ifndef DBUS_SERVICE_H
#define DBUS_SERVICE_H

#include <QObject>
#include <QString>

class VeQItem;

class DBusService : public QObject
{
	Q_OBJECT
public:
	DBusService(VeQItem *serviceRoot, QObject *parent = 0);
	VeQItem *getServiceRoot() const;
	VeQItem *getDeviceInstance() const;
	static QString getDeviceType(const QString &serviceName);

private slots:
	void onDeviceInstanceChanged();

private:
	VeQItem *mServiceRoot;
	VeQItem *mDeviceInstance;
};

#endif
