#ifndef DBUS_SERVICE_H
#define DBUS_SERVICE_H

#include <QObject>
#include <QHash>
#include <QString>

class VeQItem;

class DBusService : public QObject
{
	Q_OBJECT
public:
	DBusService(VeQItem *serviceRoot, QObject *parent = 0);
	VeQItem *getServiceRoot() const
	{
		return mServiceRoot;
	}
	VeQItem *getDeviceInstance() const
	{
		return mDeviceInstance;
	}
	VeQItem *getItem(const QString &path);
	static QString getDeviceType(const QString &serviceName);

private slots:
	void onDeviceInstanceChanged();

private:
	QHash<QString, VeQItem *> mItems;
	VeQItem *mServiceRoot;
	VeQItem *mDeviceInstance;
};

#endif
