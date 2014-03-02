#ifndef DBUS_SERVICE_H
#define DBUS_SERVICE_H

#include <QObject>
#include <QString>
#include "busitem_cons.h"

class DBusService : public QObject
{
	Q_OBJECT
public:
	enum DbusServiceType {
		DBUS_SERVICE_UNKNOWN,
		DBUS_SERVICE_LOGGER,
		DBUS_SERVICE_SETTINGS,
		DBUS_SERVICE_MULTI,
		DBUS_SERVICE_BATTERY,
		DBUS_SERVICE_SOLAR_CHARGER,
		DBUS_SERVICE_LYNX_ION,
		DBUS_SERVICE_PV_INVERTER,
		DBUS_SERVICE_MOTORDRIVE,
		DBUS_SERVICE_GPS
	};

	DBusService(const QString &name, QObject *parent = 0);
	QString getServiceName() const { return mDbusServiceName; }
	int getDeviceInstance() { return mDeviceInstance.getValue().toInt(); }

	bool getConnected() { return mConnected; }
	void setConnected(bool connected) { mConnected = connected; }
	void registerObjects(const QStringList &pathList);
	void registerObject(const QString &path);
	QVariant getValue(const QString path);
	static DbusServiceType getDeviceType(const QString &serviceName);

private:
	void setDeviceType(const QString &name);

	QString mDbusServiceName;
	DbusServiceType mServiceType;
	bool mConnected;
	BusItemCons mDeviceInstance;
	QHash<QString, BusItemCons *> mBusItems;
};

#endif
