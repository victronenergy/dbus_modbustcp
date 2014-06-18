#ifndef DBUS_SERVICE_H
#define DBUS_SERVICE_H

#include <QObject>
#include <QString>
#include "busitem_cons.h"

class DBusService : public QObject
{
	Q_OBJECT
public:
	DBusService(const QString &name, QObject *parent = 0);
	QString getServiceName() const { return mDbusServiceName; }
	int getDeviceInstance() { return mDeviceInstance.getValue().toInt(); }

	bool getConnected() { return mConnected; }
	void setConnected(bool connected) { mConnected = connected; }
	void registerObjects(const QStringList &pathList);
	void registerObject(const QString &path);
	QVariant getValue(const QString path);
	static QString getDeviceType(const QString &serviceName);

private:
	void setDeviceType(const QString &name);

	QString mDbusServiceName;
	QString mServiceType;
	bool mConnected;
	BusItemCons mDeviceInstance;
	QHash<QString, BusItemCons *> mBusItems;
};

#endif
